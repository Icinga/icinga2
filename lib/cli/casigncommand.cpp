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

#include "cli/casigncommand.hpp"
#include "remote/apilistener.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"

using namespace icinga;

REGISTER_CLICOMMAND("ca/sign", CASignCommand);

String CASignCommand::GetDescription() const
{
	return "Signs an outstanding certificate request.";
}

String CASignCommand::GetShortDescription() const
{
	return "signs an outstanding certificate request";
}

int CASignCommand::GetMinArguments() const
{
	return 1;
}

ImpersonationLevel CASignCommand::GetImpersonationLevel() const
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
	String requestFile = ApiListener::GetCertificateRequestsDir() + "/" + ap[0] + ".json";

	if (!Utility::PathExists(requestFile)) {
		Log(LogCritical, "cli")
			<< "No request exists for fingerprint '" << ap[0] << "'.";
		return 1;
	}

	Dictionary::Ptr request = Utility::LoadJsonFile(requestFile);

	if (!request)
		return 1;

	String certRequestText = request->Get("cert_request");

	std::shared_ptr<X509> certRequest = StringToCertificate(certRequestText);

	if (!certRequest) {
		Log(LogCritical, "cli", "Certificate request is invalid. Could not parse X.509 certificate for the 'cert_request' attribute.");
		return 1;
	}

	std::shared_ptr<X509> certResponse = CreateCertIcingaCA(certRequest);

	BIO *out = BIO_new(BIO_s_mem());
	X509_NAME_print_ex(out, X509_get_subject_name(certRequest.get()), 0, XN_FLAG_ONELINE & ~ASN1_STRFLGS_ESC_MSB);

	char *data;
	long length;
	length = BIO_get_mem_data(out, &data);

	String subject = String(data, data + length);
	BIO_free(out);

	if (!certResponse) {
		Log(LogCritical, "cli")
			<< "Could not sign certificate for '" << subject << "'.";
		return 1;
	}

	request->Set("cert_response", CertificateToString(certResponse));

	Utility::SaveJsonFile(requestFile, 0600, request);

	Log(LogInformation, "cli")
		<< "Signed certificate for '" << subject << "'.";

	return 0;
}
