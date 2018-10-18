/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>
#include <iostream>

using namespace icinga;

String NodeUtility::GetConstantsConfPath()
{
	return Configuration::ConfigDir + "/constants.conf";
}

String NodeUtility::GetZonesConfPath()
{
	return Configuration::ConfigDir + "/zones.conf";
}

/*
 * Node Setup helpers
 */

int NodeUtility::GenerateNodeIcingaConfig(const String& endpointName, const String& zoneName,
	const String& parentZoneName, const std::vector<std::string>& endpoints,
	const std::vector<String>& globalZones)
{
	Array::Ptr config = new Array();

	Array::Ptr myParentZoneMembers = new Array();

	for (const String& endpoint : endpoints) {
		/* extract all --endpoint arguments and store host,port info */
		std::vector<String> tokens = endpoint.Split(",");

		Dictionary::Ptr myParentEndpoint = new Dictionary();

		if (tokens.size() > 1) {
			String host = tokens[1].Trim();

			if (!host.IsEmpty())
				myParentEndpoint->Set("host", host);
		}

		if (tokens.size() > 2) {
			String port = tokens[2].Trim();

			if (!port.IsEmpty())
				myParentEndpoint->Set("port", port);
		}

		String myEndpointName = tokens[0].Trim();
		myParentEndpoint->Set("__name", myEndpointName);
		myParentEndpoint->Set("__type", "Endpoint");

		/* save endpoint in master zone */
		myParentZoneMembers->Add(myEndpointName);

		config->Add(myParentEndpoint);
	}

	/* add the parent zone to the config */
	config->Add(new Dictionary({
		{ "__name", parentZoneName },
		{ "__type", "Zone" },
		{ "endpoints", myParentZoneMembers }
	}));

	/* store the local generated node configuration */
	config->Add(new Dictionary({
		{ "__name", endpointName },
		{ "__type", "Endpoint" }
	}));

	config->Add(new Dictionary({
		{ "__name", zoneName },
		{ "__type", "Zone" },
		{ "parent", parentZoneName },
		{ "endpoints", new Array({ endpointName }) }
	}));

	for (const String& globalzone : globalZones) {
		config->Add(new Dictionary({
			{ "__name", globalzone },
			{ "__type", "Zone" },
			{ "global", true }
		}));
	}

	/* Write the newly generated configuration. */
	NodeUtility::WriteNodeConfigObjects(GetZonesConfPath(), config);

	return 0;
}

int NodeUtility::GenerateNodeMasterIcingaConfig(const String& endpointName, const String& zoneName,
	const std::vector<String>& globalZones)
{
	Array::Ptr config = new Array();

	/* store the local generated node master configuration */
	config->Add(new Dictionary({
		{ "__name", endpointName },
		{ "__type", "Endpoint" }
	}));

	config->Add(new Dictionary({
		{ "__name", zoneName },
		{ "__type", "Zone" },
		{ "endpoints", new Array({ endpointName }) }
	}));

	for (const String& globalzone : globalZones) {
		config->Add(new Dictionary({
			{ "__name", globalzone },
			{ "__type", "Zone" },
			{ "global", true }
		}));
	}

	/* Write the newly generated configuration. */
	NodeUtility::WriteNodeConfigObjects(GetZonesConfPath(), config);

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

	String user = Configuration::RunAsUser;
	String group = Configuration::RunAsGroup;

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
bool NodeUtility::CreateBackupFile(const String& target, bool isPrivate)
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
	if (isPrivate)
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

/*
 * include = false, will comment out the include statement
 * include = true, will add an include statement or uncomment a statement if one is existing
 * resursive = false, will search for a non-resursive include statement
 * recursive = true, will search for a resursive include statement
 * Returns true on success, false if option was not found
 */
bool NodeUtility::UpdateConfiguration(const String& value, bool include, bool recursive)
{
	String configurationFile = Configuration::ConfigDir + "/icinga2.conf";

	Log(LogInformation, "cli")
		<< "Updating '" << value << "' include in '" << configurationFile << "'.";

	NodeUtility::CreateBackupFile(configurationFile);

	std::ifstream ifp(configurationFile.CStr());
	std::fstream ofp;
	String tempFile = Utility::CreateTempFile(configurationFile + ".XXXXXX", 0644, ofp);

	String affectedInclude = value;

	if (recursive)
		affectedInclude = "include_recursive " + affectedInclude;
	else
		affectedInclude = "include " + affectedInclude;

	bool found = false;

	std::string line;

	while (std::getline(ifp, line)) {
		if (include) {
			if (line.find("//" + affectedInclude) != std::string::npos || line.find("// " + affectedInclude) != std::string::npos) {
				found = true;
				ofp << "// Added by the node setup CLI command on "
					<< Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", Utility::GetTime())
					<< "\n" + affectedInclude + "\n";
			} else if (line.find(affectedInclude) != std::string::npos) {
				found = true;

				Log(LogInformation, "cli")
					<< "Include statement '" + affectedInclude + "' already set.";

				ofp << line << "\n";
			} else {
				ofp << line << "\n";
			}
		} else {
			if (line.find(affectedInclude) != std::string::npos) {
				found = true;
				ofp << "// Disabled by the node setup CLI command on "
					<< Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", Utility::GetTime())
					<< "\n// " + affectedInclude + "\n";
			} else {
				ofp << line << "\n";
			}
		}
	}

	if (include && !found) {
		ofp << "// Added by the node setup CLI command on "
			<< Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", Utility::GetTime())
			<< "\n" + affectedInclude + "\n";
	}

	ifp.close();
	ofp.close();

#ifdef _WIN32
	_unlink(configurationFile.CStr());
#endif /* _WIN32 */

	if (rename(tempFile.CStr(), configurationFile.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("rename")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(configurationFile));
	}

	return (found || include);
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
