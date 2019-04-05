/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/consolecommand.hpp"
#include "config/configcompiler.hpp"
#include "remote/apiclient.hpp"
#include "remote/consolehandler.hpp"
#include "remote/url.hpp"
#include "base/configwriter.hpp"
#include "base/serializer.hpp"
#include "base/json.hpp"
#include "base/console.hpp"
#include "base/application.hpp"
#include "base/objectlock.hpp"
#include "base/unixsocket.hpp"
#include "base/utility.hpp"
#include "base/networkstream.hpp"
#include "base/defer.hpp"
#include "base/io-engine.hpp"
#include "base/stream.hpp"
#include "base/tcpsocket.hpp" /* include global icinga::Connect */
#include <base/base64.hpp>
#include "base/exception.hpp"
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/write.hpp>
#include <iostream>
#include <fstream>


#ifdef HAVE_EDITLINE
#include "cli/editline.hpp"
#endif /* HAVE_EDITLINE */

using namespace icinga;
namespace po = boost::program_options;

static ScriptFrame *l_ScriptFrame;
static std::shared_ptr<AsioTlsStream> l_TlsStream;
static String l_Session;

REGISTER_CLICOMMAND("console", ConsoleCommand);

INITIALIZE_ONCE(&ConsoleCommand::StaticInitialize);

extern "C" void dbg_spawn_console()
{
	ScriptFrame frame(true);
	ConsoleCommand::RunScriptConsole(frame);
}

extern "C" void dbg_inspect_value(const Value& value)
{
	ConfigWriter::EmitValue(std::cout, 1, Serialize(value, 0));
	std::cout << std::endl;
}

extern "C" void dbg_inspect_object(Object *obj)
{
	Object::Ptr objr = obj;
	dbg_inspect_value(objr);
}

extern "C" void dbg_eval(const char *text)
{
	std::unique_ptr<Expression> expr;

	try {
		ScriptFrame frame(true);
		expr = ConfigCompiler::CompileText("<dbg>", text);
		Value result = Serialize(expr->Evaluate(frame), 0);
		dbg_inspect_value(result);
	} catch (const std::exception& ex) {
		std::cout << "Error: " << DiagnosticInformation(ex) << "\n";
	}
}

extern "C" void dbg_eval_with_value(const Value& value, const char *text)
{
	std::unique_ptr<Expression> expr;

	try {
		ScriptFrame frame(true);
		frame.Locals = new Dictionary({
			{ "arg", value }
		});
		expr = ConfigCompiler::CompileText("<dbg>", text);
		Value result = Serialize(expr->Evaluate(frame), 0);
		dbg_inspect_value(result);
	} catch (const std::exception& ex) {
		std::cout << "Error: " << DiagnosticInformation(ex) << "\n";
	}
}

extern "C" void dbg_eval_with_object(Object *object, const char *text)
{
	std::unique_ptr<Expression> expr;

	try {
		ScriptFrame frame(true);
		frame.Locals = new Dictionary({
			{ "arg", object }
		});
		expr = ConfigCompiler::CompileText("<dbg>", text);
		Value result = Serialize(expr->Evaluate(frame), 0);
		dbg_inspect_value(result);
	} catch (const std::exception& ex) {
		std::cout << "Error: " << DiagnosticInformation(ex) << "\n";
	}
}

void ConsoleCommand::BreakpointHandler(ScriptFrame& frame, ScriptError *ex, const DebugInfo& di)
{
	static boost::mutex mutex;
	boost::mutex::scoped_lock lock(mutex);

	if (!Application::GetScriptDebuggerEnabled())
		return;

	if (ex && ex->IsHandledByDebugger())
		return;

	std::cout << "Breakpoint encountered.\n";

	if (ex) {
		std::cout << "Exception: " << DiagnosticInformation(*ex) << "\n";
		ex->SetHandledByDebugger(true);
	} else
		ShowCodeLocation(std::cout, di);

	std::cout << "You can inspect expressions (such as variables) by entering them at the prompt.\n"
		<< "To leave the debugger and continue the program use \"$continue\".\n"
		<< "For further commands see \"$help\".\n";

#ifdef HAVE_EDITLINE
	rl_completion_entry_function = ConsoleCommand::ConsoleCompleteHelper;
	rl_completion_append_character = '\0';
#endif /* HAVE_EDITLINE */

	ConsoleCommand::RunScriptConsole(frame);
}

