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
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
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
		    << "Cannot add agent repo. '" << path << "' already exists.\n";
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
		    << "Cannot remove agent repo. '" << GetAgentRepositoryFile(name) << "' does not exist.\n";
		return false;
	}

	if (Utility::PathExists(GetAgentSettingsFile(name))) {
		if (!RemoveAgentFile(GetAgentSettingsFile(name))) {
			Log(LogWarning, "cli")
			    << "Cannot remove agent settings. '" << GetAgentSettingsFile(name) << "' does not exist.\n";
			    return false;
		}
	}

	return true;
}

bool AgentUtility::RemoveAgentFile(const String& path)
{
	if (!Utility::PathExists(path)) {
		Log(LogCritical, "cli")
		    << "Cannot remove '" << path << "'. Does not exist.";
		return false;
	}

	if (unlink(path.CStr()) < 0) {
		Log(LogCritical, "cli")
		    << "Cannot remove file '" << path
		    << "'. Failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) + "\".";
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
	Log(LogInformation, "cli")
	    << "Dumping agent to file '" << filename << "'";

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
		Log(LogCritical, "cli")
		    << "Cannot access path '" << path << "'.";
		return false;
	}

	return true;
}

void AgentUtility::CollectAgents(const String& agent_file, std::vector<String>& agents)
{
	String agent = Utility::BaseName(agent_file);
	boost::algorithm::replace_all(agent, ".repo", "");

	Log(LogDebug, "cli")
	    << "Adding agent: " << agent;

	agents.push_back(agent);
}

/*
 * Agent Setup helpers
 */

int AgentUtility::GenerateAgentIcingaConfig(const std::vector<std::string>& endpoints, const String& nodename)
{
	Array::Ptr my_config = make_shared<Array>();

	Dictionary::Ptr my_master_zone = make_shared<Dictionary>();
	Array::Ptr my_master_zone_members = make_shared<Array>();

	BOOST_FOREACH(const std::string& endpoint, endpoints) {

		/* extract all --endpoint arguments and store host,port info */
		std::vector<String> tokens;
		boost::algorithm::split(tokens, endpoint, boost::is_any_of(","));

		Dictionary::Ptr my_master_endpoint = make_shared<Dictionary>();

		if (tokens.size() > 1)
			my_master_endpoint->Set("host", tokens[1]);

		if (tokens.size() > 2)
			my_master_endpoint->Set("port", tokens[2]);

		my_master_endpoint->Set("__name", tokens[0]);
		my_master_endpoint->Set("__type", "Endpoint");

		/* save endpoint in master zone */
		my_master_zone_members->Add(tokens[0]);

		my_config->Add(my_master_endpoint);
	}


	/* add the master zone to the config */
	my_master_zone->Set("__name", "master"); //hardcoded name
	my_master_zone->Set("__type", "Zone");
	my_master_zone->Set("endpoints", my_master_zone_members);

	my_config->Add(my_master_zone);

	/* store the local generated agent configuration */
	Dictionary::Ptr my_endpoint = make_shared<Dictionary>();
	Dictionary::Ptr my_zone = make_shared<Dictionary>();

	my_endpoint->Set("__name", nodename);
	my_endpoint->Set("__type", "Endpoint");

	Array::Ptr my_zone_members = make_shared<Array>();
	my_zone_members->Add(nodename);

	my_zone->Set("__name", nodename);
	my_zone->Set("__type", "Zone");
	my_zone->Set("//this is the local agent", nodename);
	my_zone->Set("endpoints", my_zone_members);

	/* store the local config */
	my_config->Add(my_endpoint);
	my_config->Add(my_zone);

	/* write the newly generated configuration */
	String zones_path = Application::GetSysconfDir() + "/icinga2/zones.conf";

	AgentUtility::WriteAgentConfigObjects(zones_path, my_config);

	return 0;
}

int AgentUtility::GenerateAgentMasterIcingaConfig(const String& nodename)
{
	Array::Ptr my_config = make_shared<Array>();

	/* store the local generated agent master configuration */
	Dictionary::Ptr my_master_endpoint = make_shared<Dictionary>();
	Dictionary::Ptr my_master_zone = make_shared<Dictionary>();
	Array::Ptr my_master_zone_members = make_shared<Array>();

	my_master_endpoint->Set("__name", nodename);
	my_master_endpoint->Set("__type", "Endpoint");

	my_master_zone_members->Add(nodename);

	my_master_zone->Set("__name", "master");
	my_master_zone->Set("__type", "Zone");
	my_master_zone->Set("//this is the local agent master named ", "master");
	my_master_zone->Set("endpoints", my_master_zone_members);

	/* store the local config */
	my_config->Add(my_master_endpoint);
	my_config->Add(my_master_zone);

	/* write the newly generated configuration */
	String zones_path = Application::GetSysconfDir() + "/icinga2/zones.conf";

	AgentUtility::WriteAgentConfigObjects(zones_path, my_config);

	return 0;
}

