/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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
#ifdef HAVE_EDITLINE
#include "cli/editline.hpp"
#endif /* HAVE_EDITLINE */

using namespace icinga;
namespace po = boost::program_options;

static ScriptFrame *l_ScriptFrame;
static ApiClient::Ptr l_ApiClient;
static String l_Session;

REGISTER_CLICOMMAND("console", ConsoleCommand);

INITIALIZE_ONCE(&ConsoleCommand::StaticInitialize);

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
	          << "To leave the debugger and continue the program use \"$continue\".\n";

#ifdef HAVE_EDITLINE
	rl_completion_entry_function = ConsoleCommand::ConsoleCompleteHelper;
	rl_completion_append_character = '\0';
#endif /* HAVE_EDITLINE */

	ConsoleCommand::RunScriptConsole(frame);
}

void ConsoleCommand::StaticInitialize(void)
{
	Expression::OnBreakpoint.connect(&ConsoleCommand::BreakpointHandler);
}

String ConsoleCommand::GetDescription(void) const
{
	return "Interprets Icinga script expressions.";
}

String ConsoleCommand::GetShortDescription(void) const
{
	return "Icinga console";
}

ImpersonationLevel ConsoleCommand::GetImpersonationLevel(void) const
{
	return ImpersonateNone;
}

void ConsoleCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("connect,c", po::value<std::string>(), "connect to an Icinga 2 instance")
		("eval,e", po::value<std::string>(), "evaluate expression and terminate")
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
			    boost::bind(&ConsoleCommand::AutocompleteScriptCompletionHandler,
			    boost::ref(mutex), boost::ref(cv), boost::ref(ready),
			    _1, _2,
			    boost::ref(suggestions)));

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

	if (state >= matches.size())
		return NULL;

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
	ScriptFrame scriptFrame;

	session = Utility::NewUniqueID();

	if (vm.count("sandbox"))
		scriptFrame.Sandboxed = true;

	scriptFrame.Self = scriptFrame.Locals;

	if (!vm.count("eval"))
		std::cout << "Icinga 2 (version: " << Application::GetAppVersion() << ")\n";

	const char *addrEnv = getenv("ICINGA2_API_URL");
	if (addrEnv)
		addr = addrEnv;

	if (vm.count("connect"))
		addr = vm["connect"].as<std::string>();

	String command;

	if (vm.count("eval"))
		command = vm["eval"].as<std::string>();

	return RunScriptConsole(scriptFrame, addr, session, command);;
}

int ConsoleCommand::RunScriptConsole(ScriptFrame& scriptFrame, const String& addr, const String& session, const String& commandOnce)
{
	std::map<String, String> lines;
	int next_line = 1;

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
		String fileName = "<" + Convert::ToString(next_line) + ">";
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

			add_history(cline);

			line = cline;

			free(cline);
#else /* HAVE_EDITLINE */
			std::getline(std::cin, line);
#endif /* HAVE_EDITLINE */
		} else
			line = commandOnce;

		if (!line.empty() && line[0] == '$') {
			if (line == "$continue")
				break;

			std::cout << "Unknown debugger command: " << line << "\n";
			continue;
		}

		if (!command.empty())
			command += "\n";

		command += line;

		Expression *expr = NULL;

		try {
			lines[fileName] = command;

			Value result;

			if (!l_ApiClient) {
				expr = ConfigCompiler::CompileText(fileName, command);
				result = Serialize(expr->Evaluate(scriptFrame), 0);
			} else {
				boost::mutex mutex;
				boost::condition_variable cv;
				bool ready = false;
				boost::exception_ptr eptr;

				l_ApiClient->ExecuteScript(l_Session, command, scriptFrame.Sandboxed,
				    boost::bind(&ConsoleCommand::ExecuteScriptCompletionHandler,
				    boost::ref(mutex), boost::ref(cv), boost::ref(ready),
				    _1, _2,
				    boost::ref(result), boost::ref(eptr)));

				{
					boost::mutex::scoped_lock lock(mutex);
					while (!ready)
						cv.wait(lock);
				}

				if (eptr)
					boost::rethrow_exception(eptr);
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
			if (ex.IsIncompleteExpression()) {
				continuation = true;
				goto incomplete;
			}

			DebugInfo di = ex.GetDebugInfo();

			if (lines.find(di.Path) != lines.end()) {
				String text = lines[di.Path];

				std::vector<String> ulines;
				boost::algorithm::split(ulines, text, boost::is_any_of("\n"));

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

		delete expr;
	}

	return EXIT_SUCCESS;
}

void ConsoleCommand::ExecuteScriptCompletionHandler(boost::mutex& mutex, boost::condition_variable& cv,
    bool& ready, boost::exception_ptr eptr, const Value& result, Value& resultOut, boost::exception_ptr& eptrOut)
{
	if (eptr) {
		try {
			boost::rethrow_exception(eptr);
		} catch (const ScriptError& ex) {
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
    bool& ready, boost::exception_ptr eptr, const Array::Ptr& result, Array::Ptr& resultOut)
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
