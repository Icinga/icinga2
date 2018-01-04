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

#include "cli/variablegetcommand.hpp"
#include "cli/variableutility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/convert.hpp"
#include "base/configobject.hpp"
#include "base/configtype.hpp"
#include "base/json.hpp"
#include "base/netstring.hpp"
#include "base/stdiostream.hpp"
#include "base/debug.hpp"
#include "base/objectlock.hpp"
#include "base/console.hpp"
#include "base/scriptglobal.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("variable/get", VariableGetCommand);

String VariableGetCommand::GetDescription() const
{
	return "Prints the value of an Icinga 2 variable.";
}

String VariableGetCommand::GetShortDescription() const
{
	return "gets a variable";
}

void VariableGetCommand::InitParameters(boost::program_options::options_description& visibleDesc,
	boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("current", "Uses the current value (i.e. from the running process, rather than from the vars file)");
}

int VariableGetCommand::GetMinArguments() const
{
	return 1;
}

/**
 * The entry point for the "variable get" CLI command.
 *
 * @returns An exit status.
 */
int VariableGetCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	if (vm.count("current")) {
		std::cout << ScriptGlobal::Get(ap[0], &Empty) << "\n";
		return 0;
	}

	String varsfile = Application::GetVarsPath();

	if (!Utility::PathExists(varsfile)) {
		Log(LogCritical, "cli")
			<< "Cannot open variables file '" << varsfile << "'.";
		Log(LogCritical, "cli", "Run 'icinga2 daemon -C' to validate config and generate the cache file.");
		return 1;
	}

	Value value = VariableUtility::GetVariable(ap[0]);

	std::cout << value << "\n";

	return 0;
}
