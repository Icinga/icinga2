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

#include "cli/repositoryutility.hpp"
#include "cli/clicommand.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/convert.hpp"
#include "base/json.hpp"
#include "base/netstring.hpp"
#include "base/stdiostream.hpp"
#include "base/debug.hpp"
#include "base/objectlock.hpp"
#include "base/console.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <fstream>
#include <iostream>

using namespace icinga;

Dictionary::Ptr RepositoryUtility::GetArgumentAttributes(const std::vector<std::string>& arguments)
{
	Dictionary::Ptr attr = make_shared<Dictionary>();

	BOOST_FOREACH(const String& kv, arguments) {
		std::vector<String> tokens;
		boost::algorithm::split(tokens, kv, boost::is_any_of("="));

		if (tokens.size() == 2) {
			attr->Set(tokens[0], tokens[1]);
		} else
			Log(LogWarning, "cli")
			    << "Cannot parse passed attributes: " << boost::algorithm::join(tokens, "=");
	}

	return attr;
}

String RepositoryUtility::GetRepositoryConfigPath(void)
{
	return Application::GetSysconfDir() + "/icinga2/repository.d";
}

String RepositoryUtility::GetRepositoryObjectConfigPath(const String& type, const Dictionary::Ptr& object)
{
	String path = GetRepositoryConfigPath() + "/";

	if (type == "Host") {
		path += "hosts";
	}
	else if (type == "Service")
		path += "hosts/" + object->Get("host_name");
	else if (type == "Zone")
		path += "zones";
	else if (type == "Endpoints")
		path += "endpoints";

	return path;
}

String RepositoryUtility::GetRepositoryObjectConfigFilePath(const String& type, const Dictionary::Ptr& object)
{
	String path = GetRepositoryObjectConfigPath(type, object);

	path += "/" + object->Get("name") + ".conf";

	return path;
}

String RepositoryUtility::GetRepositoryChangeLogPath(void)
{
	return Application::GetLocalStateDir() + "/lib/icinga2/repository/changes";
}

void RepositoryUtility::PrintObjects(std::ostream& fp, const String& type)
{
	std::vector<String> objects = GetObjects(); //full path

	BOOST_FOREACH(const String& object, objects) {
		Dictionary::Ptr obj = GetObjectFromRepository(object);

		if (obj) {
			fp << "Object Name: " << object << "\n";
			fp << JsonEncode(obj);
		}
	}
}

/* public interface, only logs changes */
bool RepositoryUtility::AddObject(const String& name, const String& type, const Dictionary::Ptr& attr)
{
	/* add a new changelog entry by timestamp */
	String path = GetRepositoryChangeLogPath() + "/" + Convert::ToString(static_cast<long>(Utility::GetTime())) + ".change";

	Dictionary::Ptr change = make_shared<Dictionary>();

	change->Set("timestamp", Utility::GetTime());
	change->Set("name", name);
	change->Set("type", type);
	change->Set("command", "add");
	change->Set("attr", attr);

	return WriteObjectToRepositoryChangeLog(path, change);
}

bool RepositoryUtility::RemoveObject(const String& name, const String& type, const Dictionary::Ptr& attr)
{
	/* add a new changelog entry by timestamp */
	String path = GetRepositoryChangeLogPath() + "/" + Convert::ToString(static_cast<long>(Utility::GetTime())) + ".change";

	Dictionary::Ptr change = make_shared<Dictionary>();

	change->Set("timestamp", Utility::GetTime());
	change->Set("name", name);
	change->Set("type", type);
	change->Set("command", "remove");
	change->Set("attr", attr); //required for service->host_name

	return WriteObjectToRepositoryChangeLog(path, change);
}

bool RepositoryUtility::CommitChangeLog(void)
{
	GetChangeLog(boost::bind(RepositoryUtility::CommitChange, _1, _2));

	return true;
}

