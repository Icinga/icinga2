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
class I2_CLI_API NodeUtility
{
public:
	static String GetRepositoryPath(void);
	static String GetNodeRepositoryFile(const String& name);
	static String GetNodeSettingsFile(const String& name);
	static void CreateRepositoryPath(const String& path = GetRepositoryPath());
	static std::vector<String> GetNodeCompletionSuggestions(const String& word);

	static void PrintNodes(std::ostream& fp);
	static void PrintNodesJson(std::ostream& fp);
	static void PrintNodeRepository(std::ostream& fp, const Dictionary::Ptr& repository);
	static void AddNode(const String& name);
	static void AddNodeSettings(const String& name, const String& host, const String& port, double log_duration);
	static void RemoveNode(const String& name);

	static std::vector<Dictionary::Ptr> GetNodes(void);

	static bool CreateBackupFile(const String& target, bool is_private = false);

	static bool WriteNodeConfigObjects(const String& filename, const Array::Ptr& objects);

	static void UpdateConstant(const String& name, const String& value);

	/* node setup helpers */
	static int GenerateNodeIcingaConfig(const std::vector<std::string>& endpoints);
	static int GenerateNodeMasterIcingaConfig(void);

	/* black/whitelist */
	static String GetBlackAndWhiteListPath(const String& type);
	static Array::Ptr GetBlackAndWhiteList(const String& type);
	static int UpdateBlackAndWhiteList(const String& type, const String& node_filter,
	    const String& host_filter, const String& service_filter);
	static int RemoveBlackAndWhiteList(const String& type, const String& node_filter,
	    const String& host_filter, const String& service_filter);
	static int PrintBlackAndWhiteList(std::ostream& fp, const String& type);

	static bool CheckAgainstBlackAndWhiteList(const String& type, const String& node, const String& host, const String& service);

private:
	NodeUtility(void);
	static bool RemoveNodeFile(const String& path);
	static Dictionary::Ptr LoadNodeFile(const String& node_file);
	static void CollectNodes(const String& node_file, std::vector<Dictionary::Ptr>& nodes);

	static void SerializeObject(std::ostream& fp, const Dictionary::Ptr& object);
};

}

#endif /* NODEUTILITY_H */
