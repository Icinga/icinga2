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

#include "cli/nodesetupcommand.hpp"
#include "cli/nodeutility.hpp"
#include "cli/featureutility.hpp"
#include "cli/apisetuputility.hpp"
#include "remote/apilistener.hpp"
#include "remote/pkiutility.hpp"
#include "base/logger.hpp"
#include "base/console.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"
#include "base/scriptglobal.hpp"
#include "base/exception.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>

#include <iostream>
#include <fstream>
#include <vector>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("node/setup", NodeSetupCommand);

String NodeSetupCommand::GetDescription(void) const
{
	return "Sets up an Icinga 2 node.";
}

String NodeSetupCommand::GetShortDescription(void) const
{
	return "set up node";
}

void NodeSetupCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("zone", po::value<std::string>(), "The name of the local zone")
		("master_host", po::value<std::string>(), "The name of the master host for auto-signing the csr; syntax: host[,port]")
		("endpoint", po::value<std::vector<std::string> >(), "Connect to remote endpoint; syntax: cn[,host,port]")
		("listen", po::value<std::string>(), "Listen on host,port")
		("ticket", po::value<std::string>(), "Generated ticket number for this request (optional)")
		("trustedcert", po::value<std::string>(), "Trusted master certificate file")
		("cn", po::value<std::string>(), "The certificate's common name")
		("accept-config", "Accept config from master")
		("accept-commands", "Accept commands from master")
		("master", "Use setup for a master instance");

	hiddenDesc.add_options()
		("master_zone", po::value<std::string>(), "The name of the master zone");
}

std::vector<String> NodeSetupCommand::GetArgumentSuggestions(const String& argument, const String& word) const
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

ImpersonationLevel NodeSetupCommand::GetImpersonationLevel(void) const
{
	return ImpersonateRoot;
}

/**
 * The entry point for the "node setup" CLI command.
 *
 * @returns An exit status.
 */
int NodeSetupCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	if (!ap.empty()) {
		Log(LogWarning, "cli")
		    << "Ignoring parameters: " << boost::algorithm::join(ap, " ");
	}

	if (vm.count("master"))
		return SetupMaster(vm, ap);
	else
		return SetupNode(vm, ap);
}

int NodeSetupCommand::SetupMaster(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap)
{
	/* Ignore not required parameters */
	if (vm.count("ticket"))
		Log(LogWarning, "cli", "Master for Node setup: Ignoring --ticket");

	if (vm.count("endpoint"))
		Log(LogWarning, "cli", "Master for Node setup: Ignoring --endpoint");

	if (vm.count("trustedcert"))
		Log(LogWarning, "cli", "Master for Node setup: Ignoring --trustedcert");

	if (vm.count("accept-config"))
		Log(LogWarning, "cli", "Master for Node setup: Ignoring --accept-config");

	if (vm.count("accept-commands"))
		Log(LogWarning, "cli", "Master for Node setup: Ignoring --accept-commands");

        String cn = Utility::GetFQDN();

        if (vm.count("cn"))
                cn = vm["cn"].as<std::string>();

	/* check whether the user wants to generate a new certificate or not */
	String existingPath = ApiListener::GetCertsDir() + "/" + cn + ".crt";

	Log(LogInformation, "cli")
	    << "Checking in existing certificates for common name '" << cn << "'...";

	if (Utility::PathExists(existingPath)) {
		Log(LogWarning, "cli")
		    << "Certificate '" << existingPath << "' for CN '" << cn << "' already exists. Not generating new certificate.";
	} else {
		Log(LogInformation, "cli")
		    << "Certificates not yet generated. Running 'api setup' now.";

		ApiSetupUtility::SetupMasterCertificates(cn);
	}

	Log(LogInformation, "cli", "Generating master configuration for Icinga 2.");
	ApiSetupUtility::SetupMasterApiUser();

	if (!FeatureUtility::CheckFeatureEnabled("api")) {
		ApiSetupUtility::SetupMasterEnableApi();
	} else {
		Log(LogInformation, "cli")
		    << "'api' feature already enabled.\n";
	}

	/* write zones.conf and update with zone + endpoint information */
	Log(LogInformation, "cli", "Generating zone and object configuration.");

	std::vector<String> globalZones;

	globalZones.push_back("global-templates");
	globalZones.push_back("director-global");

	NodeUtility::GenerateNodeMasterIcingaConfig(globalZones);

	/* update the ApiListener config - SetupMaster() will always enable it */
	Log(LogInformation, "cli", "Updating the APIListener feature.");

	String apipath = FeatureUtility::GetFeaturesAvailablePath() + "/api.conf";
	NodeUtility::CreateBackupFile(apipath);

	std::fstream fp;
	String tempApiPath = Utility::CreateTempFile(apipath + ".XXXXXX", 0644, fp);

	fp << "/**\n"
	    << " * The API listener is used for distributed monitoring setups.\n"
	    << " */\n"
	    << "object ApiListener \"api\" {\n";

	if (vm.count("listen")) {
		std::vector<String> tokens;
		boost::algorithm::split(tokens, vm["listen"].as<std::string>(), boost::is_any_of(","));

		if (tokens.size() > 0)
			fp << "  bind_host = \"" << tokens[0] << "\"\n";
		if (tokens.size() > 1)
			fp << "  bind_port = " << tokens[1] << "\n";
	}

	fp << "\n"
	    << "  ticket_salt = TicketSalt\n"
	    << "}\n";

	fp.close();

#ifdef _WIN32
	_unlink(apipath.CStr());
#endif /* _WIN32 */

	if (rename(tempApiPath.CStr(), apipath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(tempApiPath));
	}

	/* update constants.conf with NodeName = CN + TicketSalt = random value */
	if (cn != Utility::GetFQDN()) {
		Log(LogWarning, "cli")
			<< "CN '" << cn << "' does not match the default FQDN '" << Utility::GetFQDN() << "'. Requires update for NodeName constant in constants.conf!";
	}

	Log(LogInformation, "cli", "Updating constants.conf.");

	NodeUtility::CreateBackupFile(Application::GetSysconfDir() + "/icinga2/constants.conf");

	NodeUtility::UpdateConstant("NodeName", cn);
	NodeUtility::UpdateConstant("ZoneName", cn);

	String salt = RandomString(16);

	NodeUtility::UpdateConstant("TicketSalt", salt);

	Log(LogInformation, "cli")
	    << "Edit the api feature config file '" << apipath << "' and set a secure 'ticket_salt' attribute.";

	/* tell the user to reload icinga2 */

	Log(LogInformation, "cli", "Make sure to restart Icinga 2.");

	return 0;
}

