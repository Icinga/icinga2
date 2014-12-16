/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "cli/replcommand.hpp"
#include "config/configcompiler.hpp"
#include "config/configcompilercontext.hpp"
#include "base/json.hpp"
#include "base/console.hpp"
#include "base/application.hpp"
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("repl", ReplCommand);

String ReplCommand::GetDescription(void) const
{
	return "Interprets Icinga script expressions.";
}

String ReplCommand::GetShortDescription(void) const
{
	return "Icinga REPL console";
}

bool ReplCommand::IsHidden(void) const
{
	return true;
}

ImpersonationLevel ReplCommand::GetImpersonationLevel(void) const
{
	return ImpersonateNone;
}

void ReplCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
}

/**
 * The entry point for the "repl" CLI command.
 *
 * @returns An exit status.
 */
int ReplCommand::Run(const po::variables_map& vm, const std::vector<std::string>& ap) const
{
	ScriptFrame frame;
	std::map<String, String> lines;
	int next_line = 1;

	std::cout << "Icinga (version: " << Application::GetVersion() << ")\n";

	while (std::cin.good()) {
		String fileName = "<" + Convert::ToString(next_line) + ">";
		next_line++;

		std::cout << ConsoleColorTag(Console_ForegroundCyan)
		    << fileName << ConsoleColorTag(Console_ForegroundRed) << " => "
		    << ConsoleColorTag(Console_Normal);

		std::string line;
		std::getline(std::cin, line);

		Expression *expr;

		try {
			ConfigCompilerContext::GetInstance()->Reset();

			lines[fileName] = line;

			expr = ConfigCompiler::CompileText(fileName, line);

			bool has_errors = false;

			BOOST_FOREACH(const ConfigCompilerMessage& message, ConfigCompilerContext::GetInstance()->GetMessages()) {
				if (message.Error)
					has_errors = true;

				std::cout << (message.Error ? "Error" : "Warning") << ": " << message.Text << "\n";
			}

			if (expr && !has_errors) {
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

			std::cout << di.Path << ": " << lines[di.Path] << "\n";
			std::cout << String(di.Path.GetLength() + 2, ' ');
			std::cout << String(di.FirstColumn, ' ') << String(di.LastColumn - di.FirstColumn + 1, '^') << "\n";

			std::cout << ex.what() << "\n";
		} catch (const std::exception& ex) {
			std::cout << "Error: " << DiagnosticInformation(ex) << "\n";
		}

		delete expr;
	}

	return 0;
}
