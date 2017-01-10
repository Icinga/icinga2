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

#include "cli/nodeutility.hpp"
#include "cli/clicommand.hpp"
#include "cli/variableutility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/scriptglobal.hpp"
#include "base/json.hpp"
#include "base/netstring.hpp"
#include "base/stdiostream.hpp"
#include "base/debug.hpp"
#include "base/objectlock.hpp"
#include "base/console.hpp"
#include "base/exception.hpp"
#include "base/configwriter.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <fstream>
#include <iostream>

using namespace icinga;

String NodeUtility::GetRepositoryPath(void)
{
	return Application::GetLocalStateDir() + "/lib/icinga2/api/repository";
}

String NodeUtility::GetNodeRepositoryFile(const String& name)
{
	return GetRepositoryPath() + "/" + SHA256(name) + ".repo";
}

String NodeUtility::GetNodeSettingsFile(const String& name)
{
	return GetRepositoryPath() + "/" + SHA256(name) + ".settings";
}

void NodeUtility::CreateRepositoryPath(const String& path)
{
	if (!Utility::PathExists(path))
		Utility::MkDirP(path, 0750);

	String user = ScriptGlobal::Get("RunAsUser");
	String group = ScriptGlobal::Get("RunAsGroup");

	if (!Utility::SetFileOwnership(path, user, group)) {
		Log(LogWarning, "cli")
		    << "Cannot set ownership for user '" << user << "' group '" << group << "' on path '" << path << "'. Verify it yourself!";
	}
}

std::vector<String> NodeUtility::GetNodeCompletionSuggestions(const String& word)
{
	std::vector<String> suggestions;

	for (const Dictionary::Ptr& node : GetNodes()) {
		String node_name = node->Get("endpoint");

		if (node_name.Find(word) == 0)
			suggestions.push_back(node_name);
	}

	return suggestions;
}

void NodeUtility::PrintNodes(std::ostream& fp)
{
	bool first = true;

	for (const Dictionary::Ptr& node : GetNodes()) {
		if (first)
			first = false;
		else
			fp << "\n";

		fp << "Node '"
		   << ConsoleColorTag(Console_ForegroundBlue | Console_Bold) << node->Get("endpoint") << ConsoleColorTag(Console_Normal)
		   << "' (";

		Dictionary::Ptr settings = node->Get("settings");

		if (settings) {
			String host = settings->Get("host");
			String port = settings->Get("port");
			double log_duration = settings->Get("log_duration");

			if (!host.IsEmpty() && !port.IsEmpty())
				fp << "host: " << host << ", port: " << port << ", ";

			fp << "log duration: " << Utility::FormatDuration(log_duration) << ", ";
		}

		fp << "last seen: " << Utility::FormatDateTime("%c", node->Get("seen")) << ")\n";

		PrintNodeRepository(fp, node->Get("repository"));
	}
}

void NodeUtility::PrintNodeRepository(std::ostream& fp, const Dictionary::Ptr& repository)
{
	if (!repository)
		return;

	ObjectLock olock(repository);
	for (const Dictionary::Pair& kv : repository) {
		fp << std::setw(4) << " "
		   << "* Host '" << ConsoleColorTag(Console_ForegroundGreen | Console_Bold) << kv.first << ConsoleColorTag(Console_Normal) << "'\n";

		Array::Ptr services = kv.second;
		ObjectLock xlock(services);
		for (const String& service : services) {
			fp << std::setw(8) << " " << "* Service '" << ConsoleColorTag(Console_ForegroundGreen | Console_Bold) << service << ConsoleColorTag(Console_Normal) << "'\n";
		}
	}
}

void NodeUtility::PrintNodesJson(std::ostream& fp)
{
	Dictionary::Ptr result = new Dictionary();

	for (const Dictionary::Ptr& node : GetNodes()) {
		result->Set(node->Get("endpoint"), node);
	}

	fp << JsonEncode(result);
}

void NodeUtility::AddNode(const String& name)
{
	String path = GetNodeRepositoryFile(name);

	if (Utility::PathExists(path) ) {
		Log(LogInformation, "cli")
		    << "Node '" << name << "' exists already.";
	}

	Dictionary::Ptr node = new Dictionary();

	node->Set("seen", Utility::GetTime());
	node->Set("endpoint", name);
	node->Set("zone", name);
	node->Set("repository", Empty);

	CreateRepositoryPath();
	Utility::SaveJsonFile(path, 0600, node);
}

void NodeUtility::AddNodeSettings(const String& name, const String& host,
    const String& port, double log_duration)
{
	Dictionary::Ptr settings = new Dictionary();

	settings->Set("host", host);
	settings->Set("port", port);
	settings->Set("log_duration", log_duration);

	CreateRepositoryPath();
	Utility::SaveJsonFile(GetNodeSettingsFile(name), 0600, settings);
}

