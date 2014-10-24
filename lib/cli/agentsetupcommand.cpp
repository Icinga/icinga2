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
#include "base/tlsutility.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>

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
		("master_host", po::value<std::string>(), "The name of the master host for auto-signing the csr")
		("endpoint", po::value<std::vector<std::string> >(), "Connect to remote endpoint; syntax: cn[,host,port]")
		("listen", po::value<std::string>(), "Listen on host,port")
		("ticket", po::value<std::string>(), "Generated ticket number for this request")
		("trustedcert", po::value<std::string>(), "Trusted master certificate file")
		("cn", po::value<std::string>(), "The certificate's common name")
		("master", "Use setup for a master instance");
}

std::vector<String> AgentSetupCommand::GetArgumentSuggestions(const String& argument, const String& word) const
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

ImpersonationLevel AgentSetupCommand::GetImpersonationLevel(void) const
{
	return ImpersonateRoot;
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

	if (vm.count("master"))
		return SetupMaster(vm, ap);
	else
		return SetupAgent(vm, ap);
}

int AgentSetupCommand::SetupMaster(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap)
{
	/* Ignore not required parameters */
	if (vm.count("ticket"))
		Log(LogWarning, "cli", "Master for Agent setup: Ignoring --ticket");

	if (vm.count("endpoint"))
		Log(LogWarning, "cli", "Master for Agent setup: Ignoring --endpoint");

	if (vm.count("trustedcert"))
		Log(LogWarning, "cli", "Master for Agent setup: Ignoring --trustedcert");

	/* Generate a new CA, if not already existing */

	Log(LogInformation, "cli", "Generating new CA.");

	if (PkiUtility::NewCa() > 0) {
		Log(LogWarning, "cli", "Found CA, skipping and using the existing one.");
	}

	/* Generate a self signed certificate */

	Log(LogInformation, "cli", "Generating new self-signed certificate.");

	String pki_path = PkiUtility::GetPkiPath();

	if (!Utility::MkDirP(pki_path, 0700)) {
		Log(LogCritical, "cli")
		    << "Could not create local pki directory '" << pki_path << "'.";
		return 1;
	}

	String cn = Utility::GetFQDN();

	if (vm.count("cn"))
		cn = vm["cn"].as<std::string>();

	String key = pki_path + "/" + cn + ".key";
	String csr = pki_path + "/" + cn + ".csr";

	if (PkiUtility::NewCert(cn, key, csr, "") > 0) {
		Log(LogCritical, "cli", "Failed to create self-signed certificate");
		return 1;
	}

	/* Sign the CSR with the CA key */

	String cert = pki_path + "/" + cn + ".crt";

	if (PkiUtility::SignCsr(csr, cert) != 0) {
		Log(LogCritical, "cli", "Could not sign CSR.");
		return 1;
	}

	/* Copy CA certificate to /etc/icinga2/pki */

	String ca = PkiUtility::GetLocalCaPath() + "/ca.crt";

	Log(LogInformation, "cli")
	    << "Copying CA certificate to '" << ca << "'.";

	String target_ca = pki_path + "/ca.crt";

	/* does not overwrite existing files! */
	Utility::CopyFile(ca, target_ca);

	/* read zones.conf and update with zone + endpoint information */

	Log(LogInformation, "cli", "Generating zone and object configuration.");

	AgentUtility::GenerateAgentMasterIcingaConfig(cn);

	/* enable the ApiListener config (verify its data) */

	Log(LogInformation, "cli", "Enabling the APIListener feature.");

	String api_path = FeatureUtility::GetFeaturesEnabledPath() + "/api.conf";
	//TODO: verify that the correct attributes are set on the ApiListener object
	//by reading the configuration (CompileFile) and fetching the object

	std::vector<std::string> enable;
	enable.push_back("api");
	FeatureUtility::EnableFeatures(enable);

	//TODO read --listen and set that as bind_host,port on ApiListener

	/* update constants.conf with NodeName = CN + TicketSalt = random value */
	if (cn != Utility::GetFQDN()) {
		Log(LogWarning, "cli")
			<< "CN '" << cn << "' does not match the default FQDN '" << Utility::GetFQDN() << "'. Requires update for NodeName constant in constants.conf!";
	}

	Log(LogInformation, "cli", "Updating constants.conf.");

	AgentUtility::CreateBackupFile(Application::GetSysconfDir() + "/icinga2/constants.conf");

	AgentUtility::UpdateConstant("NodeName", cn);

	String salt = RandomString(16);

	AgentUtility::UpdateConstant("TicketSalt", salt);

	Log(LogInformation, "cli")
	    << "Edit the api feature config file '" << api_path << "' and set a secure 'ticket_salt' attribute.";

	/* tell the user to reload icinga2 */

	Log(LogInformation, "cli", "Make sure to restart Icinga 2.");

	return 0;
}

