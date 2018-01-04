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

#include "cli/apisetupcommand.hpp"
#include "cli/apisetuputility.hpp"
#include "cli/variableutility.hpp"
#include "base/logger.hpp"
#include "base/console.hpp"
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("api/setup", ApiSetupCommand);

String ApiSetupCommand::GetDescription() const
{
	return "Setup for Icinga 2 API.";
}

String ApiSetupCommand::GetShortDescription() const
{
	return "setup for api";
}

ImpersonationLevel ApiSetupCommand::GetImpersonationLevel() const
{
	return ImpersonateRoot;
}

int ApiSetupCommand::GetMaxArguments() const
{
	return -1;
}

/**
 * The entry point for the "api setup" CLI command.
 *
 * @returns An exit status.
 */
int ApiSetupCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	String cn = VariableUtility::GetVariable("NodeName");

	if (cn.IsEmpty())
		cn = Utility::GetFQDN();

	if (!ApiSetupUtility::SetupMaster(cn, true))
		return 1;

	return 0;
}