void NodeUtility::RemoveNode(const String& name)
{
	String repoPath = GetNodeRepositoryFile(name);

	if (!Utility::PathExists(repoPath))
		return;

	if (unlink(repoPath.CStr()) < 0) {
		Log(LogCritical, "cli")
		    << "Cannot remove file '" << repoPath
		    << "'. Failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) + "\".";
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("unlink")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(repoPath));
	}

	String settingsPath = GetNodeSettingsFile(name);

	if (Utility::PathExists(settingsPath)) {
		if (unlink(settingsPath.CStr()) < 0) {
			Log(LogCritical, "cli")
			    << "Cannot remove file '" << settingsPath
			    << "'. Failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) + "\".";
			BOOST_THROW_EXCEPTION(posix_error()
			    << boost::errinfo_api_function("unlink")
			    << boost::errinfo_errno(errno)
			    << boost::errinfo_file_name(settingsPath));
		}
	}
}

std::vector<Dictionary::Ptr> NodeUtility::GetNodes(void)
{
	std::vector<Dictionary::Ptr> nodes;

	Utility::Glob(GetRepositoryPath() + "/*.repo",
	    boost::bind(&NodeUtility::CollectNodes, _1, boost::ref(nodes)), GlobFile);

	return nodes;
}

Dictionary::Ptr NodeUtility::LoadNodeFile(const String& node_file)
{
	Dictionary::Ptr node = Utility::LoadJsonFile(node_file);

	if (!node)
		return Dictionary::Ptr();

	String settingsFile = GetNodeSettingsFile(node->Get("endpoint"));

	if (Utility::PathExists(settingsFile))
		node->Set("settings", Utility::LoadJsonFile(settingsFile));
	else
		node->Remove("settings");

	return node;
}

void NodeUtility::CollectNodes(const String& node_file, std::vector<Dictionary::Ptr>& nodes)
{
	Dictionary::Ptr node;

	try {
		node = LoadNodeFile(node_file);
	} catch (const std::exception&) {
		return;
	}

	if (!node)
		return;

	nodes.push_back(node);
}

/*
 * Node Setup helpers
 */

int NodeUtility::GenerateNodeIcingaConfig(const std::vector<std::string>& endpoints)
{
	Array::Ptr my_config = new Array();

	Dictionary::Ptr my_master_zone = new Dictionary();
	Array::Ptr my_master_zone_members = new Array();

	String master_zone_name = "master"; //TODO: Find a better name.

	for (const std::string& endpoint : endpoints) {

		/* extract all --endpoint arguments and store host,port info */
		std::vector<String> tokens;
		boost::algorithm::split(tokens, endpoint, boost::is_any_of(","));

		Dictionary::Ptr my_master_endpoint = new Dictionary();

		if (tokens.size() > 1) {
			String host = tokens[1].Trim();

			if (!host.IsEmpty())
				my_master_endpoint->Set("host", host);
		}

		if (tokens.size() > 2) {
			String port = tokens[2].Trim();

			if (!port.IsEmpty())
				my_master_endpoint->Set("port", port);
		}

		String cn = tokens[0].Trim();
		my_master_endpoint->Set("__name", cn);
		my_master_endpoint->Set("__type", "Endpoint");

		/* save endpoint in master zone */
		my_master_zone_members->Add(cn);

		my_config->Add(my_master_endpoint);
	}

	/* add the master zone to the config */
	my_master_zone->Set("__name", master_zone_name);
	my_master_zone->Set("__type", "Zone");
	my_master_zone->Set("endpoints", my_master_zone_members);

	my_config->Add(my_master_zone);

	/* store the local generated node configuration */
	Dictionary::Ptr my_endpoint = new Dictionary();
	Dictionary::Ptr my_zone = new Dictionary();

	my_endpoint->Set("__name", new ConfigIdentifier("NodeName"));
	my_endpoint->Set("__type", "Endpoint");

	Array::Ptr my_zone_members = new Array();
	my_zone_members->Add(new ConfigIdentifier("NodeName"));

	my_zone->Set("__name", new ConfigIdentifier("ZoneName"));
	my_zone->Set("__type", "Zone");
	my_zone->Set("parent", master_zone_name); //set the master zone as parent

	my_zone->Set("endpoints", my_zone_members);

	/* store the local config */
	my_config->Add(my_endpoint);
	my_config->Add(my_zone);

	/* write the newly generated configuration */
	String zones_path = Application::GetSysconfDir() + "/icinga2/zones.conf";

	NodeUtility::WriteNodeConfigObjects(zones_path, my_config);

	return 0;
}

