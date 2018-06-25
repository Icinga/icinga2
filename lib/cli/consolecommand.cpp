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
#include "base/exception.hpp"
#include <iostream>
#include <fstream>
#ifdef HAVE_EDITLINE
#include "cli/editline.hpp"
#endif /* HAVE_EDITLINE */

#define CONSOLE_TIMEOUT_SECONDS 15

using namespace icinga;
namespace po = boost::program_options;

static ScriptFrame *l_ScriptFrame;
static ApiClient::Ptr l_ApiClient;
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

#ifdef HAVE_EDITLINE
char *ConsoleCommand::ConsoleCompleteHelper(const char *word, int state)
{
	static std::vector<String> matches;

	if (state == 0) {
		if (!l_ApiClient)
			matches = ConsoleHandler::GetAutocompletionSuggestions(word, *l_ScriptFrame);
		else {
			boost::mutex mutex;
			boost::condition_variable cv;
			bool ready = false;
			Array::Ptr suggestions;

			l_ApiClient->AutocompleteScript(l_Session, word, l_ScriptFrame->Sandboxed,
				std::bind(&ConsoleCommand::AutocompleteScriptCompletionHandler,
				std::ref(mutex), std::ref(cv), std::ref(ready),
				_1, _2,
				std::ref(suggestions)));

			{
				boost::mutex::scoped_lock lock(mutex);
				while (!ready)
					cv.wait(lock);
			}

			matches.clear();

			ObjectLock olock(suggestions);
			std::copy(suggestions->Begin(), suggestions->End(), std::back_inserter(matches));
		}
	}

	if (state >= static_cast<int>(matches.size()))
		return nullptr;

	return strdup(matches[state].CStr());
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


	const char *addrEnv = getenv("ICINGA2_API_URL");
	if (addrEnv)
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

int ConsoleCommand::RunScriptConsole(ScriptFrame& scriptFrame, const String& addr, const String& session, const String& commandOnce, const String& commandOnceFileName, bool syntaxOnly)
{
	std::map<String, String> lines;
	int next_line = 1;

#ifdef HAVE_EDITLINE
	char *homeEnv = getenv("HOME");

	String historyPath;
	std::fstream historyfp;

	if (homeEnv) {
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

	if (!addr.IsEmpty()) {
		Url::Ptr url;

		try {
			url = new Url(addr);
		} catch (const std::exception& ex) {
			Log(LogCritical, "ConsoleCommand", ex.what());
			return EXIT_FAILURE;
		}

		const char *usernameEnv = getenv("ICINGA2_API_USERNAME");
		const char *passwordEnv = getenv("ICINGA2_API_PASSWORD");

		if (usernameEnv)
			url->SetUsername(usernameEnv);
		if (passwordEnv)
			url->SetPassword(passwordEnv);

		if (url->GetPort().IsEmpty())
			url->SetPort("5665");

		l_ApiClient = new ApiClient(url->GetHost(), url->GetPort(), url->GetUsername(), url->GetPassword());
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

			if (!l_ApiClient) {
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
				boost::mutex mutex;
				boost::condition_variable cv;
				bool ready = false;
				boost::exception_ptr eptr;

				boost::system_time const timeout = boost::get_system_time()
					+ boost::posix_time::seconds(CONSOLE_TIMEOUT_SECONDS);
				l_ApiClient->ExecuteScript(l_Session, command, scriptFrame.Sandboxed,
					std::bind(&ConsoleCommand::ExecuteScriptCompletionHandler,
					std::ref(mutex), std::ref(cv), std::ref(ready),
					_1, _2,
					std::ref(result), std::ref(eptr)));

				{
					boost::mutex::scoped_lock lock(mutex);
					while (!ready && timeout > boost::get_system_time())
						cv.timed_wait(lock, timeout);
				}

				if (eptr)
					boost::rethrow_exception(eptr);

				if (timeout < boost::get_system_time()) {
					std::cout << ConsoleColorTag(Console_ForegroundRed) << "ERROR: " << ConsoleColorTag(Console_Normal)
						<< "Remote console timeout (" << CONSOLE_TIMEOUT_SECONDS << " seconds)\n";
					break;
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

				for (int i = 1; i <= ulines.size(); i++) {
					int start, len;

					if (i == di.FirstLine)
						start = di.FirstColumn;
					else
						start = 0;

					if (i == di.LastLine)
						len = di.LastColumn - di.FirstColumn + 1;
					else
						len = ulines[i - 1].GetLength();

					int offset;

					if (di.Path != fileName) {
						std::cout << di.Path << ": " << ulines[i - 1] << "\n";
						offset = 2;
					} else
						offset = 4;

					if (i >= di.FirstLine && i <= di.LastLine) {
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

void ConsoleCommand::ExecuteScriptCompletionHandler(boost::mutex& mutex, boost::condition_variable& cv,
	bool& ready, const boost::exception_ptr& eptr, const Value& result, Value& resultOut, boost::exception_ptr& eptrOut)
{
	if (eptr) {
		try {
			boost::rethrow_exception(eptr);
		} catch (const ScriptError&) {
			eptrOut = boost::current_exception();
		} catch (const std::exception& ex) {
			Log(LogCritical, "ConsoleCommand")
				<< "HTTP query failed: " << ex.what();
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

void ConsoleCommand::AutocompleteScriptCompletionHandler(boost::mutex& mutex, boost::condition_variable& cv,
	bool& ready, const boost::exception_ptr& eptr, const Array::Ptr& result, Array::Ptr& resultOut)
{
	if (eptr) {
		try {
			boost::rethrow_exception(eptr);
		} catch (const std::exception& ex) {
			Log(LogCritical, "ConsoleCommand")
				<< "HTTP query failed: " << ex.what();
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
