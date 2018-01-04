/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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
#include <boost/thread/once.hpp>
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/console", ConsoleHandler);

static boost::mutex l_QueryMutex;
static std::map<String, ApiScriptFrame> l_ApiScriptFrames;
static Timer::Ptr l_FrameCleanupTimer;
static boost::mutex l_ApiScriptMutex;

static void ScriptFrameCleanupHandler()
{
	boost::mutex::scoped_lock lock(l_ApiScriptMutex);

	std::vector<String> cleanup_keys;

	typedef std::pair<String, ApiScriptFrame> KVPair;

	for (const KVPair& kv : l_ApiScriptFrames) {
		if (kv.second.Seen < Utility::GetTime() - 1800)
			cleanup_keys.push_back(kv.first);
	}

	for (const String& key : cleanup_keys)
		l_ApiScriptFrames.erase(key);
}

static void EnsureFrameCleanupTimer()
{
	static boost::once_flag once = BOOST_ONCE_INIT;

	boost::call_once(once, []() {
		l_FrameCleanupTimer = new Timer();
		l_FrameCleanupTimer->OnTimerExpired.connect(std::bind(ScriptFrameCleanupHandler));
		l_FrameCleanupTimer->SetInterval(30);
		l_FrameCleanupTimer->Start();
	});
}

bool ConsoleHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response, const Dictionary::Ptr& params)
{
	if (request.RequestUrl->GetPath().size() > 3)
		return false;

	if (request.RequestMethod != "POST")
		return false;

	QueryDescription qd;

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
	Log(LogNotice, "Console")
		<< "Executing expression: " << command;

	EnsureFrameCleanupTimer();

	ApiScriptFrame& lsf = l_ApiScriptFrames[session];
	lsf.Seen = Utility::GetTime();

	if (!lsf.Locals)
		lsf.Locals = new Dictionary();

	String fileName = "<" + Convert::ToString(lsf.NextLine) + ">";
	lsf.NextLine++;

	lsf.Lines[fileName] = command;

	Array::Ptr results = new Array();
	Dictionary::Ptr resultInfo = new Dictionary();
	std::unique_ptr<Expression> expr;
	Value exprResult;

	try {
		expr = ConfigCompiler::CompileText(fileName, command);

		ScriptFrame frame(true);
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
	}

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

	EnsureFrameCleanupTimer();

	ApiScriptFrame& lsf = l_ApiScriptFrames[session];
	lsf.Seen = Utility::GetTime();

	if (!lsf.Locals)
		lsf.Locals = new Dictionary();

	Array::Ptr results = new Array();
	Dictionary::Ptr resultInfo = new Dictionary();

	ScriptFrame frame(true);
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

static void AddSuggestions(std::vector<String>& matches, const String& word, const String& pword, bool withFields, const Value& value)
{
	String prefix;

	if (!pword.IsEmpty())
		prefix = pword + ".";

	if (value.IsObjectType<Dictionary>()) {
		Dictionary::Ptr dict = value;

		ObjectLock olock(dict);
		for (const Dictionary::Pair& kv : dict) {
			AddSuggestion(matches, word, prefix + kv.first);
		}
	}

	if (withFields) {
		Type::Ptr type = value.GetReflectionType();

		for (int i = 0; i < type->GetFieldCount(); i++) {
			Field field = type->GetFieldInfo(i);

			AddSuggestion(matches, word, prefix + field.Name);
		}

		while (type) {
			Object::Ptr prototype = type->GetPrototype();
			Dictionary::Ptr dict = dynamic_pointer_cast<Dictionary>(prototype);

			if (dict) {
				ObjectLock olock(dict);
				for (const Dictionary::Pair& kv : dict) {
					AddSuggestion(matches, word, prefix + kv.first);
				}
			}

			type = type->GetBaseType();
		}
	}
}

std::vector<String> ConsoleHandler::GetAutocompletionSuggestions(const String& word, ScriptFrame& frame)
{
	std::vector<String> matches;

	for (const String& keyword : ConfigWriter::GetKeywords()) {
		AddSuggestion(matches, word, keyword);
	}

	{
		ObjectLock olock(frame.Locals);
		for (const Dictionary::Pair& kv : frame.Locals) {
			AddSuggestion(matches, word, kv.first);
		}
	}

	{
		ObjectLock olock(ScriptGlobal::GetGlobals());
		for (const Dictionary::Pair& kv : ScriptGlobal::GetGlobals()) {
			AddSuggestion(matches, word, kv.first);
		}
	}

	{
		Array::Ptr imports = ScriptFrame::GetImports();
		ObjectLock olock(imports);
		for (const Value& import : imports) {
			AddSuggestions(matches, word, "", false, import);
		}
	}

	String::SizeType cperiod = word.RFind(".");

	if (cperiod != String::NPos) {
		String pword = word.SubStr(0, cperiod);

		Value value;

		try {
			std::unique_ptr<Expression> expr = ConfigCompiler::CompileText("temp", pword);

			if (expr)
				value = expr->Evaluate(frame);

			AddSuggestions(matches, word, pword, true, value);
		} catch (...) { /* Ignore the exception */ }
	}

	return matches;
}