int AgentSetupCommand::SetupAgent(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap)
{
	/* require ticket number (generated on master) and at least one endpoint */

	if (!vm.count("ticket")) {
		Log(LogCritical, "cli")
		    << "Please pass the ticket number generated on master\n"
		    << "(Hint: 'icinga2 pki ticket --cn <masterhost> --salt <ticket_salt>').";
		return 1;
	}

	if (!vm.count("endpoint")) {
		Log(LogCritical, "cli", "You need to specify at least one endpoint (--endpoint).");
		return 1;
	}

	String ticket = vm["ticket"].as<std::string>();

	Log(LogInformation, "cli")
	    << "Verifying ticket '" << ticket << "'.";

	/* require master host information for auto-signing requests */

	if (!vm.count("master_host")) {
		Log(LogCritical, "cli", "Please pass the master host connection information for auto-signing using '--master_host <host>'");
		return 1;
	}

	std::vector<String> tokens;
	boost::algorithm::split(tokens, vm["master_host"].as<std::string>(), boost::is_any_of(","));
	String master_host;
	String master_port = "5665";

	if (tokens.size() == 1 || tokens.size() == 2)
		master_host = tokens[0];

	if (tokens.size() == 2)
		master_port = tokens[1];

	Log(LogInformation, "cli")
	    << "Verifying master host connection information: host '" << master_host << "', port '" << master_port << "'.";

	/* trusted cert must be passed (retrieved by the user with 'pki save-cert' before) */

	if (!vm.count("trustedcert")) {
		Log(LogCritical, "cli")
		    << "Please pass the trusted cert retrieved from the master\n"
		    << "(Hint: 'icinga2 pki save-cert --host <masterhost> --port <5665> --key local.key --cert local.crt --trustedcert master.crt').";
		return 1;
	}

	String trustedcert = vm["trustedcert"].as<std::string>();

	Log(LogInformation, "cli")
	    << "Verifying trusted certificate from file '" << trustedcert << "'.";

	/* retrieve CN and pass it (defaults to FQDN) */

	String cn = Utility::GetFQDN();

	if (vm.count("cn"))
		cn = vm["cn"].as<std::string>();

	Log(LogInformation, "cli")
	    << "Using the following CN (defaults to FQDN): '" << cn << "'.";

	/* pki request a signed certificate from the master */

	String pki_path = PkiUtility::GetPkiPath();

	String key = pki_path + "/" + cn + ".key";
	String cert = pki_path + "/" + cn + ".crt";
	String ca = pki_path + "/ca.crt";

	if (PkiUtility::NewCert(cn, key, String(), cert) != 0) {
		Log(LogCritical, "cli", "Failed to generate new self-signed certificate.");
		return 1;
	}

	Log(LogInformation, "cli", "Requesting a signed certificate from the master.");

	if (PkiUtility::RequestCertificate(master_host, master_port, key, cert, ca, trustedcert, ticket) != 0) {
		Log(LogCritical, "cli", "Failed to request certificate from Icinga 2 master.");
		return 1;
	}

	/* enable the ApiListener config (verify its data) */

	Log(LogInformation, "cli", "Enabling the APIListener feature.");

	std::vector<std::string> enable;
	enable.push_back("api");
	FeatureUtility::EnableFeatures(enable);

	String api_path = FeatureUtility::GetFeaturesEnabledPath() + "/api.conf";
	//TODO: verify that the correct attributes are set on the ApiListener object
	//by reading the configuration (CompileFile) and fetching the object

	/*
        ConfigCompilerContext::GetInstance()->Reset();
        ConfigCompiler::CompileFile(api_path);

	DynamicType::Ptr dt = DynamicType::GetByName("ApiListener");

	BOOST_FOREACH(const DynamicObject::Ptr& object, dt->GetObjects()) {
		std::cout << JsonSerialize(object) << std::endl;
	}*/


	//TODO read --listen and set that as bind_host,port on ApiListener

	/* generate local zones.conf with zone+endpoint */

	Log(LogInformation, "cli", "Generating zone and object configuration.");

	AgentUtility::GenerateAgentIcingaConfig(vm["endpoint"].as<std::vector<std::string> >(), cn);

	/* update constants.conf with NodeName = CN */
	if (cn != Utility::GetFQDN()) {
		Log(LogWarning, "cli")
		    << "CN '" << cn << "' does not match the default FQDN '" << Utility::GetFQDN() << "'. Requires update for NodeName constant in constants.conf!";
	}

	Log(LogInformation, "cli", "Updating constants.conf.");

	AgentUtility::CreateBackupFile(Application::GetSysconfDir() + "/icinga2/constants.conf");

	AgentUtility::UpdateConstant("NodeName", cn);

	/* tell the user to reload icinga2 */

	Log(LogInformation, "cli", "Make sure to restart Icinga 2.");

	return 0;
}
