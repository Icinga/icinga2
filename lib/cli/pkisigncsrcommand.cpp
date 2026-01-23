/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/pkisigncsrcommand.hpp"
#include "remote/pkiutility.hpp"
#include "base/logger.hpp"

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("pki/sign-csr", PKISignCSRCommand);

String PKISignCSRCommand::GetDescription() const
{
	return "Reads a Certificate Signing Request from stdin and prints a signed certificate on stdout.";
}

String PKISignCSRCommand::GetShortDescription() const
{
	return "signs a CSR";
}

void PKISignCSRCommand::InitParameters(boost::program_options::options_description& visibleDesc,
	[[maybe_unused]] boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("csr", po::value<std::string>(), "CSR file path (input)")
		("cert", po::value<std::string>(), "Certificate file path (output)");
}

std::vector<String> PKISignCSRCommand::GetArgumentSuggestions(const String& argument, const String& word) const
{
	if (argument == "csr" || argument == "cert")
		return GetBashCompletionSuggestions("file", word);
	else
		return CLICommand::GetArgumentSuggestions(argument, word);
}

/**
 * The entry point for the "pki sign-csr" CLI command.
 *
 * @returns An exit status.
 */
int PKISignCSRCommand::Run(const boost::program_options::variables_map& vm, [[maybe_unused]] const std::vector<std::string>& ap) const
{
	if (!vm.count("csr")) {
		Log(LogCritical, "cli", "Certificate signing request file path (--csr) must be specified.");
		return 1;
	}

	if (!vm.count("cert")) {
		Log(LogCritical, "cli", "Certificate file path (--cert) must be specified.");
		return 1;
	}

	return PkiUtility::SignCsr(vm["csr"].as<std::string>(), vm["cert"].as<std::string>());
}
