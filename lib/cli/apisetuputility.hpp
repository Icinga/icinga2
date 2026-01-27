// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef APISETUPUTILITY_H
#define APISETUPUTILITY_H

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

#endif /* APISETUPUTILITY_H */
