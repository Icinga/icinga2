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

#include "cli/agentsetupcommand.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <iostream>
#include <fstream>
#include <vector>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("agent/setup", AgentSetupCommand);

String AgentSetupCommand::GetDescription(void) const
{
	return "Sets up an Icinga 2 agent.";
}

String AgentSetupCommand::GetShortDescription(void) const
{
	return "set up agent";
}

void AgentSetupCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("zone", po::value<std::string>(), "The name of the local zone")
		("master_zone", po::value<std::string>(), "The name of the master zone")
		("endpoint", po::value<std::vector<std::string> >(), "Connect to remote endpoint(s) on host,port")
		("listen", po::value<std::string>(), "Listen on host,port")
		("ticket", po::value<std::string>(), "Generated ticket number for this request")
		("trustedcert", po::value<std::string>(), "Trusted master certificate file")
		("cn", po::value<std::string>(), "The certificate's common name")
		("master", po::value<std::string>(), "Use setup for a master instance");
}

/**
 * The entry point for the "agent setup" CLI command.
 *
 * @returns An exit status.
 */
int AgentSetupCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	if (!ap.empty()) {
		Log(LogWarning, "cli")
		    << "Ignoring parameters: " << boost::algorithm::join(ap, " ");
	}

	Log(LogWarning, "cli", "TODO: Not implemented yet.");

	if (vm.count("master")) {
		SetupMaster(vm, ap);
	} else {
		SetupAgent(vm, ap);
	}

	return 0;
}

bool AgentSetupCommand::SetupMaster(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap)
{
	return true;
}

bool AgentSetupCommand::SetupAgent(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap)
{
	return true;
}
