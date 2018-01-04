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

#include "cli/variablelistcommand.hpp"
#include "cli/variableutility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/convert.hpp"
#include "base/configobject.hpp"
#include "base/debug.hpp"
#include "base/objectlock.hpp"
#include "base/console.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("variable/list", VariableListCommand);

String VariableListCommand::GetDescription() const
{
	return "Lists all Icinga 2 variables.";
}

String VariableListCommand::GetShortDescription() const
{
	return "lists all variables";
}

/**
 * The entry point for the "variable list" CLI command.
 *
 * @returns An exit status.
 */
int VariableListCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	String varsfile = Application::GetVarsPath();

	if (!Utility::PathExists(varsfile)) {
		Log(LogCritical, "cli")
			<< "Cannot open variables file '" << varsfile << "'.";
		Log(LogCritical, "cli", "Run 'icinga2 daemon -C' to validate config and generate the cache file.");
		return 1;
	}

	VariableUtility::PrintVariables(std::cout);

	return 0;
}

