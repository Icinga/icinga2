// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "cli/pkinewcertcommand.hpp"
#include "remote/pkiutility.hpp"
#include "base/logger.hpp"

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("pki/new-cert", PKINewCertCommand);

String PKINewCertCommand::GetDescription() const
{
	return "Creates a new Certificate Signing Request, a self-signed X509 certificate or both.";
}

String PKINewCertCommand::GetShortDescription() const
{
	return "creates a new CSR";
}

void PKINewCertCommand::InitParameters(boost::program_options::options_description& visibleDesc,
	[[maybe_unused]] boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("cn", po::value<std::string>(), "Common Name")
		("key", po::value<std::string>(), "Key file path (output)")
		("csr", po::value<std::string>(), "CSR file path (optional, output)")
		("cert", po::value<std::string>(), "Certificate file path (optional, output)");
}

std::vector<String> PKINewCertCommand::GetArgumentSuggestions(const String& argument, const String& word) const
{
	if (argument == "key" || argument == "csr" || argument == "cert")
		return GetBashCompletionSuggestions("file", word);
	else
		return CLICommand::GetArgumentSuggestions(argument, word);
}

/**
 * The entry point for the "pki new-cert" CLI command.
 *
 * @returns An exit status.
 */
int PKINewCertCommand::Run(const boost::program_options::variables_map& vm, [[maybe_unused]] const std::vector<std::string>& ap) const
{
	if (!vm.count("cn")) {
		Log(LogCritical, "cli", "Common name (--cn) must be specified.");
		return 1;
	}

	if (!vm.count("key")) {
		Log(LogCritical, "cli", "Key file path (--key) must be specified.");
		return 1;
	}

	String csr, cert;

	if (vm.count("csr"))
		csr = vm["csr"].as<std::string>();

	if (vm.count("cert"))
		cert = vm["cert"].as<std::string>();

	return PkiUtility::NewCert(vm["cn"].as<std::string>(), vm["key"].as<std::string>(), csr, cert);
}
