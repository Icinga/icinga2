/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#ifndef AGENTUTILITY_H
#define AGENTUTILITY_H

#include "base/i2-base.hpp"
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
class AgentUtility
{
public:
	static String GetRepositoryPath(void);
	static String GetAgentRepositoryFile(const String& name);
	static String GetAgentSettingsFile(const String& name);
	static std::vector<String> GetAgentCompletionSuggestions(const String& word);

	static void PrintAgents(std::ostream& fp);
	static void PrintAgentsJson(std::ostream& fp);
	static void PrintAgentRepository(std::ostream& fp, const Dictionary::Ptr& repository);
	static void AddAgent(const String& name);
	static void AddAgentSettings(const String& name, const String& host, const String& port, double log_duration);
	static void RemoveAgent(const String& name);

	static std::vector<Dictionary::Ptr> GetAgents(void);

	static bool CreateBackupFile(const String& target);

	static bool WriteAgentConfigObjects(const String& filename, const Array::Ptr& objects);

	static void UpdateConstant(const String& name, const String& value);

	/* agent setup helpers */
	static int GenerateAgentIcingaConfig(const std::vector<std::string>& endpoints, const String& nodename);
	static int GenerateAgentMasterIcingaConfig(const String& nodename);

	/* black/whitelist */
	static int UpdateBlackAndWhiteList(const String& type, const String& agent_filter,
	    const String& host_filter, const String& service_filter);
	static int RemoveBlackAndWhiteList(const String& type, const String& agent_filter,
	    const String& host_filter, const String& service_filter);
	static int PrintBlackAndWhiteList(std::ostream& fp, const String& type);

private:
	AgentUtility(void);
	static bool RemoveAgentFile(const String& path);
	static Dictionary::Ptr LoadAgentFile(const String& agent_file);
	static void CollectAgents(const String& agent_file, std::vector<Dictionary::Ptr>& agents);

	static void SerializeObject(std::ostream& fp, const String& name, const String& type, const Dictionary::Ptr& object);
	static void FormatValue(std::ostream& fp, const Value& val);
	static void FormatArray(std::ostream& fp, const Array::Ptr& arr);
};

}

#endif /* AGENTUTILITY_H */
