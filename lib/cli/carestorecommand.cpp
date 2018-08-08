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

#include "cli/carestorecommand.hpp"
#include "remote/apilistener.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"

using namespace icinga;

REGISTER_CLICOMMAND("ca/restore", CARestoreCommand);

String CARestoreCommand::GetDescription() const
{
	return "Restores a previously removed certificate request.";
}

String CARestoreCommand::GetShortDescription() const
{
	return "restores a removed certificate request";
}

int CARestoreCommand::GetMinArguments() const
{
	return 1;
}

ImpersonationLevel CARestoreCommand::GetImpersonationLevel() const
{
	return ImpersonateIcinga;
}

/**
 * The entry point for the "ca restore" CLI command.
 *
 * @returns An exit status.
 */
int CARestoreCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	String requestFile = ApiListener::GetCertificateRequestsDir() + "/" + ap[0] + ".removed";

	if (!Utility::PathExists(requestFile)) {
		Log(LogCritical, "cli")
			<< "No removed request exists for fingerprint '" << ap[0] << "'.";
		return 1;
	}

	Dictionary::Ptr request = Utility::LoadJsonFile(requestFile);
	std::shared_ptr<X509> certRequest = StringToCertificate(request->Get("cert_request"));

	if (!certRequest) {
		Log(LogCritical, "cli", "Certificate request is invalid. Could not parse X.509 certificate for the 'cert_request' attribute.");
		return 1;
	}

	Utility::SaveJsonFile(ApiListener::GetCertificateRequestsDir() + "/" + ap[0] + ".json", 0600, request);
	if(remove(requestFile.CStr()) != 0)
		return 1;

	Log(LogInformation, "cli")
		<< "Certificate " << GetCertificateCN(certRequest) << " restored, you can now sign it using:\n"
	       	<< "\"icinga2 ca sign " << ap[0] << "\"";

	return 0;
}