void ConsoleCommand::StaticInitialize()
{
	Expression::OnBreakpoint.connect(&ConsoleCommand::BreakpointHandler);
}

String ConsoleCommand::GetDescription() const
{
	return "Interprets Icinga script expressions.";
}

String ConsoleCommand::GetShortDescription() const
{
	return "Icinga console";
}

ImpersonationLevel ConsoleCommand::GetImpersonationLevel() const
{
	return ImpersonateNone;
}

void ConsoleCommand::InitParameters(boost::program_options::options_description& visibleDesc,
	boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("connect,c", po::value<std::string>(), "connect to an Icinga 2 instance")
		("eval,e", po::value<std::string>(), "evaluate expression and terminate")
		("file,r", po::value<std::string>(), "evaluate a file and terminate")
		("syntax-only", "only validate syntax (requires --eval or --file)")
		("sandbox", "enable sandbox mode")
	;
}

/**
 * Connects to host:port and performs a TLS shandshake
 *
 * @param host To connect to.
 * @param port To connect to.
 *
 * @returns AsioTlsStream pointer for future HTTP connections.
 */
std::shared_ptr<AsioTlsStream> ConsoleCommand::Connect(const String& host, const String& port)
{
	std::shared_ptr<boost::asio::ssl::context> sslContext;

	try {
		sslContext = MakeAsioSslContext(Empty, Empty, Empty); //TODO: Add support for cert, key, ca parameters
	} catch(const std::exception& ex) {
		Log(LogCritical, "DebugConsole")
				<< "Cannot make SSL context: " << ex.what();
		throw;
	}

	std::shared_ptr<AsioTlsStream> stream = std::make_shared<AsioTlsStream>(IoEngine::Get().GetIoService(), *sslContext, host);

	try {
		icinga::Connect(stream->lowest_layer(), host, port);
	} catch (const std::exception& ex) {
		Log(LogWarning, "DebugConsole")
			<< "Cannot connect to REST API on host '" << host << "' port '" << port << "': " << ex.what();
		throw;
	}

	auto& tlsStream (stream->next_layer());

	try {
		tlsStream.handshake(tlsStream.client);
	} catch (const std::exception& ex) {
		Log(LogWarning, "DebugConsole")
			<< "TLS handshake with host '" << host << "' failed: " << ex.what();
		throw;
	}

	return std::move(stream);
}


#ifdef HAVE_EDITLINE
char *ConsoleCommand::ConsoleCompleteHelper(const char *word, int state)
{
	static std::vector<String> matches;
/*
	if (state == 0) {
		if (!l_TlsStream)
			matches = ConsoleHandler::GetAutocompletionSuggestions(word, *l_ScriptFrame);
		else {
			Array::Ptr suggestions;

			l_ApiClient->AutocompleteScript(l_Session, word, l_ScriptFrame->Sandboxed,
				std::bind(&ConsoleCommand::AutocompleteScriptCompletionHandler,
				std::ref(mutex), std::ref(cv), std::ref(ready),
				_1, _2,
				std::ref(suggestions)));


			matches.clear();

			ObjectLock olock(suggestions);
			std::copy(suggestions->Begin(), suggestions->End(), std::back_inserter(matches));
		}
	}

	if (state >= static_cast<int>(matches.size()))
		return nullptr;

	return strdup(matches[state].CStr());
 */
}
#endif /* HAVE_EDITLINE */

/**
 * The entry point for the "console" CLI command.
 *
 * @returns An exit status.
 */
int ConsoleCommand::Run(const po::variables_map& vm, const std::vector<std::string>& ap) const
{
#ifdef HAVE_EDITLINE
	rl_completion_entry_function = ConsoleCommand::ConsoleCompleteHelper;
	rl_completion_append_character = '\0';
#endif /* HAVE_EDITLINE */

	String addr, session;
	ScriptFrame scriptFrame(true);

	session = Utility::NewUniqueID();

	if (vm.count("sandbox"))
		scriptFrame.Sandboxed = true;

	scriptFrame.Self = scriptFrame.Locals;

	if (!vm.count("eval") && !vm.count("file"))
		std::cout << "Icinga 2 (version: " << Application::GetAppVersion() << ")\n"
			<< "Type $help to view available commands.\n";


	String addrEnv = Utility::GetFromEnvironment("ICINGA2_API_URL");
	if (!addrEnv.IsEmpty())
		addr = addrEnv;

	if (vm.count("connect"))
		addr = vm["connect"].as<std::string>();

	String command;
	bool syntaxOnly = false;

	if (vm.count("syntax-only")) {
		if (vm.count("eval") || vm.count("file"))
			syntaxOnly = true;
		else {
			std::cerr << "The option --syntax-only can only be used in combination with --eval or --file." << std::endl;
			return EXIT_FAILURE;
		}
	}

	String commandFileName;

	if (vm.count("eval"))
		command = vm["eval"].as<std::string>();
	else if (vm.count("file")) {
		commandFileName = vm["file"].as<std::string>();

		try {
			std::ifstream fp(commandFileName.CStr());
			fp.exceptions(std::ifstream::failbit | std::ifstream::badbit);
			command = String(std::istreambuf_iterator<char>(fp), std::istreambuf_iterator<char>());
		} catch (const std::exception&) {
			std::cerr << "Could not read file '" << commandFileName << "'." << std::endl;
			return EXIT_FAILURE;
		}
	}

	return RunScriptConsole(scriptFrame, addr, session, command, commandFileName, syntaxOnly);
}

