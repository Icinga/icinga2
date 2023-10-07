/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/nodewizardcommand.hpp"
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
#include <boost/algorithm/string/case_conv.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("node/wizard", NodeWizardCommand);

String NodeWizardCommand::GetDescription() const
{
	return "Wizard for Icinga 2 node setup.";
}

String NodeWizardCommand::GetShortDescription() const
{
	return "wizard for node setup";
}

ImpersonationLevel NodeWizardCommand::GetImpersonationLevel() const
{
	return ImpersonateIcinga;
}

int NodeWizardCommand::GetMaxArguments() const
{
	return -1;
}

void NodeWizardCommand::InitParameters(boost::program_options::options_description& visibleDesc,
	boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("verbose", "increase log level");
}

/**
 * The entry point for the "node wizard" CLI command.
 *
 * @returns An exit status.
 */
int NodeWizardCommand::Run(const boost::program_options::variables_map& vm,
	const std::vector<std::string>& ap) const
{
	if (!vm.count("verbose"))
		Logger::SetConsoleLogSeverity(LogCritical);

	/*
	 * The wizard will get all information from the user,
	 * and then call all required functions.
	 */

	std::cout << ConsoleColorTag(Console_Bold | Console_ForegroundBlue)
		<< "Welcome to the Icinga 2 Setup Wizard!\n"
		<< "\n"
		<< "We will guide you through all required configuration details.\n"
		<< "\n"
		<< ConsoleColorTag(Console_Normal);

	/* 0. master or node setup?
	 * 1. Ticket
	 * 2. Master information for autosigning
	 * 3. Trusted cert location
	 * 4. CN to use (defaults to FQDN)
	 * 5. Local CA
	 * 6. New self signed certificate
	 * 7. Request signed certificate from master
	 * 8. copy key information to /var/lib/icinga2/certs
	 * 9. enable ApiListener feature
	 * 10. generate zones.conf with endpoints and zone objects
	 * 11. set NodeName = cn and ZoneName in constants.conf
	 * 12. disable conf.d directory?
	 * 13. reload icinga2, or tell the user to
	 */

	std::string answer;
	/* master or satellite/agent setup */
	std::cout << ConsoleColorTag(Console_Bold)
		<< "Please specify if this is an agent/satellite setup "
		<< "('n' installs a master setup)" << ConsoleColorTag(Console_Normal)
		<< " [Y/n]: ";
	std::getline (std::cin, answer);

	boost::algorithm::to_lower(answer);

	String choice = answer;

	std::cout << "\n";

	int res = 0;

	if (choice.Contains("n"))
		res = MasterSetup();
	else
		res = AgentSatelliteSetup();

	if (res != 0)
		return res;

	std::cout << "\n";
	std::cout << ConsoleColorTag(Console_Bold | Console_ForegroundGreen)
		<< "Done.\n\n"
		<< ConsoleColorTag(Console_Normal);

	std::cout << ConsoleColorTag(Console_Bold | Console_ForegroundRed)
		<< "Now restart your Icinga 2 daemon to finish the installation!\n"
		<< ConsoleColorTag(Console_Normal);

	return 0;
}

