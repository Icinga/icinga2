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

#include "cli/agentlistcommand.hpp"
#include "cli/agentutility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/console.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <iostream>
#include <fstream>
#include <vector>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("agent/list", AgentListCommand);

String AgentListCommand::GetDescription(void) const
{
	return "Lists all Icinga 2 agents.";
}

String AgentListCommand::GetShortDescription(void) const
{
	return "lists all agents";
}

void AgentListCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("batch", "list agents in json");
}

/**
 * The entry point for the "agent list" CLI command.
 *
 * @returns An exit status.
 */
int AgentListCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	if (!ap.empty()) {
		Log(LogWarning, "cli")
		    << "Ignoring parameters: " << boost::algorithm::join(ap, " ");
	}

	if (vm.count("batch")) {
		AgentUtility::PrintAgentsJson(std::cout);
		std::cout << "\n";
		return 0;
	}

	std::cout << "Configured agents: \n";
	AgentUtility::PrintAgents(std::cout);
	std::cout << "\n";

	return 0;
}