/*
 * This is ugly and requires refactoring into a generic config writer class.
 * TODO.
 */
bool AgentUtility::WriteAgentConfigObjects(const String& filename, const Array::Ptr& objects)
{
	Log(LogInformation, "cli")
	    << "Dumping config items to file '" << filename << "'.";

	/* create a backup first */
	CreateBackupFile(filename);

	Utility::MkDirP(Utility::DirName(filename), 0755);

	String tempPath = filename + ".tmp";

        std::ofstream fp(tempPath.CStr(), std::ofstream::out | std::ostream::trunc);

	fp << "/*\n";
	fp << " * Generated by Icinga 2 agent setup commands\n";
	fp << " * on " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", Utility::GetTime()) << "\n";
	fp << " */\n\n";

	ObjectLock olock(objects);

	BOOST_FOREACH(const Dictionary::Ptr& object, objects) {
		String name = object->Get("__name");
		String type = object->Get("__type");

		SerializeObject(fp, name, type, object);
	}

	fp << std::endl;
        fp.close();

#ifdef _WIN32
	_unlink(filename.CStr());
#endif /* _WIN32 */

	if (rename(tempPath.CStr(), filename.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(tempPath));
	}

	return true;
}

/*
 * We generally don't overwrite files without backup before
 */
bool AgentUtility::CreateBackupFile(const String& target)
{
	if (Utility::PathExists(target)) {
		String backup = target + ".orig";

#ifdef _WIN32
		_unlink(backup.CStr());
#endif /* _WIN32 */

		if (rename(target.CStr(), backup.CStr()) < 0) {
			BOOST_THROW_EXCEPTION(posix_error()
			    << boost::errinfo_api_function("rename")
			    << boost::errinfo_errno(errno)
			    << boost::errinfo_file_name(target));
		}

		Log(LogInformation, "cli")
		    << "Created backup file '" << backup << "'.";
	}

	return true;
}

void AgentUtility::SerializeObject(std::ostream& fp, const String& name, const String& type, const Dictionary::Ptr& object)
{
        fp << "object " << type << " \"" << name << "\" {\n";
	ObjectLock olock(object);
        BOOST_FOREACH(const Dictionary::Pair& kv, object) {
		if (kv.first == "__type" || kv.first == "__name")
			continue;

                fp << "\t" << kv.first << " = ";
                FormatValue(fp, kv.second);
                fp << "\n";
        }
        fp << "}\n\n";
}

void AgentUtility::FormatValue(std::ostream& fp, const Value& val)
{
        if (val.IsObjectType<Array>()) {
                FormatArray(fp, val);
                return;
        }

        if (val.IsString()) {
                fp << "\"" << Convert::ToString(val) << "\"";
                return;
        }

        fp << Convert::ToString(val);
}

void AgentUtility::FormatArray(std::ostream& fp, const Array::Ptr& arr)
{
        bool first = true;

        fp << "[ ";

        if (arr) {
                ObjectLock olock(arr);
                BOOST_FOREACH(const Value& value, arr) {
                        if (first)
                                first = false;
                        else
                                fp << ", ";

                        FormatValue(fp, value);
                }
        }

        if (!first)
                fp << " ";

        fp << "]";
}

void AgentUtility::UpdateConstant(const String& name, const String& value)
{
	String constantsFile = Application::GetSysconfDir() + "/icinga2/constants.conf";
	String tempFile = constantsFile + ".tmp";

	std::ifstream ifp(constantsFile.CStr());
	std::ofstream ofp(tempFile.CStr());

	bool found = false;

	std::string line;
	while (std::getline(ifp, line)) {
		if (line.find("const " + name + " = ") != std::string::npos) {
			ofp << "const " + name + " = \"" + value + "\"\n";
			found = true;
		} else
			ofp << line << "\n";
	}

	if (!found)
		ofp << "const " + name + " = \"" + value + "\"\n";

	ifp.close();
	ofp.close();

#ifdef _WIN32
	_unlink(constantsFile.CStr());
#endif /* _WIN32 */

	if (rename(tempFile.CStr(), constantsFile.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("rename")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(constantsFile));
	}
}
