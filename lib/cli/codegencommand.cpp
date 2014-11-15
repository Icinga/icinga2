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

#include "cli/codegencommand.hpp"
#include "config/expression.hpp"
#include "config/configcompiler.hpp"
#include "config/configcompilercontext.hpp"
#include "base/logger.hpp"

using namespace icinga;

REGISTER_CLICOMMAND("codegen", CodeGenCommand);

String CodeGenCommand::GetDescription(void) const
{
	return "Generates native code for an Icinga 2 config file.";
}

String CodeGenCommand::GetShortDescription(void) const
{
	return "compiles an Icinga 2 config file";
}

void CodeGenCommand::InitParameters(boost::program_options::options_description& visibleDesc,
	boost::program_options::options_description& hiddenDesc) const
{
}

bool CodeGenCommand::IsHidden(void) const
{
	return true;
}

/**
 * The entry point for the "codegen" CLI command.
 *
 * @returns An exit status.
 */
int CodeGenCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	Logger::SetConsoleLogSeverity(LogWarning);

	Expression *expr = ConfigCompiler::CompileStream("<stdin>", &std::cin);

	int errors = 0;

	BOOST_FOREACH(const ConfigCompilerMessage& message, ConfigCompilerContext::GetInstance()->GetMessages()) {
		String logmsg = String("Config ") + (message.Error ? "error" : "warning") + ": " + message.Text;

		if (message.Error) {
			Log(LogCritical, "config", logmsg);
			errors++;
		} else {
			Log(LogWarning, "config", logmsg);
		}
	}

	if (errors > 0)
		return 1;

	std::cout << "#include \"config/expression.hpp\"" << "\n"
		  << "#include \"config/vmops.hpp\"" << "\n"
		  << "#include \"base/json.hpp\"" << "\n"
		  << "#include \"base/initialize.hpp\"" << "\n"
		  << "#include <boost/smart_ptr/make_shared.hpp>" << "\n"
		  << "\n"
		  << "using namespace icinga;" << "\n"
		  << "\n";

	std::map<String, String> definitions;

	String name = CodeGenExpression(definitions, expr);

	BOOST_FOREACH(const DefinitionMap::value_type& kv, definitions) {
		std::cout << "static Value " << kv.first << "(const Object::Ptr& context);" << "\n";
	}

	std::cout << "\n"
		  << "static Dictionary::Ptr l_ModuleScope = new Dictionary();" << "\n"
		  << "\n"
		  << "static void RunCode(void)" << "\n"
		  << "{" << "\n"
		  << "  " << name << "(l_ModuleScope);" << "\n"
		  << "}" << "\n"
		  << "\n"
		  << "INITIALIZE_ONCE(RunCode);" << "\n"
		  << "\n"
		  << "int main(int argc, char **argv)" << "\n"
		  << "{" << "\n"
		  << "  RunCode();" << "\n"
		  << "  if (l_ModuleScope->Contains(\"__result\"))" << "\n"
		  << "    std::cout << \"Result: \" << JsonEncode(l_ModuleScope->Get(\"__result\")) << \"\\n\";" << "\n"
		  << "  else" << "\n"
		  << "    std::cout << \"No result.\" << \"\\n\";" << "\n"
		  << "}" << "\n"
		  << "\n";

	BOOST_FOREACH(const DefinitionMap::value_type& kv, definitions) {
		std::cout << kv.second << "\n";
	}

	delete expr;

	return 0;
}
