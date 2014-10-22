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

#include "cli/agentsetupcommand.hpp"
#include "cli/agentutility.hpp"
#include "cli/featureutility.hpp"
#include "cli/pkiutility.hpp"
#include "base/logger.hpp"
#include "base/console.hpp"
#include "base/application.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <iostream>
#include <fstream>
#include <vector>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("agent/setup", AgentSetupCommand);

String AgentSetupCommand::GetDescription(void) const
{
	return "Sets up an Icinga 2 agent.";
}

String AgentSetupCommand::GetShortDescription(void) const
{
	return "set up agent";
}

void AgentSetupCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("zone", po::value<std::string>(), "The name of the local zone")
		("master_zone", po::value<std::string>(), "The name of the master zone")
		("endpoint", po::value<std::vector<std::string> >(), "Connect to remote endpoint(s) on host,port")
		("listen", po::value<std::string>(), "Listen on host,port")
		("ticket", po::value<std::string>(), "Generated ticket number for this request")
		("trustedcert", po::value<std::string>(), "Trusted master certificate file")
		("cn", po::value<std::string>(), "The certificate's common name")
		("master", "Use setup for a master instance");
}

std::vector<String> AgentSetupCommand::GetArgumentSuggestions(const String& argument, const String& word) const
{
	if (argument == "keyfile" || argument == "certfile" || argument == "trustedcert")
		return GetBashCompletionSuggestions("file", word);
	else if (argument == "host")
		return GetBashCompletionSuggestions("hostname", word);
	else if (argument == "port")
		return GetBashCompletionSuggestions("service", word);
	else
		return CLICommand::GetArgumentSuggestions(argument, word);
}

/**
 * The entry point for the "agent setup" CLI command.
 *
 * @returns An exit status.
 */
int AgentSetupCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	if (!ap.empty()) {
		Log(LogWarning, "cli")
		    << "Ignoring parameters: " << boost::algorithm::join(ap, " ");
	}

	if (vm.count("master")) {
		return SetupMaster(vm, ap);
	} else {
		return SetupAgent(vm, ap);
	}

	return 0;
}

int AgentSetupCommand::SetupMaster(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap)
{
	/* 1. Generate a new CA, if not already existing */
	Log(LogInformation, "cli")
	    << "Generating new CA.";

	if (PkiUtility::NewCa() > 0) {
		Log(LogWarning, "cli", "Found CA, skipping and using the existing one.\n");
	}

	/* 2. Generate a self signed certificate */
	Log(LogInformation, "cli", "Generating new self-signed certificate.");

	String local_pki_path = PkiUtility::GetLocalPkiPath();

	if (!Utility::MkDirP(local_pki_path, 0700)) {
		Log(LogCritical, "cli")
		    << "Could not create local pki directory '" << local_pki_path << "'.";
		return 1;
	}

	String cn = Utility::GetFQDN();

	if (vm.count("cn"))
		cn = vm["cn"].as<std::string>();

	String keyfile = local_pki_path + "/" + cn + ".key";
	String certfile = local_pki_path + "/" + cn + ".crt";
	String cafile = PkiUtility::GetLocalCaPath() + "/ca.crt";

	if (PkiUtility::NewCert(cn, keyfile, Empty, certfile) > 0) {
		Log(LogCritical, "cli", "Failed to create self-signed certificate");
	}

	/* 3. Copy certificates to /etc/icinga2/pki */
	String pki_path = PkiUtility::GetPkiPath();

	Log(LogInformation, "cli")
	    << "Moving certificates to " << pki_path << ".";

	String target_keyfile = pki_path + "/" + cn + ".key";
	String target_certfile = pki_path + "/" + cn + ".crt";
	String target_cafile = pki_path + "/ca.crt";

	//TODO
	PkiUtility::CopyCertFile(keyfile, target_keyfile);
	PkiUtility::CopyCertFile(certfile, target_certfile);
	PkiUtility::CopyCertFile(cafile, target_cafile);

	std::cout << ConsoleColorTag(Console_ForegroundRed | Console_Bold) << "PLACEHOLDER" << ConsoleColorTag(Console_Normal) << std::endl;

	/* 4. read zones.conf and update with zone + endpoint information */
	Log(LogInformation, "cli", "Generating zone and object configuration.");

	std::cout << ConsoleColorTag(Console_ForegroundRed | Console_Bold) << "PLACEHOLDER" << ConsoleColorTag(Console_Normal) << std::endl;

	/* 5. enable the ApiListener config (verifiy its data) */
	Log(LogInformation, "cli", "Enabling the APIListener feature.");

	String api_path = FeatureUtility::GetFeaturesEnabledPath() + "/api.conf";

	std::cout << ConsoleColorTag(Console_ForegroundRed | Console_Bold) << "PLACEHOLDER" << ConsoleColorTag(Console_Normal) << std::endl;

	/* 5. tell the user to set a safe salt in api.conf */
	Log(LogInformation, "cli")
	    << "Edit the api feature config file '" << api_path << "' and set a secure 'ticket_salt' attribute.";

	/* 6. tell the user to reload icinga2 */
	Log(LogInformation, "cli", "Make sure to restart Icinga 2.");

	return 0;
}

