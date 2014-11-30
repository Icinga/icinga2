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

#include "cli/consolecommand.hpp"
#include "config/configcompiler.hpp"
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

static ScriptFrame l_ScriptFrame;

REGISTER_CLICOMMAND("console", ConsoleCommand);

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
	;
}

#ifdef HAVE_EDITLINE
static void AddSuggestion(std::vector<String>& matches, const String& word, const String& suggestion)
{
	if (suggestion.Find(word) != 0)
		return;

	matches.push_back(suggestion);
}

static char *ConsoleCompleteHelper(const char *word, int state)
{
	static std::vector<String> matches;
	String aword = word;

	if (state == 0) {
		matches.clear();

		AddSuggestion(matches, word, "object");
		AddSuggestion(matches, word, "template");
		AddSuggestion(matches, word, "include");
		AddSuggestion(matches, word, "include_recursive");
		AddSuggestion(matches, word, "library");
		AddSuggestion(matches, word, "null");
		AddSuggestion(matches, word, "true");
		AddSuggestion(matches, word, "false");
		AddSuggestion(matches, word, "const");
		AddSuggestion(matches, word, "var");
		AddSuggestion(matches, word, "this");
		AddSuggestion(matches, word, "globals");
		AddSuggestion(matches, word, "locals");
		AddSuggestion(matches, word, "use");
		AddSuggestion(matches, word, "apply");
		AddSuggestion(matches, word, "to");
		AddSuggestion(matches, word, "where");
		AddSuggestion(matches, word, "import");
		AddSuggestion(matches, word, "assign");
		AddSuggestion(matches, word, "ignore");
		AddSuggestion(matches, word, "function");
		AddSuggestion(matches, word, "return");
		AddSuggestion(matches, word, "break");
		AddSuggestion(matches, word, "continue");
		AddSuggestion(matches, word, "for");
		AddSuggestion(matches, word, "if");
		AddSuggestion(matches, word, "else");
		AddSuggestion(matches, word, "while");

		{
			ObjectLock olock(l_ScriptFrame.Locals);
			BOOST_FOREACH(const Dictionary::Pair& kv, l_ScriptFrame.Locals) {
				AddSuggestion(matches, word, kv.first);
			}
		}

		{
			ObjectLock olock(ScriptGlobal::GetGlobals());
			BOOST_FOREACH(const Dictionary::Pair& kv, ScriptGlobal::GetGlobals()) {
				AddSuggestion(matches, word, kv.first);
			}
		}

		String::SizeType cperiod = aword.RFind(".");

		if (cperiod != -1) {
			String pword = aword.SubStr(0, cperiod);

			Value value;

			try {
				Expression *expr = ConfigCompiler::CompileText("temp", pword, false);

				if (expr)
					value = expr->Evaluate(l_ScriptFrame);

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

				Object::Ptr prototype = type->GetPrototype();
				Dictionary::Ptr dict = dynamic_pointer_cast<Dictionary>(prototype);

				if (dict) {
					ObjectLock olock(dict);
					BOOST_FOREACH(const Dictionary::Pair& kv, dict) {
						AddSuggestion(matches, word, pword + "." + kv.first);
					}
				}
			} catch (...) { /* Ignore the exception */ }
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
	std::map<String, String> lines;
	int next_line = 1;

#ifdef HAVE_EDITLINE
	rl_completion_entry_function = ConsoleCompleteHelper;
	rl_completion_append_character = '\0';
#endif /* HAVE_EDITLINE */

	String addr, session;

	if (vm.count("connect")) {
		addr = vm["connect"].as<std::string>();
		session = Utility::NewUniqueID();
	}

	std::cout << "Icinga (version: " << Application::GetVersion() << ")\n";

	while (std::cin.good()) {
		String fileName = "<" + Convert::ToString(next_line) + ">";
		next_line++;

		bool continuation = false;
		std::string command;

incomplete:
#ifdef HAVE_EDITLINE
		ConsoleType console_type = Console_VT100;
		std::ostringstream promptbuf;
		std::ostream& os = promptbuf;
#else /* HAVE_EDITLINE */
		ConsoleType console_type = Console_Autodetect;
		std::ostream& os = std::cout;
#endif /* HAVE_EDITLINE */

		os << ConsoleColorTag(Console_ForegroundCyan, console_type)
		   << fileName
		   << ConsoleColorTag(Console_ForegroundRed, console_type);

		if (!continuation)
			os << " => ";
		else
			os << " .. ";

		os << ConsoleColorTag(Console_Normal, console_type);

#ifdef HAVE_EDITLINE
		String prompt = promptbuf.str();

		char *cline;
		cline = readline(prompt.CStr());

		if (!cline)
			break;

		add_history(cline);

		std::string line = cline;

		free(cline);
#else /* HAVE_EDITLINE */
		std::string line;
		std::getline(std::cin, line);
#endif /* HAVE_EDITLINE */

		if (!command.empty())
			command += "\n";

		command += line;

		if (addr.IsEmpty()) {
			Expression *expr = NULL;

			try {
				lines[fileName] = command;

				expr = ConfigCompiler::CompileText(fileName, command, false);

				if (expr) {
					Value result = expr->Evaluate(l_ScriptFrame);
					std::cout << ConsoleColorTag(Console_ForegroundCyan);
					if (!result.IsObject() || result.IsObjectType<Array>() || result.IsObjectType<Dictionary>())
						std::cout << JsonEncode(result);
					else
						std::cout << result;
					std::cout << ConsoleColorTag(Console_Normal) << "\n";
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
					ShowCodeFragment(std::cout, di);
				}

				std::cout << ex.what() << "\n";
			} catch (const std::exception& ex) {
				std::cout << "Error: " << DiagnosticInformation(ex) << "\n";
			}

			delete expr;
		} else {
			Socket::Ptr socket;

#ifndef _WIN32
			if (addr.FindFirstOf("/") != String::NPos) {
				UnixSocket::Ptr usocket = new UnixSocket();
				usocket->Connect(addr);
				socket = usocket;
			} else {
#endif /* _WIN32 */
				Log(LogCritical, "ConsoleCommand", "Sorry, TCP sockets aren't supported yet.");
				return 1;
#ifndef _WIN32
			}
#endif /* _WIN32 */

			String query = "SCRIPT " + session + "\n" + line + "\n\n";

			NetworkStream::Ptr ns = new NetworkStream(socket);
			ns->Write(query.CStr(), query.GetLength());

			String result;
			char buf[1024];

			while (!ns->IsEof()) {
				size_t rc = ns->Read(buf, sizeof(buf), true);
				result += String(buf, buf + rc);
			}

			if (result.GetLength() < 16) {
				Log(LogCritical, "ConsoleCommand", "Received invalid response from Livestatus.");
				continue;
			}

			std::cout << result.SubStr(16) << "\n";
		}
	}

	return 0;
}
