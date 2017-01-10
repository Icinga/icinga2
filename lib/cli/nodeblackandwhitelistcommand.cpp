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

#include "cli/nodeblackandwhitelistcommand.hpp"
#include "cli/nodeutility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/objectlock.hpp"
#include "base/json.hpp"
#include <iostream>
#include <fstream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_BLACKANDWHITELIST_CLICOMMAND(whitelist);
REGISTER_BLACKANDWHITELIST_CLICOMMAND(blacklist);

BlackAndWhitelistCommand::BlackAndWhitelistCommand(const String& type, BlackAndWhitelistCommandType command)
	: m_Type(type), m_Command(command)
{ }

String BlackAndWhitelistCommand::GetDescription(void) const
{
	String description;

	switch (m_Command) {
		case BlackAndWhitelistCommandAdd:
			description = "Adds a new";
			break;
		case BlackAndWhitelistCommandRemove:
			description = "Removes a";
			break;
		case BlackAndWhitelistCommandList:
			description = "Lists all";
			break;
	}

	description += " " + m_Type + " filter";

	if (m_Command == BlackAndWhitelistCommandList)
		description += "s";

	return description;
}

String BlackAndWhitelistCommand::GetShortDescription(void) const
{
	String description;

	switch (m_Command) {
		case BlackAndWhitelistCommandAdd:
			description = "adds a new";
			break;
		case BlackAndWhitelistCommandRemove:
			description = "removes a";
			break;
		case BlackAndWhitelistCommandList:
			description = "lists all";
			break;
	}

	description += " " + m_Type + " filter";

	if (m_Command == BlackAndWhitelistCommandList)
		description += "s";

	return description;
}

bool BlackAndWhitelistCommand::IsDeprecated(void) const
{
	return true;
}

void BlackAndWhitelistCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	if (m_Command == BlackAndWhitelistCommandAdd || m_Command == BlackAndWhitelistCommandRemove) {
		visibleDesc.add_options()
			("zone", po::value<std::string>(), "The name of the zone")
			("host", po::value<std::string>(), "The name of the host")
			("service", po::value<std::string>(), "The name of the service");
	}
}

/**
 * The entry point for the "node <whitelist/blacklist> <add/remove/list>" CLI command.
 *
 * @returns An exit status.
 */
int BlackAndWhitelistCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	if (m_Command == BlackAndWhitelistCommandAdd) {
		if (!vm.count("zone")) {
			Log(LogCritical, "cli", "At least the zone name filter is required!");
			return 1;
		}

		if (!vm.count("host")) {
			Log(LogCritical, "cli", "At least the host name filter is required!");
			return 1;
		}

		String service_filter;

		if (vm.count("service"))
			service_filter = vm["service"].as<std::string>();

		return NodeUtility::UpdateBlackAndWhiteList(m_Type, vm["zone"].as<std::string>(), vm["host"].as<std::string>(), service_filter);
	} else if (m_Command == BlackAndWhitelistCommandList) {
		return NodeUtility::PrintBlackAndWhiteList(std::cout, m_Type);
	} else if (m_Command == BlackAndWhitelistCommandRemove) {
		if (!vm.count("zone")) {
			Log(LogCritical, "cli", "The zone name filter is required!");
			return 1;
		}

		if (!vm.count("host")) {
			Log(LogCritical, "cli", "The host name filter is required!");
			return 1;
		}

		String zone_filter = vm["zone"].as<std::string>();
		String host_filter = vm["host"].as<std::string>();
		String service_filter;

		if (vm.count("service")) {
			service_filter = vm["service"].as<std::string>();
		}

		return NodeUtility::RemoveBlackAndWhiteList(m_Type, vm["zone"].as<std::string>(), vm["host"].as<std::string>(), service_filter);
	}


	return 0;
}
