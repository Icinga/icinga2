/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/apisetuputility.hpp"
#include "cli/nodeutility.hpp"
#include "cli/featureutility.hpp"
#include "remote/apilistener.hpp"
#include "remote/pkiutility.hpp"
#include "base/logger.hpp"
#include "base/console.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"
#include "base/scriptglobal.hpp"
#include "base/exception.hpp"
#include "base/utility.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

using namespace icinga;

String ApiSetupUtility::GetConfdPath()
{
	return Configuration::ConfigDir + "/conf.d";
}

String ApiSetupUtility::GetApiUsersConfPath()
{
	return ApiSetupUtility::GetConfdPath() + "/api-users.conf";
}

bool ApiSetupUtility::SetupMaster(const String& cn, bool prompt_restart)
{
	if (!SetupMasterCertificates(cn))
		return false;

	if (!SetupMasterApiUser())
		return false;

	if (!SetupMasterEnableApi())
		return false;

	if (!SetupMasterUpdateConstants(cn))
		return false;

	if (prompt_restart) {
		std::cout << "Done.\n\n";
		std::cout << "Now reload your Icinga 2 daemon to finish the installation!\n\n";
	}

	return true;
}

bool ApiSetupUtility::SetupMasterCertificates(const String& cn)
{
	Log(LogInformation, "cli", "Generating new CA.");

	if (PkiUtility::NewCa() > 0)
		Log(LogWarning, "cli", "Found CA, skipping and using the existing one.");

	String pki_path = ApiListener::GetCertsDir();
	Utility::MkDirP(pki_path, 0700);

	String user = Configuration::RunAsUser;
	String group = Configuration::RunAsGroup;

	if (!Utility::SetFileOwnership(pki_path, user, group)) {
		Log(LogWarning, "cli")
			<< "Cannot set ownership for user '" << user << "' group '" << group << "' on file '" << pki_path << "'.";
	}

	String key = pki_path + "/" + cn + ".key";
	String csr = pki_path + "/" + cn + ".csr";

	if (Utility::PathExists(key)) {
		Log(LogInformation, "cli")
			<< "Private key file '" << key << "' already exists, not generating new certificate.";
		return true;
	}

	Log(LogInformation, "cli")
		<< "Generating new CSR in '" << csr << "'.";

	if (Utility::PathExists(key))
		NodeUtility::CreateBackupFile(key, true);
	if (Utility::PathExists(csr))
		NodeUtility::CreateBackupFile(csr);

	if (PkiUtility::NewCert(cn, key, csr, "") > 0) {
		Log(LogCritical, "cli", "Failed to create certificate signing request.");
		return false;
	}

	/* Sign the CSR with the CA key */
	String cert = pki_path + "/" + cn + ".crt";

	Log(LogInformation, "cli")
		<< "Signing CSR with CA and writing certificate to '" << cert << "'.";

	if (Utility::PathExists(cert))
		NodeUtility::CreateBackupFile(cert);

	if (PkiUtility::SignCsr(csr, cert) != 0) {
		Log(LogCritical, "cli", "Could not sign CSR.");
		return false;
	}

	/* Copy CA certificate to /etc/icinga2/pki */
	String ca_path = ApiListener::GetCaDir();
	String ca = ca_path + "/ca.crt";
	String ca_key = ca_path + "/ca.key";
	String target_ca = pki_path + "/ca.crt";

	Log(LogInformation, "cli")
		<< "Copying CA certificate to '" << target_ca << "'.";

	if (Utility::PathExists(target_ca))
		NodeUtility::CreateBackupFile(target_ca);

	/* does not overwrite existing files! */
	Utility::CopyFile(ca, target_ca);

	/* fix permissions: root -> icinga daemon user */
	for (const String& file : { ca_path, ca, ca_key, target_ca, key, csr, cert }) {
		if (!Utility::SetFileOwnership(file, user, group)) {
			Log(LogWarning, "cli")
				<< "Cannot set ownership for user '" << user << "' group '" << group << "' on file '" << file << "'.";
		}
	}

	return true;
}

bool ApiSetupUtility::SetupMasterApiUser()
{
	if (!Utility::PathExists(GetConfdPath())) {
		Log(LogWarning, "cli")
			<< "Path '" << GetConfdPath() << "' do not exist.";
		Log(LogInformation, "cli")
			<< "Creating path '" << GetConfdPath() << "'.";

		Utility::MkDirP(GetConfdPath(), 0755);
	}

	String api_username = "root"; // TODO make this available as cli parameter?
	String api_password = RandomString(8);
	String apiUsersPath = GetConfdPath() + "/api-users.conf";

	if (Utility::PathExists(apiUsersPath)) {
		Log(LogInformation, "cli")
			<< "API user config file '" << apiUsersPath << "' already exists, not creating config file.";
		return true;
	}

	Log(LogInformation, "cli")
		<< "Adding new ApiUser '" << api_username << "' in '" << apiUsersPath << "'.";

	NodeUtility::CreateBackupFile(apiUsersPath);

	std::fstream fp;
	String tempFilename = Utility::CreateTempFile(apiUsersPath + ".XXXXXX", 0644, fp);

	fp << "/**\n"
		<< " * The ApiUser objects are used for authentication against the API.\n"
		<< " */\n"
		<< "object ApiUser \"" << api_username << "\" {\n"
		<< "  password = \"" << api_password << "\"\n"
		<< "  // client_cn = \"\"\n"
		<< "\n"
		<< "  permissions = [ \"*\" ]\n"
		<< "}\n";

	fp.close();

	Utility::RenameFile(tempFilename, apiUsersPath);

	return true;
}

bool ApiSetupUtility::SetupMasterEnableApi()
{
	/*
	* Ensure the api-users.conf file is included, when conf.d inclusion is disabled.
	*/
	if (!NodeUtility::GetConfigurationIncludeState("\"conf.d\"", true))
		NodeUtility::UpdateConfiguration("\"conf.d/api-users.conf\"", true, false);

	/*
	* Enable the API feature
	*/
	Log(LogInformation, "cli", "Enabling the 'api' feature.");

	FeatureUtility::EnableFeatures({ "api" });

	return true;
}

bool ApiSetupUtility::SetupMasterUpdateConstants(const String& cn)
{
	NodeUtility::UpdateConstant("NodeName", cn);
	NodeUtility::UpdateConstant("ZoneName", cn);

	return true;
}
