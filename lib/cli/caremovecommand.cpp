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

#include "cli/caremovecommand.hpp"
#include "remote/apilistener.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"

using namespace icinga;

REGISTER_CLICOMMAND("ca/remove", CARemoveCommand);

String CARemoveCommand::GetDescription() const
{
	return "Removes an outstanding certificate request.";
}

String CARemoveCommand::GetShortDescription() const
{
	return "removes an outstanding certificate request";
}

int CARemoveCommand::GetMinArguments() const
{
	return 1;
}

ImpersonationLevel CARemoveCommand::GetImpersonationLevel() const
{
	return ImpersonateIcinga;
}

/**
 * The entry point for the "ca remove" CLI command.
 *
 * @returns An exit status.
 */
int CARemoveCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	String requestFile = ApiListener::GetCertificateRequestsDir() + "/" + ap[0] + ".json";

	if (!Utility::PathExists(requestFile)) {
		Log(LogCritical, "cli")
			<< "No request exists for fingerprint '" << ap[0] << "'.";
		return 1;
	}

	if(remove(requestFile.CStr()) != 0)
		return 1;

	Log(LogInformation, "cli")
		<< "Certificate " << ap[0] << " removed.";

	return 0;
}
