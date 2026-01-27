// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef NODEUTILITY_H
#define NODEUTILITY_H

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
class NodeUtility
{
public:
	static String GetConstantsConfPath();
	static String GetZonesConfPath();

	static bool CreateBackupFile(const String& target, bool isPrivate = false);

	static bool WriteNodeConfigObjects(const String& filename, const Array::Ptr& objects);

	static bool GetConfigurationIncludeState(const String& value, bool recursive);
	static bool UpdateConfiguration(const String& value, bool include, bool recursive);
	static void UpdateConstant(const String& name, const String& value);

	/* node setup helpers */
	static int GenerateNodeIcingaConfig(const String& endpointName, const String& zoneName,
		const String& parentZoneName, const std::vector<std::string>& endpoints,
		const std::vector<String>& globalZones);
	static int GenerateNodeMasterIcingaConfig(const String& endpointName, const String& zoneName,
		const std::vector<String>& globalZones);

private:
	NodeUtility();

	static void SerializeObject(std::ostream& fp, const Dictionary::Ptr& object);
};

}

#endif /* NODEUTILITY_H */
