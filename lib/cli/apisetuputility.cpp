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
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

using namespace icinga;

String ApiSetupUtility::GetConfdPath(void)
{
        return Application::GetSysconfDir() + "/icinga2/conf.d";
}

bool ApiSetupUtility::SetupMaster(const String& cn, bool prompt_restart)
{
	if (!SetupMasterCertificates(cn))
		return false;

	if (!SetupMasterApiUser())
		return false;

	if (!SetupMasterEnableApi())
		return false;

	if (prompt_restart) {
		std::cout << "Done.\n\n";
		std::cout << "Now restart your Icinga 2 daemon to finish the installation!\n\n";
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

	String user = ScriptGlobal::Get("RunAsUser");
	String group = ScriptGlobal::Get("RunAsGroup");

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

bool ApiSetupUtility::SetupMasterApiUser(void)
{
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
	    << " * The APIUser objects are used for authentication against the API.\n"
	    << " */\n"
	    << "object ApiUser \"" << api_username << "\" {\n"
	    << "  password = \"" << api_password << "\"\n"
	    << "  // client_cn = \"\"\n"
	    << "\n"
	    << "  permissions = [ \"*\" ]\n"
	    << "}\n";

	fp.close();

#ifdef _WIN32
	_unlink(apiUsersPath.CStr());
#endif /* _WIN32 */

	if (rename(tempFilename.CStr(), apiUsersPath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(tempFilename));
	}

	return true;
}

bool ApiSetupUtility::SetupMasterEnableApi(void)
{
	Log(LogInformation, "cli", "Enabling the 'api' feature.");

	FeatureUtility::EnableFeatures({ "api" });

	return true;
}
