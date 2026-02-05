// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DAEMONUTILITY_H
#define DAEMONUTILITY_H

#include "cli/i2-cli.hpp"
#include "config/configitem.hpp"
#include "base/string.hpp"
#include <boost/program_options.hpp>

namespace icinga
{

/**
 * @ingroup cli
 */
class DaemonUtility
{
public:
	static bool ValidateConfigFiles(const std::vector<std::string>& configs, const String& objectsFile = String());
	static bool LoadConfigFiles(const std::vector<std::string>& configs, std::vector<ConfigItem::Ptr>& newItems,
		const String& objectsFile = String(), const String& varsfile = String());
};

}

#endif /* DAEMONULITIY_H */