int NodeUtility::GenerateNodeMasterIcingaConfig(void)
{
	Array::Ptr my_config = new Array();

	/* store the local generated node master configuration */
	Dictionary::Ptr my_master_endpoint = new Dictionary();
	Dictionary::Ptr my_master_zone = new Dictionary();
	Array::Ptr my_master_zone_members = new Array();

	my_master_endpoint->Set("__name", new ConfigIdentifier("NodeName"));
	my_master_endpoint->Set("__type", "Endpoint");

	my_master_zone_members->Add(new ConfigIdentifier("NodeName"));

	my_master_zone->Set("__name", new ConfigIdentifier("ZoneName"));
	my_master_zone->Set("__type", "Zone");
	my_master_zone->Set("endpoints", my_master_zone_members);

	/* store the local config */
	my_config->Add(my_master_endpoint);
	my_config->Add(my_master_zone);

	/* write the newly generated configuration */
	String zones_path = Application::GetSysconfDir() + "/icinga2/zones.conf";

	NodeUtility::WriteNodeConfigObjects(zones_path, my_config);

	return 0;
}

bool NodeUtility::WriteNodeConfigObjects(const String& filename, const Array::Ptr& objects)
{
	Log(LogInformation, "cli")
	    << "Dumping config items to file '" << filename << "'.";

	/* create a backup first */
	CreateBackupFile(filename);

	String path = Utility::DirName(filename);

	Utility::MkDirP(path, 0755);

	String user = ScriptGlobal::Get("RunAsUser");
	String group = ScriptGlobal::Get("RunAsGroup");

	if (!Utility::SetFileOwnership(path, user, group)) {
		Log(LogWarning, "cli")
		    << "Cannot set ownership for user '" << user << "' group '" << group << "' on path '" << path << "'. Verify it yourself!";
	}
	if (!Utility::SetFileOwnership(filename, user, group)) {
		Log(LogWarning, "cli")
		    << "Cannot set ownership for user '" << user << "' group '" << group << "' on path '" << path << "'. Verify it yourself!";
	}

	std::fstream fp;
	String tempFilename = Utility::CreateTempFile(filename + ".XXXXXX", 0644, fp);

	fp << "/*\n";
	fp << " * Generated by Icinga 2 node setup commands\n";
	fp << " * on " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", Utility::GetTime()) << "\n";
	fp << " */\n\n";

	ObjectLock olock(objects);
	for (const Dictionary::Ptr& object : objects) {
		SerializeObject(fp, object);
	}

	fp << std::endl;
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

/*
 * Black/Whitelist helpers
 */
String NodeUtility::GetBlackAndWhiteListPath(const String& type)
{
	return NodeUtility::GetRepositoryPath() + "/" + type + ".list";
}

Array::Ptr NodeUtility::GetBlackAndWhiteList(const String& type)
{
	String list_path = GetBlackAndWhiteListPath(type);

	Array::Ptr lists = new Array();

	if (Utility::PathExists(list_path)) {
		lists = Utility::LoadJsonFile(list_path);
	}

	return lists;
}

int NodeUtility::UpdateBlackAndWhiteList(const String& type, const String& zone_filter, const String& host_filter, const String& service_filter)
{
	Array::Ptr lists = GetBlackAndWhiteList(type);

	{
		ObjectLock olock(lists);

		for (const Dictionary::Ptr& filter : lists) {

			if (filter->Get("zone") == zone_filter) {
				if (filter->Get("host") == host_filter && service_filter.IsEmpty()) {
					Log(LogWarning, "cli")
					    << "Found zone filter '" << zone_filter << "' with host filter '" << host_filter << "'. Bailing out.";
					return 1;
				} else if (filter->Get("host") == host_filter && filter->Get("service") == service_filter) {
					Log(LogWarning, "cli")
					    << "Found zone filter '" << zone_filter << "' with host filter '" << host_filter << "' and service filter '"
					    << service_filter << "'. Bailing out.";
					return 1;
				}
			}
		}
	}

	Dictionary::Ptr new_filter = new Dictionary();

	new_filter->Set("zone", zone_filter);
	new_filter->Set("host", host_filter);
	new_filter->Set("service", service_filter);

	lists->Add(new_filter);

	String list_path = GetBlackAndWhiteListPath(type);
	CreateRepositoryPath();
	Utility::SaveJsonFile(list_path, 0600, lists);

	return 0;
}

int NodeUtility::RemoveBlackAndWhiteList(const String& type, const String& zone_filter, const String& host_filter, const String& service_filter)
{
	Array::Ptr lists = GetBlackAndWhiteList(type);

	std::vector<int> remove_filters;
	int remove_idx = 0;
	{
		ObjectLock olock(lists);

		for (const Dictionary::Ptr& filter : lists) {

			if (filter->Get("zone") == zone_filter) {
				if (filter->Get("host") == host_filter && service_filter.IsEmpty()) {
					Log(LogInformation, "cli")
					    << "Found zone filter '" << zone_filter << "' with host filter '" << host_filter << "'. Removing from " << type << ".";
					remove_filters.push_back(remove_idx);
				} else if (filter->Get("host") == host_filter && filter->Get("service") == service_filter) {
					Log(LogInformation, "cli")
					    << "Found zone filter '" << zone_filter << "' with host filter '" << host_filter << "' and service filter '"
					    << service_filter << "'. Removing from " << type << ".";
					remove_filters.push_back(remove_idx);
				}
			}

			remove_idx++;
		}
	}

	/* if there are no matches for reomval, throw an error */
	if (remove_filters.empty()) {
		Log(LogCritical, "cli", "Cannot remove filter!");
		return 1;
	}

	for (int remove : remove_filters) {
		lists->Remove(remove);
	}

	String list_path = GetBlackAndWhiteListPath(type);
	CreateRepositoryPath();
	Utility::SaveJsonFile(list_path, 0600, lists);

	return 0;
}

int NodeUtility::PrintBlackAndWhiteList(std::ostream& fp, const String& type)
{
	Array::Ptr lists = GetBlackAndWhiteList(type);

	if (lists->GetLength() == 0)
		return 0;

	fp << "Listing all " << type << " entries:\n";

	ObjectLock olock(lists);
	for (const Dictionary::Ptr& filter : lists) {
		fp << type << " filter for Node: '" << filter->Get("zone") << "' Host: '"
		    << filter->Get("host") << "' Service: '" << filter->Get("service") << "'.\n";
	}

	return 0;
}

bool NodeUtility::CheckAgainstBlackAndWhiteList(const String& type, const String& zone, const String& host, const String& service)
{
	Array::Ptr lists = GetBlackAndWhiteList(type);

	Log(LogNotice, "cli")
	    << "Checking object against " << type << ".";

	ObjectLock olock(lists);
	for (const Dictionary::Ptr& filter : lists) {
		String zone_filter = filter->Get("zone");
		String host_filter = filter->Get("host");
		String service_filter;

		if (filter->Contains("service"))
			service_filter = filter->Get("service");

		Log(LogNotice, "cli")
		    << "Checking Node '" << zone << "' =~ '" << zone_filter << "', host '" << host << "' =~ '" << host_filter
		    << "', service '" << service << "' =~ '" << service_filter << "'.";

		if (Utility::Match(zone_filter, zone)) {
			Log(LogNotice, "cli")
			    << "Node '" << zone << "' matches filter '" << zone_filter << "'";

			if (Utility::Match(host_filter, host)) {
				Log(LogNotice, "cli")
				    << "Host '" << host << "' matches filter '" << host_filter << "'";

				/* no service filter means host match */
				if (service_filter.IsEmpty())
					return true;

				if (Utility::Match(service_filter, service)) {
					Log(LogNotice, "cli")
					    << "Host '" << service << "' matches filter '" << service_filter << "'";
					return true;
				}
			}
		}
	}

	return false;
}

/*
 * We generally don't overwrite files without backup before
 */
bool NodeUtility::CreateBackupFile(const String& target, bool is_private)
{
	if (!Utility::PathExists(target))
		return false;

	String backup = target + ".orig";

	if (Utility::PathExists(backup)) {
		Log(LogWarning, "cli")
		    << "Backup file '" << backup << "' already exists. Skipping backup.";
		return false;
	}

	Utility::CopyFile(target, backup);

#ifndef _WIN32
	if (is_private)
		chmod(backup.CStr(), 0600);
#endif /* _WIN32 */

	Log(LogInformation, "cli")
	    << "Created backup file '" << backup << "'.";

	return true;
}

void NodeUtility::SerializeObject(std::ostream& fp, const Dictionary::Ptr& object)
{
	fp << "object ";
	ConfigWriter::EmitIdentifier(fp, object->Get("__type"), false);
	fp << " ";
	ConfigWriter::EmitValue(fp, 0, object->Get("__name"));
	fp << " {\n";

	ObjectLock olock(object);
	for (const Dictionary::Pair& kv : object) {
		if (kv.first == "__type" || kv.first == "__name")
			continue;

		fp << "\t";
		ConfigWriter::EmitIdentifier(fp, kv.first, true);
		fp << " = ";
		ConfigWriter::EmitValue(fp, 1, kv.second);
		fp << "\n";
	}

	fp << "}\n\n";
}

void NodeUtility::UpdateConstant(const String& name, const String& value)
{
	String constantsFile = Application::GetSysconfDir() + "/icinga2/constants.conf";

	std::ifstream ifp(constantsFile.CStr());
	std::fstream ofp;
	String tempFile = Utility::CreateTempFile(constantsFile + ".XXXXXX", 0644, ofp);

	bool found = false;

	Log(LogInformation, "cli")
	    << "Updating constants file '" << constantsFile << "'.";

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
