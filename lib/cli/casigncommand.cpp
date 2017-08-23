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

#include "cli/casigncommand.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"

using namespace icinga;

REGISTER_CLICOMMAND("ca/sign", CASignCommand);

String CASignCommand::GetDescription(void) const
{
	return "Signs an outstanding certificate request.";
}

String CASignCommand::GetShortDescription(void) const
{
	return "signs an outstanding certificate request";
}

int CASignCommand::GetMinArguments(void) const
{
	return 1;
}

ImpersonationLevel CASignCommand::GetImpersonationLevel(void) const
{
        return ImpersonateIcinga;
}

/**
 * The entry point for the "ca sign" CLI command.
 *
 * @returns An exit status.
 */
int CASignCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	String requestFile = Application::GetLocalStateDir() + "/lib/icinga2/pki-requests/" + ap[0] + ".json";

	Dictionary::Ptr request = Utility::LoadJsonFile(requestFile);

	if (!request)
		return 1;

	String certRequestText = request->Get("cert_request");

	boost::shared_ptr<X509> certRequest = StringToCertificate(certRequestText);

	boost::shared_ptr<X509> certResponse = CreateCertIcingaCA(certRequest);

	request->Set("cert_response", CertificateToString(certResponse));

	Utility::SaveJsonFile(requestFile, 0600, request);

	return 0;
}
