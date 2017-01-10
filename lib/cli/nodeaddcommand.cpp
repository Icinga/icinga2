/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "cli/nodeaddcommand.hpp"
#include "cli/nodeutility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <iostream>
#include <fstream>
#include <vector>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("node/add", NodeAddCommand);

String NodeAddCommand::GetDescription(void) const
{
	return "Add Icinga 2 node.";
}

String NodeAddCommand::GetShortDescription(void) const
{
	return "add node";
}

int NodeAddCommand::GetMinArguments(void) const
{
	return 1;
}

bool NodeAddCommand::IsDeprecated(void) const
{
	return true;
}

/**
 * The entry point for the "node add" CLI command.
 *
 * @returns An exit status.
 */
int NodeAddCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	NodeUtility::AddNode(ap[0]);

	return 0;
}