int NodeSetupCommand::SetupNode(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap)
{
	/* require at least one endpoint. Ticket is optional. */
	if (!vm.count("endpoint")) {
		Log(LogCritical, "cli", "You need to specify at least one endpoint (--endpoint).");
		return 1;
	}

	if (!vm.count("zone")) {
		Log(LogCritical, "cli", "You need to specify the local zone (--zone).");
		return 1;
	}

	String ticket;

	if (vm.count("ticket"))
		ticket = vm["ticket"].as<std::string>();

	if (ticket.IsEmpty()) {
		Log(LogInformation, "cli")
		    << "Requesting certificate without a ticket.";
	} else {
		Log(LogInformation, "cli")
		    << "Requesting certificate with ticket '" << ticket << "'.";
	}

	/* require master host information for auto-signing requests */

	if (!vm.count("master_host")) {
		Log(LogCritical, "cli", "Please pass the master host connection information for auto-signing using '--master_host <host>'. This can also be a direct parent satellite since 2.8.");

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
	    << "Verifying parent host connection information: host '" << master_host << "', port '" << master_port << "'.";

	/* trusted cert must be passed (retrieved by the user with 'pki save-cert' before) */

	if (!vm.count("trustedcert")) {
		Log(LogCritical, "cli")
		    << "Please pass the trusted cert retrieved from the parent node (master or satellite)\n"
		    << "(Hint: 'icinga2 pki save-cert --host <masterhost> --port <5665> --key local.key --cert local.crt --trustedcert master.crt').";
		return 1;
	}

	std::shared_ptr<X509> trustedcert = GetX509Certificate(vm["trustedcert"].as<std::string>());

	Log(LogInformation, "cli")
	    << "Verifying trusted certificate file '" << vm["trustedcert"].as<std::string>() << "'.";

	/* retrieve CN and pass it (defaults to FQDN) */
	String cn = Utility::GetFQDN();

	if (vm.count("cn"))
		cn = vm["cn"].as<std::string>();

	Log(LogInformation, "cli")
	    << "Using the following CN (defaults to FQDN): '" << cn << "'.";

	/* pki request a signed certificate from the master */

	String pki_path = ApiListener::GetCertsDir();
	Utility::MkDirP(pki_path, 0700);

	String user = ScriptGlobal::Get("RunAsUser");
	String group = ScriptGlobal::Get("RunAsGroup");

	if (!Utility::SetFileOwnership(pki_path, user, group)) {
		Log(LogWarning, "cli")
		    << "Cannot set ownership for user '" << user << "' group '" << group << "' on file '" << pki_path << "'. Verify it yourself!";
	}

	String key = pki_path + "/" + cn + ".key";
	String cert = pki_path + "/" + cn + ".crt";
	String ca = pki_path + "/ca.crt";

	if (Utility::PathExists(key))
		NodeUtility::CreateBackupFile(key, true);
	if (Utility::PathExists(cert))
		NodeUtility::CreateBackupFile(cert);

	if (PkiUtility::NewCert(cn, key, String(), cert) != 0) {
		Log(LogCritical, "cli", "Failed to generate new self-signed certificate.");
		return 1;
	}

	/* fix permissions: root -> icinga daemon user */
	if (!Utility::SetFileOwnership(key, user, group)) {
		Log(LogWarning, "cli")
		    << "Cannot set ownership for user '" << user << "' group '" << group << "' on file '" << key << "'. Verify it yourself!";
	}

	Log(LogInformation, "cli", "Requesting a signed certificate from the parent Icinga node.");

	if (PkiUtility::RequestCertificate(master_host, master_port, key, cert, ca, trustedcert, ticket) > 0) {
		Log(LogCritical, "cli")
		    << "Failed to fetch signed certificate from parent Icinga node '"
		    << master_host << ", "
		    << master_port << "'. Please try again.";
		return 1;
	}

	if (!Utility::SetFileOwnership(ca, user, group)) {
		Log(LogWarning, "cli")
		    << "Cannot set ownership for user '" << user << "' group '" << group << "' on file '" << ca << "'. Verify it yourself!";
	}

	/* fix permissions (again) when updating the signed certificate */
	if (!Utility::SetFileOwnership(cert, user, group)) {
		Log(LogWarning, "cli")
		    << "Cannot set ownership for user '" << user << "' group '" << group << "' on file '" << cert << "'. Verify it yourself!";
	}

	/* disable the notifications feature */
	Log(LogInformation, "cli", "Disabling the Notification feature.");

	std::vector<std::string> disable;
	disable.push_back("notification");
	FeatureUtility::DisableFeatures(disable);

	/* enable the ApiListener config */

	Log(LogInformation, "cli", "Updating the ApiListener feature.");

	std::vector<std::string> enable;
	enable.push_back("api");
	FeatureUtility::EnableFeatures(enable);

	String apipath = FeatureUtility::GetFeaturesAvailablePath() + "/api.conf";
	NodeUtility::CreateBackupFile(apipath);

	std::fstream fp;
	String tempApiPath = Utility::CreateTempFile(apipath + ".XXXXXX", 0644, fp);

	fp << "/**\n"
	    << " * The API listener is used for distributed monitoring setups.\n"
	    << " */\n"
	    << "object ApiListener \"api\" {\n";

	if (vm.count("listen")) {
		std::vector<String> tokens;
		boost::algorithm::split(tokens, vm["listen"].as<std::string>(), boost::is_any_of(","));

		if (tokens.size() > 0)
			fp << "  bind_host = \"" << tokens[0] << "\"\n";
		if (tokens.size() > 1)
			fp << "  bind_port = " << tokens[1] << "\n";
	}

	fp << "\n";

	if (vm.count("accept-config"))
		fp << "  accept_config = true\n";
	else
		fp << "  accept_config = false\n";

	if (vm.count("accept-commands"))
		fp << "  accept_commands = true\n";
	else
		fp << "  accept_commands = false\n";

	fp << "\n"
	    << "}\n";

	fp.close();

#ifdef _WIN32
	_unlink(apipath.CStr());
#endif /* _WIN32 */

	if (rename(tempApiPath.CStr(), apipath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(tempApiPath));
	}

	/* generate local zones.conf with zone+endpoint */

	Log(LogInformation, "cli", "Generating zone and object configuration.");

	std::vector<String> globalZones;

	globalZones.push_back("global-templates");
	globalZones.push_back("director-global");

	NodeUtility::GenerateNodeIcingaConfig(vm["endpoint"].as<std::vector<std::string> >(), globalZones);

	/* update constants.conf with NodeName = CN */
	if (cn != Utility::GetFQDN()) {
		Log(LogWarning, "cli")
		    << "CN '" << cn << "' does not match the default FQDN '" << Utility::GetFQDN() << "'. Requires an update for the NodeName constant in constants.conf!";
	}

	Log(LogInformation, "cli", "Updating constants.conf.");

	NodeUtility::CreateBackupFile(Application::GetSysconfDir() + "/icinga2/constants.conf");

	NodeUtility::UpdateConstant("NodeName", cn);
	NodeUtility::UpdateConstant("ZoneName", vm["zone"].as<std::string>());

	if (!ticket.IsEmpty()) {
		String ticketPath = ApiListener::GetCertsDir() + "/ticket";

		String tempTicketPath = Utility::CreateTempFile(ticketPath + ".XXXXXX", 0600, fp);

		if (!Utility::SetFileOwnership(tempTicketPath, user, group)) {
			Log(LogWarning, "cli")
			    << "Cannot set ownership for user '" << user
			    << "' group '" << group
			    << "' on file '" << tempTicketPath << "'. Verify it yourself!";
		}

		fp << ticket;

		fp.close();

#ifdef _WIN32
		_unlink(ticketPath.CStr());
#endif /* _WIN32 */

		if (rename(tempTicketPath.CStr(), ticketPath.CStr()) < 0) {
			BOOST_THROW_EXCEPTION(posix_error()
			    << boost::errinfo_api_function("rename")
			    << boost::errinfo_errno(errno)
			    << boost::errinfo_file_name(tempTicketPath));
		}
	}

	/* tell the user to reload icinga2 */
	Log(LogInformation, "cli", "Make sure to restart Icinga 2.");

	return 0;
}
