/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/nodesetupcommand.hpp"
#include "cli/nodeutility.hpp"
#include "cli/featureutility.hpp"
#include "cli/apisetuputility.hpp"
#include "remote/apilistener.hpp"
#include "remote/pkiutility.hpp"
#include "base/atomic-file.hpp"
#include "base/logger.hpp"
#include "base/console.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"
#include "base/scriptglobal.hpp"
#include "base/exception.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <iostream>
#include <fstream>
#include <vector>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("node/setup", NodeSetupCommand);

String NodeSetupCommand::GetDescription() const
{
	return "Sets up an Icinga 2 node.";
}

String NodeSetupCommand::GetShortDescription() const
{
	return "set up node";
}

void NodeSetupCommand::InitParameters(boost::program_options::options_description& visibleDesc,
	boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("zone", po::value<std::string>(), "The name of the local zone")
		("endpoint", po::value<std::vector<std::string> >(), "Connect to remote endpoint; syntax: cn[,host,port]")
		("parent_host", po::value<std::string>(), "The name of the parent host for auto-signing the csr; syntax: host[,port]")
		("parent_zone", po::value<std::string>(), "The name of the parent zone")
		("listen", po::value<std::string>(), "Listen on host,port")
		("ticket", po::value<std::string>(), "Generated ticket number for this request (optional)")
		("trustedcert", po::value<std::string>(), "Trusted parent certificate file as connection verification (received via 'pki save-cert')")
		("cn", po::value<std::string>(), "The certificate's common name")
		("accept-config", "Accept config from parent node")
		("accept-commands", "Accept commands from parent node")
		("master", "Use setup for a master instance")
		("global_zones", po::value<std::vector<std::string> >(), "The names of the additional global zones to 'global-templates' and 'director-global'.")
		("disable-confd", "Disables the conf.d directory during the setup");

	hiddenDesc.add_options()
		("master_zone", po::value<std::string>(), "DEPRECATED: The name of the master zone")
		("master_host", po::value<std::string>(), "DEPRECATED: The name of the master host for auto-signing the csr; syntax: host[,port]");
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

ImpersonationLevel NodeSetupCommand::GetImpersonationLevel() const
{
	return ImpersonateIcinga;
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
		return SetupMaster(vm);
	else
		return SetupNode(vm);
}

int NodeSetupCommand::SetupMaster(const boost::program_options::variables_map& vm)
{
	/* Ignore not required parameters */
	if (vm.count("ticket"))
		Log(LogWarning, "cli", "Master for Node setup: Ignoring --ticket");

	if (vm.count("endpoint"))
		Log(LogWarning, "cli", "Master for Node setup: Ignoring --endpoint");

	if (vm.count("trustedcert"))
		Log(LogWarning, "cli", "Master for Node setup: Ignoring --trustedcert");

	String cn = Utility::GetFQDN();

	if (vm.count("cn"))
		cn = vm["cn"].as<std::string>();

	/* Setup command hardcodes this as FQDN */
	String endpointName = cn;

	/* Allow to specify zone name. */
	String zoneName = "master";

	if (vm.count("zone"))
		zoneName = vm["zone"].as<std::string>();

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

	std::vector<String> globalZones { "global-templates", "director-global" };
	std::vector<std::string> setupGlobalZones;

	if (vm.count("global_zones"))
		setupGlobalZones = vm["global_zones"].as<std::vector<std::string> >();

	for (decltype(setupGlobalZones.size()) i = 0; i < setupGlobalZones.size(); i++) {
		if (std::find(globalZones.begin(), globalZones.end(), setupGlobalZones[i]) != globalZones.end()) {
			Log(LogCritical, "cli")
				<< "The global zone '" << setupGlobalZones[i] << "' is already specified.";
			return 1;
		}
	}

	globalZones.insert(globalZones.end(), setupGlobalZones.begin(), setupGlobalZones.end());

	/* Generate master configuration. */
	NodeUtility::GenerateNodeMasterIcingaConfig(endpointName, zoneName, globalZones);

	/* Update the ApiListener config. */
	Log(LogInformation, "cli", "Updating the APIListener feature.");

	String apipath = FeatureUtility::GetFeaturesAvailablePath() + "/api.conf";
	NodeUtility::CreateBackupFile(apipath);

	AtomicFile fp (apipath, 0644);

	fp << "/**\n"
		<< " * The API listener is used for distributed monitoring setups.\n"
		<< " */\n"
		<< "object ApiListener \"api\" {\n";

	if (vm.count("listen")) {
		std::vector<String> tokens = String(vm["listen"].as<std::string>()).Split(",");

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
		<< "  ticket_salt = TicketSalt\n"
		<< "}\n";

	fp.Commit();

	/* update constants.conf with NodeName = CN + TicketSalt = random value */
	if (endpointName != Utility::GetFQDN()) {
		Log(LogWarning, "cli")
			<< "CN/Endpoint name '" <<  endpointName << "' does not match the default FQDN '" << Utility::GetFQDN() << "'. Requires update for NodeName constant in constants.conf!";
	}

	NodeUtility::UpdateConstant("NodeName", endpointName);
	NodeUtility::UpdateConstant("ZoneName", zoneName);

	String salt = RandomString(16);

	NodeUtility::UpdateConstant("TicketSalt", salt);

	Log(LogInformation, "cli")
		<< "Edit the api feature config file '" << apipath << "' and set a secure 'ticket_salt' attribute.";

	if (vm.count("disable-confd")) {
		/* Disable conf.d inclusion */
		if (NodeUtility::UpdateConfiguration("\"conf.d\"", false, true)) {
			Log(LogInformation, "cli")
				<< "Disabled conf.d inclusion";
		} else {
			Log(LogWarning, "cli")
				<< "Tried to disable conf.d inclusion but failed, possibly it's already disabled.";
		}

		/* Include api-users.conf */
		String apiUsersFilePath = ApiSetupUtility::GetApiUsersConfPath();

		if (Utility::PathExists(apiUsersFilePath)) {
			NodeUtility::UpdateConfiguration("\"conf.d/api-users.conf\"", true, false);
		} else {
			Log(LogWarning, "cli")
				<< "Included file doesn't exist " << apiUsersFilePath;
		}
	}

	/* tell the user to reload icinga2 */
	Log(LogInformation, "cli", "Make sure to restart Icinga 2.");

	return 0;
}

int NodeSetupCommand::SetupNode(const boost::program_options::variables_map& vm)
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

	/* Deprecation warnings. TODO: Remove in 2.10.0. */
	if (vm.count("master_zone"))
		Log(LogWarning, "cli", "The 'master_zone' parameter has been deprecated. Use 'parent_zone' instead.");
	if (vm.count("master_host"))
		Log(LogWarning, "cli", "The 'master_host' parameter has been deprecated. Use 'parent_host' instead.");

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

	/* Decide whether to directly connect to the parent node for CSR signing, or leave it to the user. */
	bool connectToParent = false;
	String parentHost;
	String parentPort = "5665";
	std::shared_ptr<X509> trustedParentCert;

	/* TODO: remove master_host in 2.10.0. */
	if (!vm.count("master_host") && !vm.count("parent_host")) {
		connectToParent = false;

		Log(LogWarning, "cli")
			<< "Node to master/satellite connection setup skipped. Please configure your parent node to\n"
			<< "connect to this node by setting the 'host' attribute for the node Endpoint object.\n";
	} else {
		connectToParent = true;

		String parentHostInfo;

		if (vm.count("parent_host"))
			parentHostInfo = vm["parent_host"].as<std::string>();
		else if (vm.count("master_host")) /* TODO: Remove in 2.10.0. */
			parentHostInfo = vm["master_host"].as<std::string>();

		std::vector<String> tokens = parentHostInfo.Split(",");

		if (tokens.size() == 1 || tokens.size() == 2)
			parentHost = tokens[0];

		if (tokens.size() == 2)
			parentPort = tokens[1];

		Log(LogInformation, "cli")
			<< "Verifying parent host connection information: host '" << parentHost << "', port '" << parentPort << "'.";

	}

	/* retrieve CN and pass it (defaults to FQDN) */
	String cn = Utility::GetFQDN();

	if (vm.count("cn"))
		cn = vm["cn"].as<std::string>();

	Log(LogInformation, "cli")
		<< "Using the following CN (defaults to FQDN): '" << cn << "'.";

	/* pki request a signed certificate from the master */
	String certsDir = ApiListener::GetCertsDir();
	Utility::MkDirP(certsDir, 0700);

	String user = Configuration::RunAsUser;
	String group = Configuration::RunAsGroup;

	if (!Utility::SetFileOwnership(certsDir, user, group)) {
		Log(LogWarning, "cli")
			<< "Cannot set ownership for user '" << user << "' group '" << group << "' on file '" << certsDir << "'. Verify it yourself!";
	}

	String key = certsDir + "/" + cn + ".key";
	String cert = certsDir + "/" + cn + ".crt";
	String ca = certsDir + "/ca.crt";

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

	/* Send a signing request to the parent immediately, or leave it to the user. */
	if (connectToParent) {
		/* In contrast to `node wizard` the user must manually fetch
		 * the trustedParentCert to prove the trust relationship (fetched with 'pki save-cert').
		 */
		if (!vm.count("trustedcert")) {
			Log(LogCritical, "cli")
				<< "Please pass the trusted cert retrieved from the parent node (master or satellite)\n"
				<< "(Hint: 'icinga2 pki save-cert --host <parenthost> --port <5665> --key local.key --cert local.crt --trustedcert trusted-parent.crt').";
			return 1;
		}

		String trustedCert = vm["trustedcert"].as<std::string>();

		try{
			trustedParentCert = GetX509Certificate(trustedCert);
		} catch (const std::exception&) {
			Log(LogCritical, "cli")
				<< "Can't read trusted cert at '" << trustedCert << "'.";
			return 1;
		}

		try {
			if (IsCa(trustedParentCert)) {
				Log(LogCritical, "cli")
					<< "The trusted parent certificate is NOT a client certificate. It seems you passed the 'ca.crt' CA certificate via '--trustedcert' parameter.";
				return 1;
			}
		} catch (const std::exception&) {
			/* Swallow the error and do not run the check on unsupported OpenSSL platforms. */
		}

		Log(LogInformation, "cli")
			<< "Verifying trusted certificate file '" << vm["trustedcert"].as<std::string>() << "'.";

		Log(LogInformation, "cli", "Requesting a signed certificate from the parent Icinga node.");

		if (PkiUtility::RequestCertificate(parentHost, parentPort, key, cert, ca, trustedParentCert, ticket) > 0) {
			Log(LogCritical, "cli")
				<< "Failed to fetch signed certificate from parent Icinga node '"
				<< parentHost << ", "
				<< parentPort << "'. Please try again.";
			return 1;
		}
	} else {
		/* We cannot retrieve the parent certificate.
		 * Tell the user to manually copy the ca.crt file
		 * into DataDir + "/certs"
		 */
		Log(LogWarning, "cli")
			<< "\nNo connection to the parent node was specified.\n\n"
			<< "Please copy the public CA certificate from your master/satellite\n"
			<< "into '" << ca << "' before starting Icinga 2.\n";

		if (Utility::PathExists(ca)) {
			Log(LogInformation, "cli")
				<< "\nFound public CA certificate in '" << ca << "'.\n"
				<< "Please verify that it is the same as on your master/satellite.\n";
		}
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

	FeatureUtility::DisableFeatures({ "notification" });

	/* enable the ApiListener config */

	Log(LogInformation, "cli", "Updating the ApiListener feature.");

	FeatureUtility::EnableFeatures({ "api" });

	String apipath = FeatureUtility::GetFeaturesAvailablePath() + "/api.conf";
	NodeUtility::CreateBackupFile(apipath);

	AtomicFile fp (apipath, 0644);

	fp << "/**\n"
		<< " * The API listener is used for distributed monitoring setups.\n"
		<< " */\n"
		<< "object ApiListener \"api\" {\n";

	if (vm.count("listen")) {
		std::vector<String> tokens = String(vm["listen"].as<std::string>()).Split(",");

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

	fp.Commit();

	/* Generate zones configuration. */
	Log(LogInformation, "cli", "Generating zone and object configuration.");

	/* Setup command hardcodes this as FQDN */
	String endpointName = cn;

	/* Allow to specify zone name. */
	String zoneName = vm["zone"].as<std::string>();

	/* Allow to specify the parent zone name. */
	String parentZoneName = "master";

	if (vm.count("parent_zone"))
		parentZoneName = vm["parent_zone"].as<std::string>();

	std::vector<String> globalZones { "global-templates", "director-global" };
	std::vector<std::string> setupGlobalZones;

	if (vm.count("global_zones"))
		setupGlobalZones = vm["global_zones"].as<std::vector<std::string> >();

	for (decltype(setupGlobalZones.size()) i = 0; i < setupGlobalZones.size(); i++) {
		if (std::find(globalZones.begin(), globalZones.end(), setupGlobalZones[i]) != globalZones.end()) {
			Log(LogCritical, "cli")
				<< "The global zone '" << setupGlobalZones[i] << "' is already specified.";
			return 1;
		}
	}

	globalZones.insert(globalZones.end(), setupGlobalZones.begin(), setupGlobalZones.end());

	/* Generate node configuration. */
	NodeUtility::GenerateNodeIcingaConfig(endpointName, zoneName, parentZoneName, vm["endpoint"].as<std::vector<std::string> >(), globalZones);

	/* update constants.conf with NodeName = CN */
	if (endpointName != Utility::GetFQDN()) {
		Log(LogWarning, "cli")
			<< "CN/Endpoint name '" << endpointName << "' does not match the default FQDN '"
			<< Utility::GetFQDN() << "'. Requires an update for the NodeName constant in constants.conf!";
	}

	NodeUtility::UpdateConstant("NodeName", endpointName);
	NodeUtility::UpdateConstant("ZoneName", zoneName);

	if (!ticket.IsEmpty()) {
		String ticketPath = ApiListener::GetCertsDir() + "/ticket";
		AtomicFile af (ticketPath, 0600);

		if (!Utility::SetFileOwnership(af.GetTempFilename(), user, group)) {
			Log(LogWarning, "cli")
				<< "Cannot set ownership for user '" << user
				<< "' group '" << group
				<< "' on file '" << ticketPath << "'. Verify it yourself!";
		}

		af << ticket;
		af.Commit();
	}

	/* If no parent connection was made, the user must supply the ca.crt before restarting Icinga 2.*/
	if (!connectToParent) {
		Log(LogWarning, "cli")
			<< "No connection to the parent node was specified.\n\n"
			<< "Please copy the public CA certificate from your master/satellite\n"
			<< "into '" << ca << "' before starting Icinga 2.\n";
	} else {
		Log(LogInformation, "cli", "Make sure to restart Icinga 2.");
	}

	if (vm.count("disable-confd")) {
		/* Disable conf.d inclusion */
		NodeUtility::UpdateConfiguration("\"conf.d\"", false, true);
	}

	/* tell the user to reload icinga2 */
	Log(LogInformation, "cli", "Make sure to restart Icinga 2.");

	return 0;
}
