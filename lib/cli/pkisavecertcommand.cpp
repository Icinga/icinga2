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

#include "cli/pkisavecertcommand.hpp"
#include "remote/pkiutility.hpp"
#include "base/logger.hpp"
#include "base/tlsutility.hpp"
#include "base/console.hpp"
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("pki/save-cert", PKISaveCertCommand);

String PKISaveCertCommand::GetDescription(void) const
{
	return "Saves another Icinga 2 instance's certificate.";
}

String PKISaveCertCommand::GetShortDescription(void) const
{
	return "saves another Icinga 2 instance's certificate";
}

void PKISaveCertCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
	    ("key", po::value<std::string>(), "Key file path (input), obsolete")
	    ("cert", po::value<std::string>(), "Certificate file path (input), obsolete")
	    ("trustedcert", po::value<std::string>(), "Trusted certificate file path (output)")
	    ("host", po::value<std::string>(), "Icinga 2 host")
	    ("port", po::value<std::string>()->default_value("5665"), "Icinga 2 port");
}

std::vector<String> PKISaveCertCommand::GetArgumentSuggestions(const String& argument, const String& word) const
{
	if (argument == "key" || argument == "cert" || argument == "trustedcert")
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
int PKISaveCertCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
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
	    << "Retrieving X.509 certificate for '" << host << ":" << port << "'.";

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
