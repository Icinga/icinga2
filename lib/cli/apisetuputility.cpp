/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
#include "cli/pkiutility.hpp"
#include "cli/nodeutility.hpp"
#include "cli/featureutility.hpp"
#include "base/logger.hpp"
#include "base/console.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"
#include "base/scriptglobal.hpp"
#include "base/exception.hpp"
#include <boost/foreach.hpp>
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

int ApiSetupUtility::SetupMaster(const String& cn)
{
	/* if the 'api' feature is enabled we can safely assume
	 * that either 'api setup' was run, or the user manually
	 * enabled the api including all certificates e.g. by 'node wizard' in <= v2.3.x
	 */
	if (FeatureUtility::CheckFeatureEnabled("api")) {
		Log(LogInformation, "cli")
		    << "'api' feature already enabled, skipping feature enable and master certificate creation.\n";
		return 0;
	}

	Log(LogInformation, "cli")
	    << "Generating new CA.\n";

	if (PkiUtility::NewCa() > 0) {
		Log(LogWarning, "cli", "Found CA, skipping and using the existing one.");
	}

	String pki_path = PkiUtility::GetPkiPath();

	if (!Utility::MkDirP(pki_path, 0700)) {
		Log(LogCritical, "cli")
		    << "Could not create local pki directory '" << pki_path << "'.";
		return 1;
	}

	String user = ScriptGlobal::Get("RunAsUser");
	String group = ScriptGlobal::Get("RunAsGroup");

	if (!Utility::SetFileOwnership(pki_path, user, group)) {
		Log(LogWarning, "cli")
		    << "Cannot set ownership for user '" << user << "' group '" << group << "' on file '" << pki_path << "'. Verify it yourself!";
	}

	String key = pki_path + "/" + cn + ".key";
	String csr = pki_path + "/" + cn + ".csr";

	Log(LogInformation, "cli")
	    << "Generating new CSR in '" << csr << "'.\n";

	if (Utility::PathExists(key))
		NodeUtility::CreateBackupFile(key, true);
	if (Utility::PathExists(csr))
		NodeUtility::CreateBackupFile(csr);

	if (PkiUtility::NewCert(cn, key, csr, "") > 0) {
		Log(LogCritical, "cli", "Failed to create certificate signing request.");
		return 1;
	}

	/* Sign the CSR with the CA key */
	String cert = pki_path + "/" + cn + ".crt";

	Log(LogInformation, "cli")
	    << "Signing CSR with CA and writing certificate to '" << cert << "'.\n";

	if (Utility::PathExists(cert))
		NodeUtility::CreateBackupFile(cert);

	if (PkiUtility::SignCsr(csr, cert) != 0) {
		Log(LogCritical, "cli", "Could not sign CSR.");
		return 1;
	}

		/* Copy CA certificate to /etc/icinga2/pki */

	String ca_path = PkiUtility::GetLocalCaPath();
	String ca = ca_path + "/ca.crt";
	String ca_key = ca_path + "/ca.key";
	String serial = ca_path + "/serial.txt";
	String target_ca = pki_path + "/ca.crt";

	Log(LogInformation, "cli")
	    << "Copying CA certificate to '" << target_ca << "'.\n";

	if (Utility::PathExists(target_ca))
		NodeUtility::CreateBackupFile(target_ca);

	/* does not overwrite existing files! */
	Utility::CopyFile(ca, target_ca);

	/* fix permissions: root -> icinga daemon user */
	std::vector<String> files;
	files.push_back(ca_path);
	files.push_back(ca);
	files.push_back(ca_key);
	files.push_back(serial);
	files.push_back(target_ca);
	files.push_back(key);
	files.push_back(csr);
	files.push_back(cert);

	BOOST_FOREACH(const String& file, files) {
		if (!Utility::SetFileOwnership(file, user, group)) {
			Log(LogWarning, "cli")
			    << "Cannot set ownership for user '" << user << "' group '" << group << "' on file '" << file << "'. Verify it yourself!";
		}
	}

	String api_username = "root"; //TODO make this available as cli parameter?
	String api_password = RandomString(8);
	String apiuserspath = GetConfdPath() + "/api-users.conf";

	Log(LogInformation, "cli")
	    << "Adding new ApiUser '" << api_username << "' in '" << apiuserspath << "'.\n";

	NodeUtility::CreateBackupFile(apiuserspath);

	String apiuserspathtmp = apiuserspath + ".tmp";

	std::ofstream fp;
	fp.open(apiuserspathtmp.CStr(), std::ofstream::out | std::ofstream::trunc);

	fp << "/**\n"
	    << " * The APIUser objects are used for authentication against the API.\n"
	    << " */\n"
	    << "object ApiUser \"" << api_username << "\" {\n"
	    << "  password = \"" << api_password << "\"\n"
	    << "  //client_cn = \"\"\n"
	    << "}\n";

	fp.close();

#ifdef _WIN32
	_unlink(apiuserspath.CStr());
#endif /* _WIN32 */

	if (rename(apiuserspathtmp.CStr(), apiuserspath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(apiuserspathtmp));
	}


	Log(LogInformation, "cli", "Enabling the ApiListener feature.\n");

	std::vector<std::string> enable;
	enable.push_back("api");
	FeatureUtility::EnableFeatures(enable);

	return 0;
}
