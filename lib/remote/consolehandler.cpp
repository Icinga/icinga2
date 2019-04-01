/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
#include "base/namespace.hpp"
#include "base/initialize.hpp"
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

bool ConsoleHandler::HandleRequest(
	AsioTlsStream& stream,
	const ApiUser::Ptr& user,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	const Url::Ptr& url,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	const Dictionary::Ptr& params,
	boost::asio::yield_context& yc,
	bool& hasStartedStreaming
)
{
	namespace http = boost::beast::http;

	if (url->GetPath().size() != 3)
		return false;

	if (request.method() != http::verb::post)
		return false;

	QueryDescription qd;

	String methodName = url->GetPath()[2];

	FilterUtility::CheckPermission(user, "console");

	String session = HttpUtility::GetLastParameter(params, "session");

	if (session.IsEmpty())
		session = Utility::NewUniqueID();

	String command = HttpUtility::GetLastParameter(params, "command");

	bool sandboxed = HttpUtility::GetLastParameter(params, "sandboxed");

	if (methodName == "execute-script")
		return ExecuteScriptHelper(request, response, params, command, session, sandboxed);
	else if (methodName == "auto-complete-script")
		return AutocompleteScriptHelper(request, response, params, command, session, sandboxed);

	HttpUtility::SendJsonError(response, params, 400, "Invalid method specified: " + methodName);
	return true;
}

bool ConsoleHandler::ExecuteScriptHelper(boost::beast::http::request<boost::beast::http::string_body>& request,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	const Dictionary::Ptr& params, const String& command, const String& session, bool sandboxed)
{
	namespace http = boost::beast::http;

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

	Dictionary::Ptr resultInfo;
	std::unique_ptr<Expression> expr;
	Value exprResult;

	try {
		expr = ConfigCompiler::CompileText(fileName, command);

		ScriptFrame frame(true);
		frame.Locals = lsf.Locals;
		frame.Self = lsf.Locals;
		frame.Sandboxed = sandboxed;

		exprResult = expr->Evaluate(frame);

		resultInfo = new Dictionary({
			{ "code", 200 },
			{ "status", "Executed successfully." },
			{ "result", Serialize(exprResult, 0) }
		});
	} catch (const ScriptError& ex) {
		DebugInfo di = ex.GetDebugInfo();

		std::ostringstream msgbuf;

		msgbuf << di.Path << ": " << lsf.Lines[di.Path] << "\n"
			<< String(di.Path.GetLength() + 2, ' ')
			<< String(di.FirstColumn, ' ') << String(di.LastColumn - di.FirstColumn + 1, '^') << "\n"
			<< ex.what() << "\n";

		resultInfo = new Dictionary({
			{ "code", 500 },
			{ "status", String(msgbuf.str()) },
			{ "incomplete_expression", ex.IsIncompleteExpression() },
			{ "debug_info", new Dictionary({
				{ "path", di.Path },
				{ "first_line", di.FirstLine },
				{ "first_column", di.FirstColumn },
				{ "last_line", di.LastLine },
				{ "last_column", di.LastColumn }
			}) }
		});
	}

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array({ resultInfo }) }
	});

	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result);

	return true;
}

bool ConsoleHandler::AutocompleteScriptHelper(boost::beast::http::request<boost::beast::http::string_body>& request,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	const Dictionary::Ptr& params, const String& command, const String& session, bool sandboxed)
{
	namespace http = boost::beast::http;

	Log(LogInformation, "Console")
		<< "Auto-completing expression: " << command;

	EnsureFrameCleanupTimer();

	ApiScriptFrame& lsf = l_ApiScriptFrames[session];
	lsf.Seen = Utility::GetTime();

	if (!lsf.Locals)
		lsf.Locals = new Dictionary();


	ScriptFrame frame(true);
	frame.Locals = lsf.Locals;
	frame.Self = lsf.Locals;
	frame.Sandboxed = sandboxed;

	Dictionary::Ptr result1 = new Dictionary({
		{ "code", 200 },
		{ "status", "Auto-completed successfully." },
		{ "suggestions", Array::FromVector(GetAutocompletionSuggestions(command, frame)) }
	});

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array({ result1 }) }
	});

	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result);

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

	if (value.IsObjectType<Namespace>()) {
		Namespace::Ptr ns = value;

		ObjectLock olock(ns);
		for (const Namespace::Pair& kv : ns) {
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
		for (const Namespace::Pair& kv : ScriptGlobal::GetGlobals()) {
			AddSuggestion(matches, word, kv.first);
		}
	}

	Namespace::Ptr systemNS = ScriptGlobal::Get("System");

	AddSuggestions(matches, word, "", false, systemNS);
	AddSuggestions(matches, word, "", true, systemNS->Get("Configuration"));
	AddSuggestions(matches, word, "", false, ScriptGlobal::Get("Types"));
	AddSuggestions(matches, word, "", false, ScriptGlobal::Get("Icinga"));

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
