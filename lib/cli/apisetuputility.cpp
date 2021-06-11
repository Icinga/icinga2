/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

/*

[2/5] Building CXX object lib/cli/CMakeFiles/cli.dir/cli_unity.cpp.o
FAILED: lib/cli/CMakeFiles/cli.dir/cli_unity.cpp.o
/usr/local/Cellar/ccache/3.7.7/libexec/c++  -DBOOST_COROUTINES_NO_DEPRECATION_WARNING -DBOOST_FILESYSTEM_NO_DEPRECATED -I/usr/local/include -I/usr/local/Cellar/openssl@1.1/1.1.1d/include -I../third-party/nlohmann_json -I../third-party/utf8cpp/source -I../ -I../lib -I. -Ilib -I../third-party/crypt_blowfish -Qunused-arguments -fcolor-diagnostics -pthread -Winvalid-pch -std=c++0x -g -DI2_DEBUG -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX10.15.sdk -MD -MT lib/cli/CMakeFiles/cli.dir/cli_unity.cpp.o -MF lib/cli/CMakeFiles/cli.dir/cli_unity.cpp.o.d -o lib/cli/CMakeFiles/cli.dir/cli_unity.cpp.o -c lib/cli/cli_unity.cpp
In file included from lib/cli/cli_unity.cpp:2:
In file included from ../lib/cli/apisetuputility.cpp:20:
In file included from /usr/local/include/boost/multiprecision/cpp_int.hpp:12:
In file included from /usr/local/include/boost/multiprecision/number.hpp:25:
In file included from /usr/local/include/boost/multiprecision/detail/precision.hpp:9:
In file included from /usr/local/include/boost/multiprecision/traits/is_variable_precision.hpp:10:
/usr/local/include/boost/multiprecision/detail/number_base.hpp:54:12: warning: non-portable path to file '<VERSION>'; specified path differs in case from file name on disk [-Wnonportable-include-path]
#  include <version>
           ^~~~~~~~~
           <VERSION>
In file included from lib/cli/cli_unity.cpp:2:
In file included from ../lib/cli/apisetuputility.cpp:20:
In file included from /usr/local/include/boost/multiprecision/cpp_int.hpp:12:
In file included from /usr/local/include/boost/multiprecision/number.hpp:25:
In file included from /usr/local/include/boost/multiprecision/detail/precision.hpp:9:
In file included from /usr/local/include/boost/multiprecision/traits/is_variable_precision.hpp:10:
In file included from /usr/local/include/boost/multiprecision/detail/number_base.hpp:54:
../version:1:1: error: unknown type name 'Version'
Version: 2.12.0-rc1
^
../version:1:8: error: expected unqualified-id
Version: 2.12.0-rc1
       ^
1 warning and 2 errors generated.
ninja: build stopped: subcommand failed.

*/
#undef __has_include

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
#include <boost/multiprecision/cpp_int.hpp>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <fstream>
#include <vector>

extern "C" {
	#include <crypt_blowfish.h>
}

using namespace icinga;

namespace mp = boost::multiprecision;

static const char * const l_BCryptSaltChars = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
static const auto l_BCryptSaltCharsAmount (strlen(l_BCryptSaltChars));
static constexpr auto l_BCryptSaltLen (22);

// ceil(log(pow(l_BCryptSaltCharsAmount, l_BCryptSaltLen)) / log(2))
static constexpr auto l_BCryptSaltBits (132);

typedef mp::number<mp::cpp_int_backend<
	l_BCryptSaltBits, l_BCryptSaltBits, mp::unsigned_magnitude, mp::unchecked, void
>> BCryptSaltInt;

static constexpr auto l_RandomStepBits (std::numeric_limits<std::random_device::result_type>::digits);
static constexpr auto l_BCryptSaltRandomSteps ((l_BCryptSaltBits + (l_RandomStepBits - 1)) / l_RandomStepBits);

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

	char api_hashed_password[64] = { 0 };

	{
		char params[7 + l_BCryptSaltLen + 1] = "$2y$10$";

		{
			BCryptSaltInt salt (0);

			{
				std::random_device rd;

				for (auto steps (l_BCryptSaltRandomSteps); steps; --steps) {
					salt <<= l_RandomStepBits;
					salt += rd();
				}
			}

			auto len (strlen(params));

			for (auto steps (l_BCryptSaltLen); steps; --steps) {
				params[len++] = l_BCryptSaltChars[decltype(l_BCryptSaltCharsAmount)(salt % l_BCryptSaltCharsAmount)];
				salt /= l_BCryptSaltCharsAmount;
			}

			params[len] = 0;
		}

		_crypt_blowfish_rn(api_password.CStr(), params, api_hashed_password, sizeof(api_hashed_password));
	}

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
		<< "  // password = \"" << api_password << "\"\n"
		<< "  hashed_password = \"" << api_hashed_password << "\"\n"
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
