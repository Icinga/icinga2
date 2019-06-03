/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/calistcommand.hpp"
#include "remote/apilistener.hpp"
#include "remote/pkiutility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"
#include "base/json.hpp"
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("ca/list", CAListCommand);

String CAListCommand::GetDescription() const
{
	return "Lists pending certificate signing requests.";
}

String CAListCommand::GetShortDescription() const
{
	return "lists pending certificate signing requests";
}

void CAListCommand::InitParameters(boost::program_options::options_description& visibleDesc,
	boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("all", "List all certificate signing requests, including signed. Note: Old requests are automatically cleaned by Icinga after 1 week.")
		("json", "encode output as JSON");
}

/**
 * The entry point for the "ca list" CLI command.
 *
 * @returns An exit status.
 */
int CAListCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	Dictionary::Ptr requests = PkiUtility::GetCertificateRequests();

	if (vm.count("json"))
		std::cout << JsonEncode(requests);
	else {
		ObjectLock olock(requests);

		std::cout << "Fingerprint                                                      | Timestamp                | Signed | Subject\n";
		std::cout << "-----------------------------------------------------------------|--------------------------|--------|--------\n";

		for (auto& kv : requests) {
			Dictionary::Ptr request = kv.second;

			/* Skip signed requests by default. */
			if (!vm.count("all") && request->Contains("cert_response"))
				continue;

			std::cout << kv.first
				<< " | "
/*			    << Utility::FormatDateTime("%Y/%m/%d %H:%M:%S", request->Get("timestamp")) */
				<< request->Get("timestamp")
				<< " | "
				<< (request->Contains("cert_response") ? "*" : " ") << "     "
				<< " | "
				<< request->Get("subject")
				<< "\n";
		}
	}

	return 0;
}