void RepositoryUtility::PrintChangeLog(std::ostream& fp)
{
	Array::Ptr changelog = make_shared<Array>();

	GetChangeLog(boost::bind(RepositoryUtility::CollectChange, _1, boost::ref(changelog)));

	ObjectLock olock(changelog);

	std::cout << "Changes to be committed:\n";

	BOOST_FOREACH(const Value& entry, changelog) {
		std::cout << JsonEncode(entry) << "\n"; //TODO better formatting
	}
}

bool RepositoryUtility::SetObjectAttribute(const String& name, const String& type, const String& attr, const Value& val)
{
	//TODO: Implement modification commands
	return true;
}

/* internal implementation when changes are committed */
bool RepositoryUtility::AddObjectInternal(const String& name, const String& type, const Dictionary::Ptr& attr)
{
	String path = GetRepositoryObjectConfigPath(type, attr) + "/" + name + ".conf";

	return WriteObjectToRepository(path, name, type, attr);
}

bool RepositoryUtility::RemoveObjectInternal(const String& name, const String& type, const Dictionary::Ptr& attr)
{
	String path = GetRepositoryObjectConfigPath(type, attr) + "/" + name + ".conf";

	return RemoveObjectFileInternal(path);
}

bool RepositoryUtility::RemoveObjectFileInternal(const String& path)
{
	if (!Utility::PathExists(path) ) {
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

bool RepositoryUtility::SetObjectAttributeInternal(const String& name, const String& type, const String& key, const Value& val, const Dictionary::Ptr& attr)
{
	//Fixme
	String path = GetRepositoryObjectConfigPath(type, attr) + "/" + name + ".conf";

	Dictionary::Ptr obj = GetObjectFromRepository(path);

	if (!obj) {
		Log(LogCritical, "cli")
		    << "Can't get object " << name << " from repository.\n";
		return false;
	}

	obj->Set(key, val);

	std::cout << "Writing object '" << name << "' to path '" << path << "'.\n";

	//TODO: Create a patch file
	if(!WriteObjectToRepository(path, name, type, obj)) {
		Log(LogCritical, "cli")
		    << "Can't write object " << name << " to repository.\n";
		return false;
	}

	return true;
}

bool RepositoryUtility::WriteObjectToRepository(const String& path, const String& name, const String& type, const Dictionary::Ptr& item)
{
	Log(LogInformation, "cli", "Dumping config items to file '" + path + "'");

	Utility::MkDirP(Utility::DirName(path), 0755);

	String tempPath = path + ".tmp";

        std::ofstream fp(tempPath.CStr(), std::ofstream::out | std::ostream::trunc);
        SerializeObject(fp, name, type, item);
	fp << std::endl;
        fp.close();

#ifdef _WIN32
	_unlink(path.CStr());
#endif /* _WIN32 */

	if (rename(tempPath.CStr(), path.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(tempPath));
	}

	return true;
}

Dictionary::Ptr RepositoryUtility::GetObjectFromRepository(const String& filename)
{
	//TODO: Parse existing configuration objects
	return Dictionary::Ptr();
}

bool RepositoryUtility::WriteObjectToRepositoryChangeLog(const String& path, const Dictionary::Ptr& item)
{
	Log(LogInformation, "cli", "Dumping changelog items to file '" + path + "'");

	Utility::MkDirP(Utility::DirName(path), 0750);

	String tempPath = path + ".tmp";

        std::ofstream fp(tempPath.CStr(), std::ofstream::out | std::ostream::trunc);
        fp << JsonEncode(item);
        fp.close();

#ifdef _WIN32
	_unlink(path.CStr());
#endif /* _WIN32 */

	if (rename(tempPath.CStr(), path.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(tempPath));
	}

	return true;
}

Dictionary::Ptr RepositoryUtility::GetObjectFromRepositoryChangeLog(const String& filename)
{
	std::fstream fp;
	fp.open(filename.CStr(), std::ifstream::in);

	if (!fp)
		return Dictionary::Ptr();

	String content((std::istreambuf_iterator<char>(fp)), std::istreambuf_iterator<char>());

	fp.close();

	return JsonDecode(content);
}

/*
 * collect functions
 */
std::vector<String> RepositoryUtility::GetObjects(void)
{
	std::vector<String> objects;
	String path = GetRepositoryConfigPath() + "/";

	Utility::Glob(path + "/*.conf",
	    boost::bind(&RepositoryUtility::CollectObjects, _1, boost::ref(objects)), GlobFile);

	return objects;
}

void RepositoryUtility::CollectObjects(const String& object_file, std::vector<String>& objects)
{
	Log(LogDebug, "cli")
	    << "Adding object: '" << object_file << "'.";

	objects.push_back(object_file);
}


bool RepositoryUtility::GetChangeLog(const boost::function<void (const Dictionary::Ptr&, const String&)>& callback)
{
	std::vector<String> changelog;
	String path = GetRepositoryChangeLogPath() + "/";

	Utility::Glob(path + "/*.change",
	    boost::bind(&RepositoryUtility::CollectChangeLog, _1, boost::ref(changelog)), GlobFile);

	/* sort by timestamp ascending */
	std::sort(changelog.begin(), changelog.end());

	BOOST_FOREACH(const String& entry, changelog) {
		String file = path + entry + ".change";
		Dictionary::Ptr change = GetObjectFromRepositoryChangeLog(file);

		Log(LogInformation, "cli")
		    << "Collecting entry " << entry << "\n";

		if (change)
			callback(change, file);
	}

	return true;
}

void RepositoryUtility::CollectChangeLog(const String& change_file, std::vector<String>& changelog)
{
	String file = Utility::BaseName(change_file);
	boost::algorithm::replace_all(file, ".change", "");

	Log(LogDebug, "cli", "Adding change file: " + file);
	changelog.push_back(file);
}

void RepositoryUtility::CommitChange(const Dictionary::Ptr& change, const String& path)
{
	Log(LogInformation, "cli")
	   << "Got change " << change->Get("name");

	String name = change->Get("name");
	String type = change->Get("type");
	String command = change->Get("command");
	Dictionary::Ptr attr;

	if (change->Contains("attr")) {
		attr = change->Get("attr");
	}

	bool success = false;

	if (command == "add") {
		success = AddObjectInternal(name, type, attr);
	}
	else if (command == "remove") {
		success = RemoveObjectInternal(name, type, attr);
	}

	if (success) {
		Log(LogInformation, "cli")
		    << "Removing changelog file '" << path << "'.";
		RemoveObjectFileInternal(path);
	}
}

void RepositoryUtility::CollectChange(const Dictionary::Ptr& change, Array::Ptr& changes)
{
	changes->Add(change);
}


/*
 * print helpers for configuration
 * TODO: Move into a separate class
 */
void RepositoryUtility::SerializeObject(std::ostream& fp, const String& name, const String& type, const Dictionary::Ptr& object)
{
	fp << "object " << type << " \"" << name << "\" {\n";

	if (!object) {
		fp << "}\n";
		return;
	}

	if (object->Contains("import")) {
		Array::Ptr imports = object->Get("import");

		ObjectLock olock(imports);
		BOOST_FOREACH(const String& import, imports) {
			fp << "\t" << "import \"" << import << "\"\n";
		}
	}

	BOOST_FOREACH(const Dictionary::Pair& kv, object) {
		if (kv.first == "import" || kv.first == "name") {
			continue;
		} else {
			fp << "\t" << kv.first << " = ";
			FormatValue(fp, kv.second);
		}
		fp << "\n";
	}
	fp << "}\n";
}

void RepositoryUtility::FormatValue(std::ostream& fp, const Value& val)
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

void RepositoryUtility::FormatArray(std::ostream& fp, const Array::Ptr& arr)
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
