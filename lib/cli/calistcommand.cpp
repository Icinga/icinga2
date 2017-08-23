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

#include "cli/calistcommand.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"

using namespace icinga;

REGISTER_CLICOMMAND("ca/list", CAListCommand);

String CAListCommand::GetDescription(void) const
{
	return "Lists all certificate signing requests.";
}

String CAListCommand::GetShortDescription(void) const
{
	return "lists all certificate signing requests";
}

void CAListCommand::PrintRequest(const String& requestFile)
{
	Dictionary::Ptr request = Utility::LoadJsonFile(requestFile);

	if (!request)
		return;

	String fingerprint = Utility::BaseName(requestFile);
	fingerprint = fingerprint.SubStr(0, fingerprint.GetLength() - 5);

	std::cout << "***\n";
	std::cout << "Fingerprint: " << fingerprint << "\n";

	String certRequestText = request->Get("cert_request");

	boost::shared_ptr<X509> certRequest = StringToCertificate(certRequestText);

	String cn = GetCertificateCN(certRequest);

	String certResponseText = request->Get("cert_response");

	if (!certResponseText.IsEmpty()) {
		boost::shared_ptr<X509> certResponse = StringToCertificate(certResponseText);
	}

	std::cout << "CN: " << cn << "\n";
	std::cout << "Certificate (request): " << certRequestText << "\n";
	std::cout << "Certificate (response): " << certResponseText << "\n";
}

/**
 * The entry point for the "ca list" CLI command.
 *
 * @returns An exit status.
 */
int CAListCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	Utility::Glob(Application::GetLocalStateDir() + "/lib/icinga2/pki-requests/*.json", &CAListCommand::PrintRequest, GlobFile);

	return 0;
}
