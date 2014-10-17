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

#include "cli/agentblackandwhitelistcommand.hpp"
#include "base/logger_fwd.hpp"
#include "base/clicommand.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include <fstream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_BLACKANDWHITELIST_CLICOMMAND("whitelist");
REGISTER_BLACKANDWHITELIST_CLICOMMAND("blacklist");

RegisterBlackAndWhitelistCLICommandHelper::RegisterBlackAndWhitelistCLICommandHelper(const String& type)
{
	String ltype = type;
	boost::algorithm::to_lower(ltype);

	std::vector<String> name;
	name.push_back("agent");
	name.push_back(ltype);
	name.push_back("add");
	CLICommand::Register(name, make_shared<BlackAndWhitelistCommand>(type, BlackAndWhitelistCommandAdd));

	name[2] = "remove";
	CLICommand::Register(name, make_shared<BlackAndWhitelistCommand>(type, BlackAndWhitelistCommandRemove));

	name[2] = "list";
	CLICommand::Register(name, make_shared<BlackAndWhitelistCommand>(type, BlackAndWhitelistCommandList));
}

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

void BlackAndWhitelistCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("agent", po::value<std::string>(), "The name of the agent")
		("host", po::value<std::string>(), "The name of the host")
		("service", po::value<std::string>(), "The name of the service");

	if (m_Command == BlackAndWhitelistCommandAdd) {
		//TODO: call list functionality
	}
}

/**
 * The entry point for the "agent <whitelist/blacklist> <add/remove/list>" CLI command.
 *
 * @returns An exit status.
 */
int BlackAndWhitelistCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	return 0;
}
