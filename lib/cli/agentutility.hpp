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
	static std::vector<String> GetFieldCompletionSuggestions(const String& word);

	static void PrintAgents(std::ostream& fp);
	static void PrintAgentsJson(std::ostream& fp);
	static void PrintAgentRepository(std::ostream& fp, const Dictionary::Ptr& repository);
	static bool AddAgent(const String& name);
	static bool AddAgentSettings(const String& name, const String& host, const String& port);
	static bool RemoveAgent(const String& name);
	static bool SetAgentAttribute(const String& name, const String& attr, const Value& val);

	static bool WriteAgentToRepository(const String& filename, const Dictionary::Ptr& item);
	static Dictionary::Ptr GetAgentFromRepository(const String& filename);

	static bool GetAgents(std::vector<String>& agents);

	static bool WriteAgentConfigObjects(const String& filename, const Array::Ptr& objects);

private:
	AgentUtility(void);
	static bool RemoveAgentFile(const String& path);
	static void CollectAgents(const String& agent_file, std::vector<String>& agents);

	static void SerializeObject(std::ostream& fp, const String& name, const String& type, const Dictionary::Ptr& object);
	static void FormatValue(std::ostream& fp, const Value& val);
	static void FormatArray(std::ostream& fp, const Array::Ptr& arr);
};

}

#endif /* AGENTUTILITY_H */