int AgentSetupCommand::SetupAgent(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap)
{
	/* 1. require ticket number (generated on master) */
	if (!vm.count("ticket")) {
		Log(LogCritical, "cli")
		    << "Please pass the ticket number generated on master\n"
		    << "(Hint: 'icinga2 pki ticket --cn <masterhost> --salt <ticket_salt>').";
		return 1;
	}

	String ticket = vm["ticket"].as<std::string>();

	Log(LogInformation, "cli")
	    << "Verifying ticket '" << ticket << "'.";

	/* 2. trusted cert must be passed (retrieved by the user with 'pki save-cert' before) */
	if (!vm.count("trustedcert")) {
		Log(LogCritical, "cli")
		    << "Please pass the trusted cert retrieved from the master\n"
		    << "(Hint: 'icinga2 pki save-cert --host <masterhost> --port <5665> --keyfile local.key --certfile local.crt --trustedfile master.crt').";
		return 1;
	}

	String trustedcert = vm["trustedcert"].as<std::string>();

	Log(LogInformation, "cli")
	    << "Verifying trusted certificate from file '" << trustedcert << "'.";

	String cn = Utility::GetFQDN();

	if (vm.count("cn"))
		cn = vm["cn"].as<std::string>();

	/* 3. retrieve CN and pass it if requested (defaults to FQDN) */
	Log(LogInformation, "cli")
	    << "Verifying CN (defaults to FQDN): '" << cn << "'.";

	std::cout << ConsoleColorTag(Console_ForegroundRed | Console_Bold) << "PLACEHOLDER" << ConsoleColorTag(Console_Normal) << std::endl;

	String pki_path = PkiUtility::GetPkiPath();

	/* 4. pki request a signed certificate from the master */
	Log(LogInformation, "cli", "Requesting a signed certificate from the master.");

	std::cout << ConsoleColorTag(Console_ForegroundRed | Console_Bold) << "PLACEHOLDER" << ConsoleColorTag(Console_Normal) << std::endl;

	/* 5. get public key signed by the master, private key and ca.crt and copy it to /etc/icinga2/pki */
	Log(LogInformation, "cli")
	    << "Copying retrieved signed certificate, private key and ca.crt to pki path '" << pki_path << "'.";

	std::cout << ConsoleColorTag(Console_ForegroundRed | Console_Bold) << "PLACEHOLDER" << ConsoleColorTag(Console_Normal) << std::endl;

	/* 6. generate local zones.conf with zone+endpoint */
	Log(LogInformation, "cli", "Generating zone and object configuration.");

	std::cout << ConsoleColorTag(Console_ForegroundRed | Console_Bold) << "PLACEHOLDER" << ConsoleColorTag(Console_Normal) << std::endl;

	/* 7. update constants.conf with NodeName = CN */
	Log(LogInformation, "cli", "Updating configuration with NodeName constant.");

	std::cout << ConsoleColorTag(Console_ForegroundRed | Console_Bold) << "PLACEHOLDER" << ConsoleColorTag(Console_Normal) << std::endl;

	/* 8. tell the user to reload icinga2 */
	Log(LogInformation, "cli", "Make sure to restart Icinga 2.");

	return 0;
}