int NodeWizardCommand::AgentSatelliteSetup() const
{
	std::string answer;
	String choice;
	bool connectToParent = false;

	std::cout << "Starting the Agent/Satellite setup routine...\n\n";

	/* CN */
	std::cout << ConsoleColorTag(Console_Bold)
		<< "Please specify the common name (CN)"
		<< ConsoleColorTag(Console_Normal)
		<< " [" << Utility::GetFQDN() << "]: ";

	std::getline(std::cin, answer);

	if (answer.empty())
		answer = Utility::GetFQDN();

	String cn = answer;
	cn = cn.Trim();

	std::vector<std::string> endpoints;

	String endpointBuffer;

	std::cout << ConsoleColorTag(Console_Bold)
		<< "\nPlease specify the parent endpoint(s) (master or satellite) where this node should connect to:"
		<< ConsoleColorTag(Console_Normal) << "\n";
	String parentEndpointName;

wizard_endpoint_loop_start:

	std::cout << ConsoleColorTag(Console_Bold)
		<< "Master/Satellite Common Name" << ConsoleColorTag(Console_Normal)
		<< " (CN from your master/satellite node): ";

	std::getline(std::cin, answer);

	if (answer.empty()) {
		Log(LogWarning, "cli", "Master/Satellite CN is required! Please retry.");
		goto wizard_endpoint_loop_start;
	}

	endpointBuffer = answer;
	endpointBuffer = endpointBuffer.Trim();

	std::cout << "\nDo you want to establish a connection to the parent node "
		<< ConsoleColorTag(Console_Bold) << "from this node?"
		<< ConsoleColorTag(Console_Normal) << " [Y/n]: ";

	std::getline (std::cin, answer);
	boost::algorithm::to_lower(answer);
	choice = answer;

	String parentEndpointPort = "5665";

	if (choice.Contains("n")) {
		connectToParent = false;

		Log(LogWarning, "cli", "Node to master/satellite connection setup skipped");
		std::cout << "Connection setup skipped. Please configure your parent node to\n"
			<< "connect to this node by setting the 'host' attribute for the node Endpoint object.\n";

	} else  {
		connectToParent = true;

		std::cout << ConsoleColorTag(Console_Bold)
			<< "Please specify the master/satellite connection information:"
			<< ConsoleColorTag(Console_Normal) << "\n"
			<< ConsoleColorTag(Console_Bold) << "Master/Satellite endpoint host"
			<< ConsoleColorTag(Console_Normal) << " (IP address or FQDN): ";

		std::getline(std::cin, answer);

		if (answer.empty()) {
			Log(LogWarning, "cli", "Please enter the parent endpoint (master/satellite) connection information.");
			goto wizard_endpoint_loop_start;
		}

		String tmp = answer;
		tmp = tmp.Trim();

		endpointBuffer += "," + tmp;
		parentEndpointName = tmp;

		std::cout << ConsoleColorTag(Console_Bold)
			<< "Master/Satellite endpoint port" << ConsoleColorTag(Console_Normal)
			<< " [" << parentEndpointPort << "]: ";

		std::getline(std::cin, answer);

		if (!answer.empty())
			parentEndpointPort = answer;

		endpointBuffer += "," + parentEndpointPort.Trim();
	}

	endpoints.push_back(endpointBuffer);

	std::cout << ConsoleColorTag(Console_Bold) << "\nAdd more master/satellite endpoints?"
		<< ConsoleColorTag(Console_Normal) << " [y/N]: ";
	std::getline (std::cin, answer);

	boost::algorithm::to_lower(answer);

	choice = answer;

	if (choice.Contains("y"))
		goto wizard_endpoint_loop_start;

	/* Extract parent node information. */
	String parentHost, parentPort;

	for (String endpoint : endpoints) {
		std::vector<String> tokens = endpoint.Split(",");

		if (tokens.size() > 1)
			parentHost = tokens[1];

		if (tokens.size() > 2)
			parentPort = tokens[2];
	}

	/* workaround for fetching the master cert */
	String certsDir = ApiListener::GetCertsDir();
	Utility::MkDirP(certsDir, 0700);

	String user = Configuration::RunAsUser;
	String group = Configuration::RunAsGroup;

	if (!Utility::SetFileOwnership(certsDir, user, group)) {
		Log(LogWarning, "cli")
			<< "Cannot set ownership for user '" << user
			<< "' group '" << group
			<< "' on file '" << certsDir << "'. Verify it yourself!";
	}

	String nodeCert = certsDir + "/" + cn + ".crt";
	String nodeKey = certsDir + "/" + cn + ".key";

	if (Utility::PathExists(nodeKey))
		NodeUtility::CreateBackupFile(nodeKey, true);
	if (Utility::PathExists(nodeCert))
		NodeUtility::CreateBackupFile(nodeCert);

	if (PkiUtility::NewCert(cn, nodeKey, Empty, nodeCert) > 0) {
		Log(LogCritical, "cli")
			<< "Failed to create new self-signed certificate for CN '"
			<< cn << "'. Please try again.";
		return 1;
	}

	/* fix permissions: root -> icinga daemon user */
	if (!Utility::SetFileOwnership(nodeKey, user, group)) {
		Log(LogWarning, "cli")
			<< "Cannot set ownership for user '" << user
			<< "' group '" << group
			<< "' on file '" << nodeKey << "'. Verify it yourself!";
	}

	std::shared_ptr<X509> trustedParentCert;

	/* Check whether we should connect to the parent node and present its trusted certificate. */
	if (connectToParent) {
		//save-cert and store the master certificate somewhere
		Log(LogInformation, "cli")
			<< "Fetching public certificate from master ("
			<< parentHost << ", " << parentPort << "):\n";

		trustedParentCert = PkiUtility::FetchCert(parentHost, parentPort);
		if (!trustedParentCert) {
			Log(LogCritical, "cli", "Peer did not present a valid certificate.");
			return 1;
		}

		std::cout << ConsoleColorTag(Console_Bold) << "Parent certificate information:\n"
			<< ConsoleColorTag(Console_Normal) << PkiUtility::GetCertificateInformation(trustedParentCert)
			<< ConsoleColorTag(Console_Bold) << "\nIs this information correct?"
			<< ConsoleColorTag(Console_Normal) << " [y/N]: ";

		std::getline (std::cin, answer);
		boost::algorithm::to_lower(answer);
		if (answer != "y") {
			Log(LogWarning, "cli", "Process aborted.");
			return 1;
		}

		Log(LogInformation, "cli", "Received trusted parent certificate.\n");
	}

wizard_ticket:
	String nodeCA = certsDir + "/ca.crt";
	String ticket;

	/* Check whether we can connect to the parent node and fetch the client and CA certificate. */
	if (connectToParent) {
		std::cout << ConsoleColorTag(Console_Bold)
			<< "\nPlease specify the request ticket generated on your Icinga 2 master "
			<< ConsoleColorTag(Console_Normal) << "(optional)"
			<< ConsoleColorTag(Console_Bold) << "."
			<< ConsoleColorTag(Console_Normal) << "\n"
			<< " (Hint: # icinga2 pki ticket --cn '" << cn << "'): ";

		std::getline(std::cin, answer);

		if (answer.empty()) {
			std::cout << ConsoleColorTag(Console_Bold) << "\n"
				<< "No ticket was specified. Please approve the certificate signing request manually\n"
				<< "on the master (see 'icinga2 ca list' and 'icinga2 ca sign --help' for details)."
				<< ConsoleColorTag(Console_Normal) << "\n";
		}

		ticket = answer;
		ticket = ticket.Trim();

		if (ticket.IsEmpty()) {
			Log(LogInformation, "cli")
				<< "Requesting certificate without a ticket.";
		} else {
			Log(LogInformation, "cli")
				<< "Requesting certificate with ticket '" << ticket << "'.";
		}

		if (Utility::PathExists(nodeCA))
			NodeUtility::CreateBackupFile(nodeCA);
		if (Utility::PathExists(nodeCert))
			NodeUtility::CreateBackupFile(nodeCert);

		if (PkiUtility::RequestCertificate(parentHost, parentPort, nodeKey,
			nodeCert, nodeCA, trustedParentCert, ticket) > 0) {
			Log(LogCritical, "cli")
				<< "Failed to fetch signed certificate from master '"
				<< parentHost << ", "
				<< parentPort << "'. Please try again.";
			goto wizard_ticket;
		}

		/* fix permissions (again) when updating the signed certificate */
		if (!Utility::SetFileOwnership(nodeCert, user, group)) {
			Log(LogWarning, "cli")
				<< "Cannot set ownership for user '" << user
				<< "' group '" << group << "' on file '"
				<< nodeCert << "'. Verify it yourself!";
		}
	} else {
		/* We cannot retrieve the parent certificate.
		 * Tell the user to manually copy the ca.crt file
		 * into DataDir + "/certs"
		 */

		std::cout <<  ConsoleColorTag(Console_Bold)
			<< "\nNo connection to the parent node was specified.\n\n"
			<< "Please copy the public CA certificate from your master/satellite\n"
			<< "into '" << nodeCA << "' before starting Icinga 2.\n"
			<< ConsoleColorTag(Console_Normal);

		if (Utility::PathExists(nodeCA)) {
			std::cout <<  ConsoleColorTag(Console_Bold)
				<< "\nFound public CA certificate in '" << nodeCA << "'.\n"
				<< "Please verify that it is the same as on your master/satellite.\n"
				<< ConsoleColorTag(Console_Normal);
		}

	}

	/* apilistener config */
	std::cout << ConsoleColorTag(Console_Bold)
		<< "Please specify the API bind host/port "
		<< ConsoleColorTag(Console_Normal) << "(optional)"
		<< ConsoleColorTag(Console_Bold) << ":\n";

	std::cout << ConsoleColorTag(Console_Bold)
		<< "Bind Host" << ConsoleColorTag(Console_Normal) << " []: ";

	std::getline(std::cin, answer);

	String bindHost = answer;
	bindHost = bindHost.Trim();

	std::cout << ConsoleColorTag(Console_Bold)
		<< "Bind Port" << ConsoleColorTag(Console_Normal) << " []: ";

	std::getline(std::cin, answer);

	String bindPort = answer;
	bindPort = bindPort.Trim();

	std::cout << ConsoleColorTag(Console_Bold) << "\n"
		<< "Accept config from parent node?" << ConsoleColorTag(Console_Normal)
		<< " [y/N]: ";
	std::getline(std::cin, answer);
	boost::algorithm::to_lower(answer);
	choice = answer;

	String acceptConfig = choice.Contains("y") ? "true" : "false";

	std::cout << ConsoleColorTag(Console_Bold)
		<< "Accept commands from parent node?" << ConsoleColorTag(Console_Normal)
		<< " [y/N]: ";
	std::getline(std::cin, answer);
	boost::algorithm::to_lower(answer);
	choice = answer;

	String acceptCommands = choice.Contains("y") ? "true" : "false";

	std::cout << "\n";

	std::cout << ConsoleColorTag(Console_Bold | Console_ForegroundGreen)
		<< "Reconfiguring Icinga...\n"
		<< ConsoleColorTag(Console_Normal);

	/* disable the notifications feature on agent/satellite nodes */
	Log(LogInformation, "cli", "Disabling the Notification feature.");

	FeatureUtility::DisableFeatures({ "notification" });

	Log(LogInformation, "cli", "Enabling the ApiListener feature.");

	FeatureUtility::EnableFeatures({ "api" });

	String apiConfPath = FeatureUtility::GetFeaturesAvailablePath() + "/api.conf";
	NodeUtility::CreateBackupFile(apiConfPath);

	AtomicFile fp (apiConfPath, 0644);

	fp << "/**\n"
		<< " * The API listener is used for distributed monitoring setups.\n"
		<< " */\n"
		<< "object ApiListener \"api\" {\n"
		<< "  accept_config = " << acceptConfig << "\n"
		<< "  accept_commands = " << acceptCommands << "\n";

	if (!bindHost.IsEmpty())
		fp << "  bind_host = \"" << bindHost << "\"\n";
	if (!bindPort.IsEmpty())
		fp << "  bind_port = " << bindPort << "\n";

	fp << "}\n";

	fp.Commit();

	/* Zones configuration. */
	Log(LogInformation, "cli", "Generating local zones.conf.");

	/* Setup command hardcodes this as FQDN */
	String endpointName = cn;

	/* Different local zone name. */
	std::cout << "\nLocal zone name [" + endpointName + "]: ";
	std::getline(std::cin, answer);

	if (answer.empty())
		answer = endpointName;

	String zoneName = answer;
	zoneName = zoneName.Trim();

	/* Different parent zone name. */
	std::cout << "Parent zone name [master]: ";
	std::getline(std::cin, answer);

	if (answer.empty())
		answer = "master";

	String parentZoneName = answer;
	parentZoneName = parentZoneName.Trim();

	/* Global zones. */
	std::vector<String> globalZones { "global-templates", "director-global" };

	std::cout << "\nDefault global zones: " << boost::algorithm::join(globalZones, " ");
	std::cout << "\nDo you want to specify additional global zones? [y/N]: ";

	std::getline(std::cin, answer);
	boost::algorithm::to_lower(answer);
	choice = answer;

wizard_global_zone_loop_start:
	if (choice.Contains("y")) {
		std::cout << "\nPlease specify the name of the global Zone: ";

		std::getline(std::cin, answer);

		if (answer.empty()) {
			std::cout << "\nName of the global Zone is required! Please retry.";
			goto wizard_global_zone_loop_start;
		}

		String globalZoneName = answer;
		globalZoneName = globalZoneName.Trim();

		if (std::find(globalZones.begin(), globalZones.end(), globalZoneName) != globalZones.end()) {
			std::cout << "The global zone '" << globalZoneName << "' is already specified."
				<< " Please retry.";
			goto wizard_global_zone_loop_start;
		}

		globalZones.push_back(globalZoneName);

		std::cout << "\nDo you want to specify another global zone? [y/N]: ";

		std::getline(std::cin, answer);
		boost::algorithm::to_lower(answer);
		choice = answer;

		if (choice.Contains("y"))
			goto wizard_global_zone_loop_start;
	} else
		Log(LogInformation, "cli", "No additional global Zones have been specified");

	/* Generate node configuration. */
	NodeUtility::GenerateNodeIcingaConfig(endpointName, zoneName, parentZoneName, endpoints, globalZones);

	if (endpointName != Utility::GetFQDN()) {
		Log(LogWarning, "cli")
			<< "CN/Endpoint name '" << endpointName << "' does not match the default FQDN '"
			<< Utility::GetFQDN() << "'. Requires update for NodeName constant in constants.conf!";
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
			<< "into '" << nodeCA << "' before starting Icinga 2.\n";
	} else {
		Log(LogInformation, "cli", "Make sure to restart Icinga 2.");
	}

	/* Disable conf.d inclusion */
	std::cout << "\nDo you want to disable the inclusion of the conf.d directory [Y/n]: ";

	std::getline(std::cin, answer);
	boost::algorithm::to_lower(answer);
	choice = answer;

	if (choice.Contains("n"))
		Log(LogInformation, "cli")
			<< "conf.d directory has not been disabled.";
	else {
		std::cout << ConsoleColorTag(Console_Bold | Console_ForegroundGreen)
			<< "Disabling the inclusion of the conf.d directory...\n"
			<< ConsoleColorTag(Console_Normal);

		if (!NodeUtility::UpdateConfiguration("\"conf.d\"", false, true)) {
			std::cout << ConsoleColorTag(Console_Bold | Console_ForegroundRed)
				<< "Failed to disable the conf.d inclusion, it may already have been disabled.\n"
				<< ConsoleColorTag(Console_Normal);
		}

		/* Satellite/Agents should not include the api-users.conf file.
		 * The configuration should instead be managed via config sync or automation tools.
		 */
	}

	return 0;
}

