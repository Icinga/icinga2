/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "remote/consolehandler.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "config/configcompiler.hpp"
#include "base/configtype.hpp"
#include "base/configwriter.hpp"
#include "base/scriptglobal.hpp"
#include "base/logger.hpp"
#include "base/serializer.hpp"
#include "base/timer.hpp"
#include "base/initialize.hpp"
#include <boost/algorithm/string.hpp>
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/console", ConsoleHandler);

static int l_ExternalCommands = 0;
static boost::mutex l_QueryMutex;
static std::map<String, ApiScriptFrame> l_ApiScriptFrames;
static Timer::Ptr l_FrameCleanupTimer;
static boost::mutex l_ApiScriptMutex;

static void ScriptFrameCleanupHandler(void)
{
	boost::mutex::scoped_lock lock(l_ApiScriptMutex);

	std::vector<String> cleanup_keys;

	typedef std::pair<String, ApiScriptFrame> KVPair;

	BOOST_FOREACH(const KVPair& kv, l_ApiScriptFrames) {
		if (kv.second.Seen < Utility::GetTime() - 1800)
			cleanup_keys.push_back(kv.first);
	}

	BOOST_FOREACH(const String& key, cleanup_keys)
		l_ApiScriptFrames.erase(key);
}

static void InitScriptFrameCleanup(void)
{
	l_FrameCleanupTimer = new Timer();
	l_FrameCleanupTimer->OnTimerExpired.connect(boost::bind(ScriptFrameCleanupHandler));
	l_FrameCleanupTimer->SetInterval(30);
	l_FrameCleanupTimer->Start();
}

INITIALIZE_ONCE(InitScriptFrameCleanup);

bool ConsoleHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	if (request.RequestUrl->GetPath().size() > 3)
		return false;

	if (request.RequestMethod != "POST")
		return false;

	QueryDescription qd;
	Dictionary::Ptr params = HttpUtility::FetchRequestParameters(request);

	String methodName = request.RequestUrl->GetPath()[2];
	
	FilterUtility::CheckPermission(user, "console");

	String session = HttpUtility::GetLastParameter(params, "session");

	if (session.IsEmpty())
		session = Utility::NewUniqueID();

	String command = HttpUtility::GetLastParameter(params, "command");

	bool sandboxed = HttpUtility::GetLastParameter(params, "sandboxed");

	if (methodName == "execute-script")
		return ExecuteScriptHelper(request, response, command, session, sandboxed);
	else if (methodName == "auto-complete-script")
		return AutocompleteScriptHelper(request, response, command, session, sandboxed);

	HttpUtility::SendJsonError(response, 400, "Invalid method specified: " + methodName);
	return true;
}

bool ConsoleHandler::ExecuteScriptHelper(HttpRequest& request, HttpResponse& response,
    const String& command, const String& session, bool sandboxed)
{
	Log(LogInformation, "Console")
	    << "Executing expression: " << command;

	ApiScriptFrame& lsf = l_ApiScriptFrames[session];
	lsf.Seen = Utility::GetTime();

	if (!lsf.Locals)
		lsf.Locals = new Dictionary();

	String fileName = "<" + Convert::ToString(lsf.NextLine) + ">";
	lsf.NextLine++;

	lsf.Lines[fileName] = command;

	Array::Ptr results = new Array();
	Dictionary::Ptr resultInfo = new Dictionary();
	Expression *expr = NULL;
	Value exprResult;

	try {
		expr = ConfigCompiler::CompileText(fileName, command);

		ScriptFrame frame;
		frame.Locals = lsf.Locals;
		frame.Self = lsf.Locals;
		frame.Sandboxed = sandboxed;

		exprResult = expr->Evaluate(frame);

		resultInfo->Set("code", 200);
		resultInfo->Set("status", "Executed successfully.");
		resultInfo->Set("result", Serialize(exprResult, 0));
	} catch (const ScriptError& ex) {
		DebugInfo di = ex.GetDebugInfo();

		std::ostringstream msgbuf;

		msgbuf << di.Path << ": " << lsf.Lines[di.Path] << "\n"
		    << String(di.Path.GetLength() + 2, ' ')
		    << String(di.FirstColumn, ' ') << String(di.LastColumn - di.FirstColumn + 1, '^') << "\n"
		    << ex.what() << "\n";

		resultInfo->Set("code", 500);
		resultInfo->Set("status", String(msgbuf.str()));
		resultInfo->Set("incomplete_expression", ex.IsIncompleteExpression());

		Dictionary::Ptr debugInfo = new Dictionary();
		debugInfo->Set("path", di.Path);
		debugInfo->Set("first_line", di.FirstLine);
		debugInfo->Set("first_column", di.FirstColumn);
		debugInfo->Set("last_line", di.LastLine);
		debugInfo->Set("last_column", di.LastColumn);
		resultInfo->Set("debug_info", debugInfo);
	} catch (...) {
		delete expr;
		throw;
	}
	delete expr;

	results->Add(resultInfo);

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);

	return true;
}