int ConsoleCommand::RunScriptConsole(ScriptFrame& scriptFrame, const String& connectAddr, const String& session,
        const String& commandOnce, const String& commandOnceFileName, bool syntaxOnly)
{
	std::map<String, String> lines;
	int next_line = 1;

#ifdef HAVE_EDITLINE
	String homeEnv = Utility::GetFromEnvironment("HOME");

	String historyPath;
	std::fstream historyfp;

	if (!homeEnv.IsEmpty()) {
		historyPath = String(homeEnv) + "/.icinga2_history";

		historyfp.open(historyPath.CStr(), std::fstream::in);

		String line;
		while (std::getline(historyfp, line.GetData()))
			add_history(line.CStr());

		historyfp.close();
	}
#endif /* HAVE_EDITLINE */

	l_ScriptFrame = &scriptFrame;
	l_Session = session;

	/* User passed --connect and wants to run the expression via REST API.
	 * Evaluate this now before any user input happens.
	 */
	Url::Ptr url;
	std::shared_ptr<AsioTlsStream> tlsStream;

	if (!connectAddr.IsEmpty()) {
		try {
			url = new Url(connectAddr);
		} catch (const std::exception& ex) {
			Log(LogCritical, "ConsoleCommand", ex.what());
			return EXIT_FAILURE;
		}

		String usernameEnv = Utility::GetFromEnvironment("ICINGA2_API_USERNAME");
		String passwordEnv = Utility::GetFromEnvironment("ICINGA2_API_PASSWORD");

		if (!usernameEnv.IsEmpty())
			url->SetUsername(usernameEnv);
		if (!passwordEnv.IsEmpty())
			url->SetPassword(passwordEnv);

		if (url->GetPort().IsEmpty())
			url->SetPort("5665");

		try {
			l_TlsStream = ConsoleCommand::Connect(url->GetHost(), url->GetPort());
		} catch (const std::exception& ex) {
			return EXIT_FAILURE;
		}
	}

	while (std::cin.good()) {
		String fileName;

		if (commandOnceFileName.IsEmpty())
			fileName = "<" + Convert::ToString(next_line) + ">";
		else
			fileName = commandOnceFileName;

		next_line++;

		bool continuation = false;
		std::string command;

incomplete:
		std::string line;

		if (commandOnce.IsEmpty()) {
#ifdef HAVE_EDITLINE
			std::ostringstream promptbuf;
			std::ostream& os = promptbuf;
#else /* HAVE_EDITLINE */
			std::ostream& os = std::cout;
#endif /* HAVE_EDITLINE */

			os << fileName;

			if (!continuation)
				os << " => ";
			else
				os << " .. ";

#ifdef HAVE_EDITLINE
			String prompt = promptbuf.str();

			char *cline;
			cline = readline(prompt.CStr());

			if (!cline)
				break;

			if (commandOnce.IsEmpty() && cline[0] != '\0') {
				add_history(cline);

				if (!historyPath.IsEmpty()) {
					historyfp.open(historyPath.CStr(), std::fstream::out | std::fstream::app);
					historyfp << cline << "\n";
					historyfp.close();
				}
			}

			line = cline;

			free(cline);
#else /* HAVE_EDITLINE */
			std::getline(std::cin, line);
#endif /* HAVE_EDITLINE */
		} else
			line = commandOnce;

		if (!line.empty() && line[0] == '$') {
			if (line == "$continue" || line == "$quit" || line == "$exit")
				break;
			else if (line == "$help")
				std::cout << "Welcome to the Icinga 2 debug console.\n"
					"Usable commands:\n"
					"  $continue      Continue running Icinga 2 (script debugger).\n"
					"  $quit, $exit   Stop debugging and quit the console.\n"
					"  $help          Print this help.\n\n"
					"For more information on how to use this console, please consult the documentation at https://icinga.com/docs\n";
			else
				std::cout << "Unknown debugger command: " << line << "\n";

			continue;
		}

		if (!command.empty())
			command += "\n";

		command += line;

		std::unique_ptr<Expression> expr;

		try {
			lines[fileName] = command;

			Value result;

			/* Local debug console. */
			if (connectAddr.IsEmpty()) {
				expr = ConfigCompiler::CompileText(fileName, command);

				/* This relies on the fact that - for syntax errors - CompileText()
				 * returns an AST where the top-level expression is a 'throw'. */
				if (!syntaxOnly || dynamic_cast<ThrowExpression *>(expr.get())) {
					if (syntaxOnly)
						std::cerr << "    => " << command << std::endl;
					result = Serialize(expr->Evaluate(scriptFrame), 0);
				} else
					result = true;
			} else {
				/* Remote debug console. */
				try {
					l_TlsStream = ConsoleCommand::Connect(url->GetHost(), url->GetPort());
				} catch (const std::exception& ex) {
					return EXIT_FAILURE;
				}

				try {
					result = ExecuteScript(l_Session, l_TlsStream, url, command, scriptFrame.Sandboxed);
				} catch (const ScriptError&) {
					/* Re-throw the exception for the outside try-catch block. */
					boost::rethrow_exception(boost::current_exception());
				} catch (const std::exception& ex) {
					Log(LogCritical, "ConsoleCommand")
						<< "HTTP query failed: " << ex.what();

#ifdef HAVE_EDITLINE
					/* Ensures that the terminal state is resetted */
					rl_deprep_terminal();
#endif /* HAVE_EDITLINE */

					return EXIT_FAILURE;
				}
			}

			if (commandOnce.IsEmpty()) {
				std::cout << ConsoleColorTag(Console_ForegroundCyan);
				ConfigWriter::EmitValue(std::cout, 1, result);
				std::cout << ConsoleColorTag(Console_Normal) << "\n";
			} else {
				std::cout << JsonEncode(result) << "\n";
				break;
			}
		} catch (const ScriptError& ex) {
			if (ex.IsIncompleteExpression() && commandOnce.IsEmpty()) {
				continuation = true;
				goto incomplete;
			}

			DebugInfo di = ex.GetDebugInfo();

			if (commandOnceFileName.IsEmpty() && lines.find(di.Path) != lines.end()) {
				String text = lines[di.Path];

				std::vector<String> ulines = text.Split("\n");

				for (decltype(ulines.size()) i = 1; i <= ulines.size(); i++) {
					int start, len;

					if (i == (decltype(i))di.FirstLine)
						start = di.FirstColumn;
					else
						start = 0;

					if (i == (decltype(i))di.LastLine)
						len = di.LastColumn - di.FirstColumn + 1;
					else
						len = ulines[i - 1].GetLength();

					int offset;

					if (di.Path != fileName) {
						std::cout << di.Path << ": " << ulines[i - 1] << "\n";
						offset = 2;
					} else
						offset = 4;

					if (i >= (decltype(i))di.FirstLine && i <= (decltype(i))di.LastLine) {
						std::cout << String(di.Path.GetLength() + offset, ' ');
						std::cout << String(start, ' ') << String(len, '^') << "\n";
					}
				}
			} else {
				ShowCodeLocation(std::cout, di);
			}

			std::cout << ex.what() << "\n";

			if (!commandOnce.IsEmpty())
				return EXIT_FAILURE;
		} catch (const std::exception& ex) {
			std::cout << "Error: " << DiagnosticInformation(ex) << "\n";

			if (!commandOnce.IsEmpty())
				return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}

/**
 * Sends the request via REST API and returns the parsed response.
 *
 * @param tlsStream Caller must prepare TLS stream/handshake.
 * @param url Fully prepared Url object.
 * @return A dictionary decoded from JSON.
 */
Dictionary::Ptr ConsoleCommand::SendRequest(const std::shared_ptr<AsioTlsStream>& tlsStream, const Url::Ptr& url)
{
	namespace beast = boost::beast;
	namespace http = beast::http;

	http::request<http::string_body> request(http::verb::post, std::string(url->Format(false)), 10);

	request.set(http::field::user_agent, "Icinga/DebugConsole/" + Application::GetAppVersion());
	request.set(http::field::host, url->GetHost() + ":" + url->GetPort());

	request.set(http::field::accept, "application/json");
	request.set(http::field::authorization, "Basic " + Base64::Encode(url->GetUsername() + ":" + url->GetPassword()));

	try {
		http::write(*tlsStream, request);
		tlsStream->flush();
	} catch (const std::exception &ex) {
		Log(LogWarning, "DebugConsole")
				<< "Cannot write HTTP request to REST API at URL '" << url->Format(true) << "': " << ex.what();
		throw;
	}

	http::parser<false, http::string_body> parser;
	beast::flat_buffer buf;

	try {
		http::read(*tlsStream, buf, parser);
	} catch (const std::exception &ex) {
		Log(LogWarning, "DebugConsole")
				<< "Failed to parse HTTP response from REST API at URL '" << url->Format(true) << "': " << ex.what();
		throw;
	}

	auto &response(parser.get());

	/* Handle HTTP errors first. */
	if (response.result() != http::status::ok) {
		std::string message = "HTTP request failed; Code: " + Convert::ToString(response.result())
				+ "; Body: " + response.body();
		BOOST_THROW_EXCEPTION(ScriptError(message));
	}

	Dictionary::Ptr jsonResponse;
	auto &body(response.body());

	try {
		jsonResponse = JsonDecode(body);
	} catch (...) {
		std::string message = "Cannot parse JSON response body: " + response.body();
		BOOST_THROW_EXCEPTION(ScriptError(message));
	}

	return jsonResponse;
}

/**
 * Executes the DSL script via HTTP and returns HTTP and user errors.
 *
 * @param session Local session handler.
 * @param tlsStream AsioTlsStream pointer, must be constructed by the caller.
 * @param url Url object, must be constructed by the caller.
 * @param command The DSL string.
 * @param sandboxed Whether to run this sandboxed.
 * @return Result value, also contains user errors.
 */
Value ConsoleCommand::ExecuteScript(const String& session, const std::shared_ptr<AsioTlsStream>& tlsStream,
		const Url::Ptr& url, const String& command, bool sandboxed)
{
	/* Extend the url parameters for the request. */
	url->SetPath({"v1", "console", "execute-script"});

	url->SetQuery({
		{"session",   session},
		{"command",   command},
		{"sandboxed", sandboxed ? "1" : "0"}
	});

	Dictionary::Ptr jsonResponse = SendRequest(tlsStream, url);

	/* Extract the result, and handle user input errors too. */
	Array::Ptr results = jsonResponse->Get("results");
	Value result;

	if (results && results->GetLength() > 0) {
		Dictionary::Ptr resultInfo = results->Get(0);

		if (resultInfo->Get("code") >= 200 && resultInfo->Get("code") <= 299) {
			result = resultInfo->Get("result");
		} else {
			String errorMessage = resultInfo->Get("status");

			DebugInfo di;
			Dictionary::Ptr debugInfo = resultInfo->Get("debug_info");

			if (debugInfo) {
				di.Path = debugInfo->Get("path");
				di.FirstLine = debugInfo->Get("first_line");
				di.FirstColumn = debugInfo->Get("first_column");
				di.LastLine = debugInfo->Get("last_line");
				di.LastColumn = debugInfo->Get("last_column");
			}

			bool incompleteExpression = resultInfo->Get("incomplete_expression");
			BOOST_THROW_EXCEPTION(ScriptError(errorMessage, di, incompleteExpression));
		}
	}

	return result;
}

void ConsoleCommand::AutocompleteScriptCompletionHandler(boost::mutex& mutex, boost::condition_variable& cv,
	bool& ready, const boost::exception_ptr& eptr, const Array::Ptr& result, Array::Ptr& resultOut)
{
	if (eptr) {
		try {
			boost::rethrow_exception(eptr);
		} catch (const std::exception& ex) {
			Log(LogCritical, "ConsoleCommand")
				<< "HTTP query failed: " << ex.what();

#ifdef HAVE_EDITLINE
			/* Ensures that the terminal state is resetted */
			rl_deprep_terminal();
#endif /* HAVE_EDITLINE */

			Application::Exit(EXIT_FAILURE);
		}
	}

	resultOut = result;

	{
		boost::mutex::scoped_lock lock(mutex);
		ready = true;
		cv.notify_all();
	}
}
