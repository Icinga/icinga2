/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/carestorecommand.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"
#include "remote/apilistener.hpp"

using namespace icinga;

REGISTER_CLICOMMAND("ca/restore", CARestoreCommand);

/**
 * Provide a long CLI description sentence.
 *
 * @return text
 */
String CARestoreCommand::GetDescription() const
{
	return "Restores a previously removed certificate request.";
}

/**
 * Provide a short CLI description.
 *
 * @return text
 */
String CARestoreCommand::GetShortDescription() const
{
	return "restores a removed certificate request";
}

/**
 * Define minimum arguments without key parameter.
 *
 * @return number of arguments
 */
int CARestoreCommand::GetMinArguments() const
{
	return 1;
}

/**
 * The entry point for the "ca restore" CLI command.
 *
 * @returns An exit status.
 */
int CARestoreCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	String fingerPrint = ap[0];
	String removedRequestFile = ApiListener::GetCertificateRequestsDir() + "/" + fingerPrint + ".removed";

	if (!Utility::PathExists(removedRequestFile)) {
		Log(LogCritical, "cli")
			<< "Cannot find removed fingerprint '" << fingerPrint << "', bailing out.";
		return 1;
	}

	Dictionary::Ptr request = Utility::LoadJsonFile(removedRequestFile);
	std::shared_ptr<X509> certRequest = StringToCertificate(request->Get("cert_request"));

	if (!certRequest) {
		Log(LogCritical, "cli", "Certificate request is invalid. Could not parse X.509 certificate for the 'cert_request' attribute.");
		/* Purge the file when we know that it is broken. */
		Utility::Remove(removedRequestFile);
		return 1;
	}

	Utility::SaveJsonFile(ApiListener::GetCertificateRequestsDir() + "/" + fingerPrint + ".json", 0600, request);

	Utility::Remove(removedRequestFile);

	Log(LogInformation, "cli")
		<< "Restored certificate request for CN '" << GetCertificateCN(certRequest) << "', sign it with:\n"
	       	<< "\"icinga2 ca sign " << fingerPrint << "\"";

	return 0;
}
