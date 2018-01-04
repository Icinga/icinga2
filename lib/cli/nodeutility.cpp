/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

String NodeUtility::GetConstantsConfPath()
{
	return Application::GetSysconfDir() + "/icinga2/constants.conf";
}

/*
 * Node Setup helpers
 */

int NodeUtility::GenerateNodeIcingaConfig(const std::vector<std::string>& endpoints, const std::vector<String>& globalZones)
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

	for (const String& globalzone : globalZones) {
		Dictionary::Ptr myGlobalZone = new Dictionary();

		myGlobalZone->Set("__name", globalzone);
		myGlobalZone->Set("__type", "Zone");
		myGlobalZone->Set("global", true);

		my_config->Add(myGlobalZone);
	}

	/* store the local config */
	my_config->Add(my_endpoint);
	my_config->Add(my_zone);

	/* write the newly generated configuration */
	String zones_path = Application::GetSysconfDir() + "/icinga2/zones.conf";

	NodeUtility::WriteNodeConfigObjects(zones_path, my_config);

	return 0;
}

int NodeUtility::GenerateNodeMasterIcingaConfig(const std::vector<String>& globalZones)
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

	for (const String& globalzone : globalZones) {
		Dictionary::Ptr myGlobalZone = new Dictionary();

		myGlobalZone->Set("__name", globalzone);
		myGlobalZone->Set("__type", "Zone");
		myGlobalZone->Set("global", true);

		my_config->Add(myGlobalZone);
	}

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
 * We generally don't overwrite files without backup before
 */
bool NodeUtility::CreateBackupFile(const String& target, bool is_private)
{
	if (!Utility::PathExists(target))
		return false;

	String backup = target + ".orig";

	if (Utility::PathExists(backup)) {
		Log(LogInformation, "cli")
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
	String constantsConfPath = NodeUtility::GetConstantsConfPath();

	Log(LogInformation, "cli")
		<< "Updating '" << name << "' constant in '" << constantsConfPath << "'.";

	NodeUtility::CreateBackupFile(constantsConfPath);

	std::ifstream ifp(constantsConfPath.CStr());
	std::fstream ofp;
	String tempFile = Utility::CreateTempFile(constantsConfPath + ".XXXXXX", 0644, ofp);

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
	_unlink(constantsConfPath.CStr());
#endif /* _WIN32 */

	if (rename(tempFile.CStr(), constantsConfPath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("rename")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(constantsConfPath));
	}
}
