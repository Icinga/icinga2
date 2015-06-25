/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "cli/apisetupcommand.hpp"
#include "cli/apisetuputility.hpp"
#include "base/logger.hpp"
#include "base/console.hpp"
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("api/setup", ApiSetupCommand);

String ApiSetupCommand::GetDescription(void) const
{
	return "Setup for Icinga 2 API.";
}

String ApiSetupCommand::GetShortDescription(void) const
{
	return "setup for api";
}

ImpersonationLevel ApiSetupCommand::GetImpersonationLevel(void) const
{
	return ImpersonateRoot;
}

int ApiSetupCommand::GetMaxArguments(void) const
{
	return -1;
}

/**
 * The entry point for the "node wizard" CLI command.
 *
 * @returns An exit status.
 */
int ApiSetupCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	/* 1. generate CA & signed certificate
	 * 2. update password inside api-users.conf for the "root" user
	 * TODO:
	 * - setup the api on a client?
	 */

	int result = ApiSetupUtility::SetupMaster(Utility::GetFQDN());

	if (result > 0) {
		Log(LogCritical, "ApiSetup", "Error occured. Bailing out.");
		return result;
	}

	std::cout << "Done.\n\n";

	std::cout << "Now restart your Icinga 2 daemon to finish the installation!\n\n";

	return 0;
}
