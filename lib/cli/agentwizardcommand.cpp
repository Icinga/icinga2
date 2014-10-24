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

#include "cli/agentwizardcommand.hpp"
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

REGISTER_CLICOMMAND("agent/wizard", AgentWizardCommand);

String AgentWizardCommand::GetDescription(void) const
{
	return "Wizard for Icinga 2 agent setup.";
}

String AgentWizardCommand::GetShortDescription(void) const
{
	return "wizard for agent setup";
}

/**
 * The entry point for the "agent wizard" CLI command.
 *
 * @returns An exit status.
 */
int AgentWizardCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	Log(LogWarning, "cli", "TODO: Not implemented yet.");

	/*
	 * The wizard will get all information from the user,
	 * and then call all required functions.
	 */

	std::cout << "Welcome to the Icinga 2 Setup Wizard!\n"
	    << "\n"
	    << "We'll guide you through all required configuration details.\n"
	    << "\n"
	    << "If you have questions, please consult the documentation at http://docs.icinga.org\n"
	    << "or join the community support channels at https://support.icinga.org\n"
	    << "\n\n";

	//TODO: Add sort of bash completion to path input?


	/* 0. master or agent setup?
	 * 1. Ticket
	 * 2. Master information for autosigning
	 * 3. Trusted cert location
	 * 4. CN to use (defaults to FQDN)
	 * 5. Local CA
	 * 6. New self signed certificate
	 * 7. Request signed certificate from master
	 * 8. copy key information to /etc/icinga2/pki
	 * 9. enable ApiListener feature
	 * 10. generate zones.conf with endpoints and zone objects
	 * 11. set NodeName = cn in constants.conf
	 * 12. reload icinga2, or tell the user to
	 */

	return 0;
}
