/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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
#include "base/logger.hpp"
#include "base/clicommand.hpp"
#include "base/tlsutility.hpp"

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("pki/new-cert", PKINewCertCommand);

String PKINewCertCommand::GetDescription(void) const
{
	return "Creates a new Certificate Signing Request, a self-signed X509 certificate or both.";
}

String PKINewCertCommand::GetShortDescription(void) const
{
	return "creates a new CSR";
}

void PKINewCertCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("cn", po::value<std::string>(), "Common Name")
		("keyfile", po::value<std::string>(), "Key file path (output")
		("csrfile", po::value<std::string>(), "CSR file path (optional, output)")
		("certfile", po::value<std::string>(), "Certificate file path (optional, output)");
}

std::vector<String> PKINewCertCommand::GetArgumentSuggestions(const String& argument, const String& word) const
{
	if (argument == "keyfile" || argument == "csrfile" || argument == "certfile")
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

	if (!vm.count("keyfile")) {
		Log(LogCritical, "cli", "Key file path (--keyfile) must be specified.");
		return 1;
	}

	String csrfile, certfile;

	if (vm.count("csrfile"))
		csrfile = vm["csrfile"].as<std::string>();

	if (vm.count("certfile"))
		certfile = vm["certfile"].as<std::string>();

	MakeX509CSR(vm["cn"].as<std::string>(), vm["keyfile"].as<std::string>(), csrfile, certfile);

	return 0;
}
