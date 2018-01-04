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
	boost::program_options::options_description& hiddenDesc) const
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
int PKINewCertCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
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
