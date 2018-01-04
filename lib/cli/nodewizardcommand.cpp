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

#include "cli/nodewizardcommand.hpp"
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
	return ImpersonateRoot;
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
	 * 11. set NodeName = cn in constants.conf
	 * 12. reload icinga2, or tell the user to
	 */

	std::string answer;
	/* master or satellite/client setup */
	std::cout << ConsoleColorTag(Console_Bold)
		<< "Please specify if this is a satellite/client setup "
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
		res = ClientSetup();

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

int NodeWizardCommand::ClientSetup() const
{
	std::string answer;
	String choice;
	bool connectToParent = false;

	std::cout << "Starting the Client/Satellite setup routine...\n\n";

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

	String parentHost, parentPort;

	for (const String& endpoint : endpoints) {
		std::vector<String> tokens = endpoint.Split(",");

		if (tokens.size() > 1)
			parentHost = tokens[1];

		if (tokens.size() > 2)
			parentPort = tokens[2];
	}

	/* workaround for fetching the master cert */
	String certsDir = ApiListener::GetCertsDir();
	Utility::MkDirP(certsDir, 0700);

	String user = ScriptGlobal::Get("RunAsUser");
	String group = ScriptGlobal::Get("RunAsGroup");

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
		 * into LocalStateDir + "/lib/icinga2/certs"
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

	/* disable the notifications feature on client nodes */
	Log(LogInformation, "cli", "Disabling the Notification feature.");

	FeatureUtility::DisableFeatures({ "notification" });

	Log(LogInformation, "cli", "Enabling the ApiListener feature.");

	FeatureUtility::EnableFeatures({ "api" });

	String apiConfPath = FeatureUtility::GetFeaturesAvailablePath() + "/api.conf";
	NodeUtility::CreateBackupFile(apiConfPath);

	std::fstream fp;
	String tempApiConfPath = Utility::CreateTempFile(apiConfPath + ".XXXXXX", 0644, fp);

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

	fp.close();

#ifdef _WIN32
	_unlink(apiConfPath.CStr());
#endif /* _WIN32 */

	if (rename(tempApiConfPath.CStr(), apiConfPath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("rename")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(tempApiConfPath));
	}

	/* apilistener config */
	Log(LogInformation, "cli", "Generating local zones.conf.");

	NodeUtility::GenerateNodeIcingaConfig(endpoints, { "global-templates", "director-global" });

	if (cn != Utility::GetFQDN()) {
		Log(LogWarning, "cli")
			<< "CN '" << cn << "' does not match the default FQDN '"
			<< Utility::GetFQDN() << "'. Requires update for NodeName constant in constants.conf!";
	}

	NodeUtility::UpdateConstant("NodeName", cn);
	NodeUtility::UpdateConstant("ZoneName", cn);

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

	NodeUtility::GenerateNodeMasterIcingaConfig({ "global-templates", "director-global" });

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

	std::fstream fp;
	String tempApiConfPath = Utility::CreateTempFile(apiConfPath + ".XXXXXX", 0644, fp);

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

	fp.close();

#ifdef _WIN32
	_unlink(apiConfPath.CStr());
#endif /* _WIN32 */

	if (rename(tempApiConfPath.CStr(), apiConfPath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("rename")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(tempApiConfPath));
	}

	/* update constants.conf with NodeName = CN + TicketSalt = random value */
	if (cn != Utility::GetFQDN()) {
		Log(LogWarning, "cli")
			<< "CN '" << cn << "' does not match the default FQDN '"
			<< Utility::GetFQDN() << "'. Requires an update for the NodeName constant in constants.conf!";
	}

	NodeUtility::UpdateConstant("NodeName", cn);
	NodeUtility::UpdateConstant("ZoneName", cn);

	String salt = RandomString(16);

	NodeUtility::UpdateConstant("TicketSalt", salt);

	return 0;
}
