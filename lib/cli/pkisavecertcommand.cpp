/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/pkisavecertcommand.hpp"
#include "remote/pkiutility.hpp"
#include "base/logger.hpp"
#include "base/tlsutility.hpp"
#include "base/console.hpp"
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("pki/save-cert", PKISaveCertCommand);

String PKISaveCertCommand::GetDescription() const
{
	return "Saves another Icinga 2 instance's certificate.";
}

String PKISaveCertCommand::GetShortDescription() const
{
	return "saves another Icinga 2 instance's certificate";
}

void PKISaveCertCommand::InitParameters(boost::program_options::options_description& visibleDesc,
	boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("trustedcert", po::value<std::string>(), "Trusted certificate file path (output)")
		("host", po::value<std::string>(), "Parent Icinga instance to fetch the public TLS certificate from")
		("port", po::value<std::string>()->default_value("5665"), "Icinga 2 port");

	hiddenDesc.add_options()
		("key", po::value<std::string>())
		("cert", po::value<std::string>());
}

std::vector<String> PKISaveCertCommand::GetArgumentSuggestions(const String& argument, const String& word) const
{
	if (argument == "trustedcert")
		return GetBashCompletionSuggestions("file", word);
	else if (argument == "host")
		return GetBashCompletionSuggestions("hostname", word);
	else if (argument == "port")
		return GetBashCompletionSuggestions("service", word);
	else
		return CLICommand::GetArgumentSuggestions(argument, word);
}

/**
 * The entry point for the "pki save-cert" CLI command.
 *
 * @returns An exit status.
 */
int PKISaveCertCommand::Run(const boost::program_options::variables_map& vm, [[maybe_unused]] const std::vector<std::string>& ap) const
{
	if (!vm.count("host")) {
		Log(LogCritical, "cli", "Icinga 2 host (--host) must be specified.");
		return 1;
	}

	if (!vm.count("trustedcert")) {
		Log(LogCritical, "cli", "Trusted certificate output file path (--trustedcert) must be specified.");
		return 1;
	}

	String host = vm["host"].as<std::string>();
	String port = vm["port"].as<std::string>();

	Log(LogInformation, "cli")
		<< "Retrieving TLS certificate for '" << host << ":" << port << "'.";

	std::shared_ptr<X509> cert = PkiUtility::FetchCert(host, port);

	if (!cert) {
		Log(LogCritical, "cli", "Failed to fetch certificate from host.");
		return 1;
	}

	std::cout << PkiUtility::GetCertificateInformation(cert) << "\n";
	std::cout << ConsoleColorTag(Console_ForegroundRed)
		<< "***\n"
		<< "*** You have to ensure that this certificate actually matches the parent\n"
		<< "*** instance's certificate in order to avoid man-in-the-middle attacks.\n"
		<< "***\n\n"
		<< ConsoleColorTag(Console_Normal);

	return PkiUtility::WriteCert(cert, vm["trustedcert"].as<std::string>());
}