bool ConsoleHandler::AutocompleteScriptHelper(HttpRequest& request, HttpResponse& response,
    const String& command, const String& session, bool sandboxed)
{
	Log(LogInformation, "Console")
	    << "Auto-completing expression: " << command;

	ApiScriptFrame& lsf = l_ApiScriptFrames[session];
	lsf.Seen = Utility::GetTime();

	if (!lsf.Locals)
		lsf.Locals = new Dictionary();

	Array::Ptr results = new Array();
	Dictionary::Ptr resultInfo = new Dictionary();

	ScriptFrame frame;
	frame.Locals = lsf.Locals;
	frame.Self = lsf.Locals;
	frame.Sandboxed = sandboxed;

	resultInfo->Set("code", 200);
	resultInfo->Set("status", "Auto-completed successfully.");
	resultInfo->Set("suggestions", Array::FromVector(GetAutocompletionSuggestions(command, frame)));

	results->Add(resultInfo);

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);

	return true;
}

static void AddSuggestion(std::vector<String>& matches, const String& word, const String& suggestion)
{
	if (suggestion.Find(word) != 0)
		return;

	matches.push_back(suggestion);
}

std::vector<String> ConsoleHandler::GetAutocompletionSuggestions(const String& word, ScriptFrame& frame)
{	
	std::vector<String> matches;

	BOOST_FOREACH(const String& keyword, ConfigWriter::GetKeywords()) {
		AddSuggestion(matches, word, keyword);
	}

	{
		ObjectLock olock(frame.Locals);
		BOOST_FOREACH(const Dictionary::Pair& kv, frame.Locals) {
			AddSuggestion(matches, word, kv.first);
		}
	}

	{
		ObjectLock olock(ScriptGlobal::GetGlobals());
		BOOST_FOREACH(const Dictionary::Pair& kv, ScriptGlobal::GetGlobals()) {
			AddSuggestion(matches, word, kv.first);
		}
	}

	String::SizeType cperiod = word.RFind(".");

	if (cperiod != -1) {
		String pword = word.SubStr(0, cperiod);

		Value value;

		try {
			Expression *expr = ConfigCompiler::CompileText("temp", pword);

			if (expr)
				value = expr->Evaluate(frame);

			if (value.IsObjectType<Dictionary>()) {
				Dictionary::Ptr dict = value;

				ObjectLock olock(dict);
				BOOST_FOREACH(const Dictionary::Pair& kv, dict) {
					AddSuggestion(matches, word, pword + "." + kv.first);
				}
			}

			Type::Ptr type = value.GetReflectionType();

			for (int i = 0; i < type->GetFieldCount(); i++) {
				Field field = type->GetFieldInfo(i);

				AddSuggestion(matches, word, pword + "." + field.Name);
			}

			while (type) {
				Object::Ptr prototype = type->GetPrototype();
				Dictionary::Ptr dict = dynamic_pointer_cast<Dictionary>(prototype);

				if (dict) {
					ObjectLock olock(dict);
					BOOST_FOREACH(const Dictionary::Pair& kv, dict) {
						AddSuggestion(matches, word, pword + "." + kv.first);
					}
				}

				type = type->GetBaseType();
			}
		} catch (...) { /* Ignore the exception */ }
	}

	return matches;
}