int NodeWizardCommand::MasterSetup() const
{
	std::string answer;
	String choice;

	std::cout << ConsoleColorTag(Console_Bold) << "Starting the Master setup routine...\n\n";

	/* CN */
	std::cout << ConsoleColorTag(Console_Bold)
		<< "Please specify the common name" << ConsoleColorTag(Console_Normal)
		<< " (CN) [" << Utility::GetFQDN() << "]: ";

	std::getline(std::cin, answer);

	if (answer.empty())
		answer = Utility::GetFQDN();

	String cn = answer;
	cn = cn.Trim();

	std::cout << ConsoleColorTag(Console_Bold | Console_ForegroundGreen)
		<< "Reconfiguring Icinga...\n"
		<< ConsoleColorTag(Console_Normal);

	/* check whether the user wants to generate a new certificate or not */
	String existing_path = ApiListener::GetCertsDir() + "/" + cn + ".crt";

	std::cout << ConsoleColorTag(Console_Normal)
		<< "Checking for existing certificates for common name '" << cn << "'...\n";

	if (Utility::PathExists(existing_path)) {
		std::cout << "Certificate '" << existing_path << "' for CN '"
			<< cn << "' already existing. Skipping certificate generation.\n";
	} else {
		std::cout << "Certificates not yet generated. Running 'api setup' now.\n";
		ApiSetupUtility::SetupMasterCertificates(cn);
	}

	std::cout << ConsoleColorTag(Console_Bold)
		<< "Generating master configuration for Icinga 2.\n"
		<< ConsoleColorTag(Console_Normal);

	ApiSetupUtility::SetupMasterApiUser();

	if (!FeatureUtility::CheckFeatureEnabled("api"))
		ApiSetupUtility::SetupMasterEnableApi();
	else
		std::cout << "'api' feature already enabled.\n";

	/* Setup command hardcodes this as FQDN */
	String endpointName = cn;

	/* Different zone name. */
	std::cout << "\nMaster zone name [master]: ";
	std::getline(std::cin, answer);

	if (answer.empty())
		answer = "master";

	String zoneName = answer;
	zoneName = zoneName.Trim();

	/* Global zones. */
	std::vector<String> globalZones { "global-templates", "director-global" };

	std::cout << "\nDefault global zones: " << boost::algorithm::join(globalZones, " ");
	std::cout << "\nDo you want to specify additional global zones? [y/N]: ";

	std::getline(std::cin, answer);
	boost::algorithm::to_lower(answer);
	choice = answer;

wizard_global_zone_loop_start:
	if (choice.Contains("y")) {
		std::cout << "\nPlease specify the name of the global Zone: ";

		std::getline(std::cin, answer);

		if (answer.empty()) {
			std::cout << "\nName of the global Zone is required! Please retry.";
			goto wizard_global_zone_loop_start;
		}

		String globalZoneName = answer;
		globalZoneName = globalZoneName.Trim();

		if (std::find(globalZones.begin(), globalZones.end(), globalZoneName) != globalZones.end()) {
			std::cout << "The global zone '" << globalZoneName << "' is already specified."
				<< " Please retry.";
			goto wizard_global_zone_loop_start;
		}

		globalZones.push_back(globalZoneName);

		std::cout << "\nDo you want to specify another global zone? [y/N]: ";

		std::getline(std::cin, answer);
		boost::algorithm::to_lower(answer);
		choice = answer;

		if (choice.Contains("y"))
			goto wizard_global_zone_loop_start;
	} else
		Log(LogInformation, "cli", "No additional global Zones have been specified");

	/* Generate master configuration. */
	NodeUtility::GenerateNodeMasterIcingaConfig(endpointName, zoneName, globalZones);

	/* apilistener config */
	std::cout << ConsoleColorTag(Console_Bold)
		<< "Please specify the API bind host/port "
		<< ConsoleColorTag(Console_Normal) << "(optional)"
		<< ConsoleColorTag(Console_Bold) << ":\n";

	std::cout << ConsoleColorTag(Console_Bold)
		<< "Bind Host" << ConsoleColorTag(Console_Normal) << " []: ";

	std::getline(std::cin, answer);

	String bindHost = answer;
	bindHost = bindHost.Trim();

	std::cout << ConsoleColorTag(Console_Bold)
		<< "Bind Port" << ConsoleColorTag(Console_Normal) << " []: ";

	std::getline(std::cin, answer);

	String bindPort = answer;
	bindPort = bindPort.Trim();

	/* api feature is always enabled, check above */
	String apiConfPath = FeatureUtility::GetFeaturesAvailablePath() + "/api.conf";
	NodeUtility::CreateBackupFile(apiConfPath);

	AtomicFile fp (apiConfPath, 0644);

	fp << "/**\n"
		<< " * The API listener is used for distributed monitoring setups.\n"
		<< " */\n"
		<< "object ApiListener \"api\" {\n";

	if (!bindHost.IsEmpty())
		fp << "  bind_host = \"" << bindHost << "\"\n";
	if (!bindPort.IsEmpty())
		fp << "  bind_port = " << bindPort << "\n";

	fp << "\n"
		<< "  ticket_salt = TicketSalt\n"
		<< "}\n";

	fp.Commit();

	/* update constants.conf with NodeName = CN + TicketSalt = random value */
	if (cn != Utility::GetFQDN()) {
		Log(LogWarning, "cli")
			<< "CN '" << cn << "' does not match the default FQDN '"
			<< Utility::GetFQDN() << "'. Requires an update for the NodeName constant in constants.conf!";
	}

	Log(LogInformation, "cli", "Updating constants.conf.");

	NodeUtility::CreateBackupFile(NodeUtility::GetConstantsConfPath());

	NodeUtility::UpdateConstant("NodeName", endpointName);
	NodeUtility::UpdateConstant("ZoneName", zoneName);

	String salt = RandomString(16);

	NodeUtility::UpdateConstant("TicketSalt", salt);

	/* Disable conf.d inclusion */
	std::cout << "\nDo you want to disable the inclusion of the conf.d directory [Y/n]: ";

	std::getline(std::cin, answer);
	boost::algorithm::to_lower(answer);
	choice = answer;

	if (choice.Contains("n"))
		Log(LogInformation, "cli")
			<< "conf.d directory has not been disabled.";
	else {
		std::cout << ConsoleColorTag(Console_Bold | Console_ForegroundGreen)
			<< "Disabling the inclusion of the conf.d directory...\n"
			<< ConsoleColorTag(Console_Normal);

		if (!NodeUtility::UpdateConfiguration("\"conf.d\"", false, true)) {
			std::cout << ConsoleColorTag(Console_Bold | Console_ForegroundRed)
				<< "Failed to disable the conf.d inclusion, it may already have been disabled.\n"
				<< ConsoleColorTag(Console_Normal);
		}

		/* Include api-users.conf */
		String apiUsersFilePath = Configuration::ConfigDir + "/conf.d/api-users.conf";

		std::cout << ConsoleColorTag(Console_Bold | Console_ForegroundGreen)
			<< "Checking if the api-users.conf file exists...\n"
			<< ConsoleColorTag(Console_Normal);

		if (Utility::PathExists(apiUsersFilePath)) {
			NodeUtility::UpdateConfiguration("\"conf.d/api-users.conf\"", true, false);
		} else {
			Log(LogWarning, "cli")
				<< "Included file '" << apiUsersFilePath << "' does not exist.";
		}
	}

	return 0;
}
