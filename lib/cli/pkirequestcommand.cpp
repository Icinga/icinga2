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

#include "cli/pkirequestcommand.hpp"
#include "remote/pkiutility.hpp"
#include "base/logger.hpp"
#include "base/tlsutility.hpp"
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("pki/request", PKIRequestCommand);

String PKIRequestCommand::GetDescription() const
{
	return "Sends a PKI request to Icinga 2.";
}

String PKIRequestCommand::GetShortDescription() const
{
	return "requests a certificate";
}

void PKIRequestCommand::InitParameters(boost::program_options::options_description& visibleDesc,
	boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("key", po::value<std::string>(), "Key file path (input)")
		("cert", po::value<std::string>(), "Certificate file path (input + output)")
		("ca", po::value<std::string>(), "CA file path (output)")
		("trustedcert", po::value<std::string>(), "Trusted certificate file path (input)")
		("host", po::value<std::string>(), "Icinga 2 host")
		("port", po::value<std::string>(), "Icinga 2 port")
		("ticket", po::value<std::string>(), "Icinga 2 PKI ticket");
}

std::vector<String> PKIRequestCommand::GetArgumentSuggestions(const String& argument, const String& word) const
{
	if (argument == "key" || argument == "cert" || argument == "ca" || argument == "trustedcert")
		return GetBashCompletionSuggestions("file", word);
	else if (argument == "host")
		return GetBashCompletionSuggestions("hostname", word);
	else if (argument == "port")
		return GetBashCompletionSuggestions("service", word);
	else
		return CLICommand::GetArgumentSuggestions(argument, word);
}

/**
 * The entry point for the "pki request" CLI command.
 *
 * @returns An exit status.
 */
int PKIRequestCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	if (!vm.count("host")) {
		Log(LogCritical, "cli", "Icinga 2 host (--host) must be specified.");
		return 1;
	}

	if (!vm.count("key")) {
		Log(LogCritical, "cli", "Key input file path (--key) must be specified.");
		return 1;
	}

	if (!vm.count("cert")) {
		Log(LogCritical, "cli", "Certificate output file path (--cert) must be specified.");
		return 1;
	}

	if (!vm.count("ca")) {
		Log(LogCritical, "cli", "CA certificate output file path (--ca) must be specified.");
		return 1;
	}

	if (!vm.count("trustedcert")) {
		Log(LogCritical, "cli", "Trusted certificate input file path (--trustedcert) must be specified.");
		return 1;
	}

	String port = "5665";
	String ticket;

	if (vm.count("port"))
		port = vm["port"].as<std::string>();

	if (vm.count("ticket"))
		ticket = vm["ticket"].as<std::string>();

	return PkiUtility::RequestCertificate(vm["host"].as<std::string>(), port, vm["key"].as<std::string>(),
		vm["cert"].as<std::string>(), vm["ca"].as<std::string>(), GetX509Certificate(vm["trustedcert"].as<std::string>()),
		ticket);
}
