/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/caremovecommand.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"
#include "remote/apilistener.hpp"

using namespace icinga;

REGISTER_CLICOMMAND("ca/remove", CARemoveCommand);

/**
 * Provide a long CLI description sentence.
 *
 * @return text
 */
String CARemoveCommand::GetDescription() const
{
	return "Removes an outstanding certificate request.";
}

/**
 * Provide a short CLI description.
 *
 * @return text
 */
String CARemoveCommand::GetShortDescription() const
{
	return "removes an outstanding certificate request";
}

/**
 * Define minimum arguments without key parameter.
 *
 * @return number of arguments
 */
int CARemoveCommand::GetMinArguments() const
{
	return 1;
}

/**
 * Impersonate as Icinga user.
 *
 * @return impersonate level
 */
ImpersonationLevel CARemoveCommand::GetImpersonationLevel() const
{
	return ImpersonateIcinga;
}

/**
 * The entry point for the "ca remove" CLI command.
 *
 * @returns An exit status.
 */
int CARemoveCommand::Run(const boost::program_options::variables_map&, const std::vector<std::string>& ap) const
{
	String fingerPrint = ap[0];
	String requestFile = ApiListener::GetCertificateRequestsDir() + "/" + fingerPrint + ".json";

	if (!Utility::PathExists(requestFile)) {
		Log(LogCritical, "cli")
			<< "No request exists for fingerprint '" << fingerPrint << "'.";
		return 1;
	}

	Dictionary::Ptr request = Utility::LoadJsonFile(requestFile);
	std::shared_ptr<X509> certRequest = StringToCertificate(request->Get("cert_request"));

	if (!certRequest) {
		Log(LogCritical, "cli", "Certificate request is invalid. Could not parse X.509 certificate for the 'cert_request' attribute.");
		return 1;
	}

	String cn = GetCertificateCN(certRequest);

	if (request->Contains("cert_response")) {
		Log(LogCritical, "cli")
			<< "Certificate request for CN '" << cn << "' already signed, removal is not possible.";
		return 1;
	}

	Utility::SaveJsonFile(ApiListener::GetCertificateRequestsDir() + "/" + fingerPrint + ".removed", 0600, request);

	Utility::Remove(requestFile);

	Log(LogInformation, "cli")
		<< "Certificate request for CN " << cn << " removed.";

	return 0;
}
