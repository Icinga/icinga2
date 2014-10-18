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

#include "cli/agentutility.hpp"
#include "cli/clicommand.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"
#include "base/convert.hpp"
#include "base/serializer.hpp"
#include "base/netstring.hpp"
#include "base/stdiostream.hpp"
#include "base/debug.hpp"
#include "base/objectlock.hpp"
#include "base/console.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <fstream>
#include <iostream>

using namespace icinga;

String AgentUtility::GetRepositoryPath(void)
{
	return Application::GetLocalStateDir() + "/lib/icinga2/api/repository";
}

String AgentUtility::GetAgentRepositoryFile(const String& name)
{
	return GetRepositoryPath() + "/" + SHA256(name) + ".repo";
}

String AgentUtility::GetAgentSettingsFile(const String& name)
{
	return GetRepositoryPath() + "/" + SHA256(name) + ".settings";
}


std::vector<String> AgentUtility::GetFieldCompletionSuggestions(const String& word)
{
	std::vector<String> cache;
	std::vector<String> suggestions;

	GetAgents(cache);

	std::sort(cache.begin(), cache.end());

	BOOST_FOREACH(const String& suggestion, cache) {
		if (suggestion.Find(word) == 0)
			suggestions.push_back(suggestion);
	}

	return suggestions;
}

void AgentUtility::PrintAgents(std::ostream& fp)
{
	std::vector<String> agents;
	GetAgents(agents);

	BOOST_FOREACH(const String& agent, agents) {
		Dictionary::Ptr agent_obj = GetAgentFromRepository(GetAgentRepositoryFile(agent));
		fp << "Agent Name: " << agent << "\n";

		if (agent_obj) {
			fp << "Endpoint: " << agent_obj->Get("endpoint") << "\n";
			fp << "Zone: " << agent_obj->Get("zone") << "\n";
			fp << "Repository: ";
			fp << std::setw(4);
			PrintAgentRepository(fp, agent_obj->Get("repository"));
			fp << std::setw(0) << "\n";
		}
	}

	fp << "All agents: " << boost::algorithm::join(agents, " ") << "\n";
}

void AgentUtility::PrintAgentRepository(std::ostream& fp, const Dictionary::Ptr& repository)
{
	//TODO better formatting
	fp << JsonSerialize(repository);
}

void AgentUtility::PrintAgentsJson(std::ostream& fp)
{
	std::vector<String> agents;
	GetAgents(agents);

	BOOST_FOREACH(const String& agent, agents) {
		Dictionary::Ptr agent_obj = GetAgentFromRepository(GetAgentRepositoryFile(agent));
		if (agent_obj) {
			fp << JsonSerialize(agent_obj);
		}
	}
}

bool AgentUtility::AddAgent(const String& name)
{
	String path = GetAgentRepositoryFile(name);

	if (Utility::PathExists(path) ) {
		Log(LogCritical, "cli")
		    << "Cannot add agent repo. '" + path + "' already exists.\n";
		return false;
	}

	Dictionary::Ptr agent = make_shared<Dictionary>();

	agent->Set("seen", Utility::GetTime());
	agent->Set("endpoint", name);
	agent->Set("zone", name);
	agent->Set("repository", Empty);

	return WriteAgentToRepository(path, agent);
}

bool AgentUtility::AddAgentSettings(const String& name, const String& host, const String& port)
{
	String path = GetAgentSettingsFile(name);

	Dictionary::Ptr peer = make_shared<Dictionary>();

	peer->Set("agent_host", host);
	peer->Set("agent_port", port);

	return WriteAgentToRepository(path, peer);
}

bool AgentUtility::RemoveAgent(const String& name)
{
	if (!RemoveAgentFile(GetAgentRepositoryFile(name))) {
		Log(LogCritical, "cli")
		    << "Cannot remove agent repo. '" + GetAgentRepositoryFile(name) + "' does not exist.\n";
		return false;
	}
	if (Utility::PathExists(GetAgentSettingsFile(name))) {
		if (!RemoveAgentFile(GetAgentSettingsFile(name))) {
			Log(LogWarning, "cli")
			    << "Cannot remove agent settings. '" + GetAgentSettingsFile(name) + "' does not exist.\n";
			    return false;
		}
	}

	return true;
}

bool AgentUtility::RemoveAgentFile(const String& path)
{
	if (!Utility::PathExists(path)) {
		Log(LogCritical, "cli", "Cannot remove '" + path + "'. Does not exist.");
		return false;
	}

	if (unlink(path.CStr()) < 0) {
		Log(LogCritical, "cli", "Cannot remove file '" + path +
		    "'. Failed with error code " + Convert::ToString(errno) + ", \"" + Utility::FormatErrorNumber(errno) + "\".");
		return false;
	}

	return true;
}

bool AgentUtility::SetAgentAttribute(const String& name, const String& attr, const Value& val)
{
	String repo_path = GetAgentRepositoryFile(name);
	Dictionary::Ptr repo = GetAgentFromRepository(repo_path);

	if (repo) {
		repo->Set(attr, val);
		WriteAgentToRepository(repo_path, repo);
		return true;
	}

	return false;
}

bool AgentUtility::WriteAgentToRepository(const String& filename, const Dictionary::Ptr& item)
{
	Log(LogInformation, "cli", "Dumping agent to file '" + filename + "'");

	String tempFilename = filename + ".tmp";

        std::ofstream fp(tempFilename.CStr(), std::ofstream::out | std::ostream::trunc);
        fp << JsonSerialize(item);
        fp.close();

#ifdef _WIN32
	_unlink(filename.CStr());
#endif /* _WIN32 */

	if (rename(tempFilename.CStr(), filename.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(tempFilename));
	}

	return true;
}

Dictionary::Ptr AgentUtility::GetAgentFromRepository(const String& filename)
{
	std::fstream fp;
	fp.open(filename.CStr(), std::ifstream::in);

	if (!fp)
		return Dictionary::Ptr();

	String content((std::istreambuf_iterator<char>(fp)), std::istreambuf_iterator<char>());

	std::cout << "Content: " << content << "\n";

	fp.close();

	return JsonDeserialize(content);
}

bool AgentUtility::GetAgents(std::vector<String>& agents)
{
	String path = GetRepositoryPath();

	if (!Utility::Glob(path + "/*.repo",
	    boost::bind(&AgentUtility::CollectAgents, _1, boost::ref(agents)), GlobFile)) {
		Log(LogCritical, "cli", "Cannot access path '" + path + "'.");
		return false;
	}

	return true;
}

void AgentUtility::CollectAgents(const String& agent_file, std::vector<String>& agents)
{
	String agent = Utility::BaseName(agent_file);
	boost::algorithm::replace_all(agent, ".repo", "");

	Log(LogDebug, "cli", "Adding agent: " + agent);
	agents.push_back(agent);
}
