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
#include "base/unixsocket.hpp"
#include "base/utility.hpp"
#include "base/networkstream.hpp"
#include <iostream>


using namespace icinga;
namespace po = boost::program_options;

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

/**
 * The entry point for the "console" CLI command.
 *
 * @returns An exit status.
 */
int ConsoleCommand::Run(const po::variables_map& vm, const std::vector<std::string>& ap) const
{
	ScriptFrame frame;
	std::map<String, String> lines;
	int next_line = 1;

	String addr, session;

	if (vm.count("connect")) {
		addr = vm["connect"].as<std::string>();
		session = Utility::NewUniqueID();
	}

	std::cout << "Icinga (version: " << Application::GetVersion() << ")\n";

	while (std::cin.good()) {
		String fileName = "<" + Convert::ToString(next_line) + ">";
		next_line++;

		std::cout << ConsoleColorTag(Console_ForegroundCyan)
		    << fileName
		    << ConsoleColorTag(Console_ForegroundRed)
		    << " => "
		    << ConsoleColorTag(Console_Normal);

		std::string line;
		std::getline(std::cin, line);

		if (addr.IsEmpty()) {
			Expression *expr;

			try {
				lines[fileName] = line;

				expr = ConfigCompiler::CompileText(fileName, line);

				if (expr) {
					Value result = expr->Evaluate(frame);
					std::cout << ConsoleColorTag(Console_ForegroundCyan);
					if (!result.IsObject() || result.IsObjectType<Array>() || result.IsObjectType<Dictionary>())
						std::cout << JsonEncode(result);
					else
						std::cout << result;
					std::cout << ConsoleColorTag(Console_Normal) << "\n";
				}
			} catch (const ScriptError& ex) {
				DebugInfo di = ex.GetDebugInfo();

				if (lines.find(di.Path) != lines.end()) {
					std::cout << di.Path << ": " << lines[di.Path] << "\n";
					std::cout << String(di.Path.GetLength() + 2, ' ');
					std::cout << String(di.FirstColumn, ' ') << String(di.LastColumn - di.FirstColumn + 1, '^') << "\n";
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
				size_t rc = ns->Read(buf, sizeof(buf));
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
