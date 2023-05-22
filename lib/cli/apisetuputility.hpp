/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/i2-base.hpp"
#include "cli/i2-cli.hpp"
#include "base/dictionary.hpp"
#include "base/array.hpp"
#include "base/value.hpp"
#include "base/string.hpp"
#include <vector>

namespace icinga
{

/**
 * @ingroup cli
 */
class ApiSetupUtility
{
public:
	static bool SetupMaster(const String& cn, bool prompt_restart = false);

	static bool SetupMasterCertificates(const String& cn);
	static bool SetupMasterApiUser();
	static bool SetupMasterEnableApi();
	static bool SetupMasterUpdateConstants(const String& cn);

	static String GetConfdPath();
	static String GetApiUsersConfPath();

private:
	ApiSetupUtility();
};

}
