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

#include "cli/repositoryutility.hpp"
#include "cli/clicommand.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/convert.hpp"
#include "base/configwriter.hpp"
#include "base/scriptglobal.hpp"
#include "base/json.hpp"
#include "base/netstring.hpp"
#include "base/tlsutility.hpp"
#include "base/stdiostream.hpp"
#include "base/debug.hpp"
#include "base/objectlock.hpp"
#include "base/console.hpp"
#include "base/serializer.hpp"
#include "base/exception.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/regex.hpp>
#include <fstream>
#include <iostream>

using namespace icinga;

Dictionary::Ptr RepositoryUtility::GetArgumentAttributes(const std::vector<std::string>& arguments)
{
	Dictionary::Ptr attrs = new Dictionary();

	for (const String& kv : arguments) {
		std::vector<String> tokens;
		boost::algorithm::split(tokens, kv, boost::is_any_of("="));

		if (tokens.size() != 2) {
			Log(LogWarning, "cli")
			    << "Cannot parse passed attributes: " << boost::algorithm::join(tokens, "=");
			continue;
		}

		Value value;

		try {
			value = Convert::ToDouble(tokens[1]);
		} catch (...) {
			value = tokens[1];
		}

		attrs->Set(tokens[0], value);
	}

	return attrs;
}

String RepositoryUtility::GetRepositoryConfigPath(void)
{
	return Application::GetSysconfDir() + "/icinga2/repository.d";
}

String RepositoryUtility::GetRepositoryObjectConfigPath(const String& type, const Dictionary::Ptr& object)
{
	String path = GetRepositoryConfigPath() + "/";

	if (type == "Host")
		path += "hosts";
	else if (type == "Service")
		path += "hosts/" + EscapeName(object->Get("host_name"));
	else if (type == "Zone")
		path += "zones";
	else if (type == "Endpoint")
		path += "endpoints";

	return path;
}

bool RepositoryUtility::FilterRepositoryObjects(const String& type, const String& path)
{
	if (type == "Host") {
		boost::regex expr("hosts/[^/]*.conf", boost::regex::icase);
		boost::smatch what;
		return boost::regex_search(path.GetData(), what, expr);
	}
	else if (type == "Service")
		return Utility::Match("*hosts/*/*.conf", path);
	else if (type == "Zone")
		return Utility::Match("*zones/*.conf", path);
	else if (type == "Endpoints")
		return Utility::Match("*endpoints/*.conf", path);

	return false;
}

String RepositoryUtility::GetRepositoryObjectConfigFilePath(const String& type, const Dictionary::Ptr& object)
{
	String path = GetRepositoryObjectConfigPath(type, object);

	path += "/" + EscapeName(object->Get("name")) + ".conf";

	return path;
}

String RepositoryUtility::GetRepositoryChangeLogPath(void)
{
	return Application::GetLocalStateDir() + "/lib/icinga2/repository/changes";
}

void RepositoryUtility::CreateRepositoryPath(const String& path)
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

/* printers */
void RepositoryUtility::PrintObjects(std::ostream& fp, const String& type)
{
	std::vector<String> objects = GetObjects(); //full path

	for (const String& object : objects) {
		if (!FilterRepositoryObjects(type, object)) {
			Log(LogDebug, "cli")
			    << "Ignoring object '" << object << "'. Type '" << type << "' does not match.";
			continue;
		}

		String file = Utility::BaseName(object);
		boost::algorithm::replace_all(file, ".conf", "");
		file = UnescapeName(file);

		fp << ConsoleColorTag(Console_ForegroundMagenta | Console_Bold) << type << ConsoleColorTag(Console_Normal)
		   << " '" << ConsoleColorTag(Console_ForegroundBlue | Console_Bold) << file << ConsoleColorTag(Console_Normal) << "'";

		String prefix = Utility::DirName(object);

		if (type == "Service") {
			std::vector<String> tokens;
			boost::algorithm::split(tokens, prefix, boost::is_any_of("/"));

			String host_name = UnescapeName(tokens[tokens.size()-1]);
			fp << " (on " << ConsoleColorTag(Console_ForegroundMagenta | Console_Bold) << "Host" << ConsoleColorTag(Console_Normal)
			   << " '" << ConsoleColorTag(Console_ForegroundBlue | Console_Bold) << host_name << ConsoleColorTag(Console_Normal) << "')";

		}

		fp << "\n";
	}
}

void RepositoryUtility::PrintChangeLog(std::ostream& fp)
{
	Array::Ptr changelog = new Array();

	GetChangeLog(boost::bind(RepositoryUtility::CollectChange, _1, changelog));

	ObjectLock olock(changelog);

	std::cout << "Changes to be committed:\n\n";

	for (const Value& entry : changelog) {
		FormatChangelogEntry(std::cout, entry);
	}
}

class RepositoryValidationUtils : public ValidationUtils
{
public:
	virtual bool ValidateName(const String& type, const String& name) const
	{
		return true;
	}
};

/* modify objects and write changelog */
bool RepositoryUtility::AddObject(const std::vector<String>& object_paths, const String& name, const String& type,
    const Dictionary::Ptr& attrs, const Array::Ptr& changes, bool check_config)
{
	String pattern;

	if (type == "Service")
		pattern = EscapeName(attrs->Get("host_name")) + "/" + EscapeName(name) + ".conf";
	else
		pattern = EscapeName(name) + ".conf";

	for (const String& object_path : object_paths) {
		if (object_path.Contains(pattern)) {
			Log(LogWarning, "cli")
			    << type << " '" << name << "' already exists. Skipping creation.";
			return false;
		}
	}

	/* add a new changelog entry by timestamp */
	String path = GetRepositoryChangeLogPath() + "/" + Convert::ToString(Utility::GetTime()) + "-" + type + "-" + SHA256(name) + ".change";

	Dictionary::Ptr change = new Dictionary();

	change->Set("timestamp", Utility::GetTime());
	change->Set("name", name);
	change->Set("type", type);
	change->Set("command", "add");
	change->Set("attrs", attrs);

	Type::Ptr utype = Type::GetByName(type);
	ASSERT(utype);

	if (check_config) {
		try {
			ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(utype->Instantiate(std::vector<Value>()));
			/* temporarly set the object type for validation */
			attrs->Set("type", utype->GetName());
			Deserialize(object, attrs, false, FAConfig);
			object->SetName(name);

			RepositoryValidationUtils utils;
			static_pointer_cast<ConfigObject>(object)->Validate(FAConfig, utils);

			attrs->Remove("type");
		} catch (const ValidationError& ex) {
			Log(LogCritical, "config", DiagnosticInformation(ex));
			return false;
		}
	}

	if (CheckChangeExists(change, changes)) {
		Log(LogWarning, "cli")
		    << "Change '" << change->Get("command") << "' for type '"
		    << change->Get("type") << "' and name '" << change->Get("name")
		    << "' already exists.";

		return false;
	}

	/* store the cached change */
	changes->Add(change);

	return WriteObjectToRepositoryChangeLog(path, change);
}

bool RepositoryUtility::RemoveObject(const String& name, const String& type, const Dictionary::Ptr& attrs, const Array::Ptr& changes)
{
	/* add a new changelog entry by timestamp */
	String path = GetRepositoryChangeLogPath() + "/" + Convert::ToString(Utility::GetTime()) + "-" + type + "-" + SHA256(name) + ".change";

	Dictionary::Ptr change = new Dictionary();

	change->Set("timestamp", Utility::GetTime());
	change->Set("name", name);
	change->Set("type", type);
	change->Set("command", "remove");
	change->Set("attrs", attrs); //required for service->host_name

	if (CheckChangeExists(change, changes)) {
		Log(LogWarning, "cli")
		    << "Change '" << change->Get("command") << "' for type '"
		    << change->Get("type") << "' and name '" << change->Get("name")
		    << "' already exists.";

		return false;
	}

	/* store the cached change */
	changes->Add(change);

	return WriteObjectToRepositoryChangeLog(path, change);
}

bool RepositoryUtility::SetObjectAttribute(const String& name, const String& type, const String& attr, const Value& val)
{
	//TODO: Implement modification commands
	return true;
}

bool RepositoryUtility::CheckChangeExists(const Dictionary::Ptr& change, const Array::Ptr& changes)
{
	Dictionary::Ptr attrs = change->Get("attrs");

	ObjectLock olock(changes);
	for (const Dictionary::Ptr& entry : changes) {
		if (entry->Get("type") != change->Get("type"))
			continue;

		if (entry->Get("name") != change->Get("name"))
			continue;

		Dictionary::Ptr their_attrs = entry->Get("attrs");

		if (entry->Get("type") == "Service" && attrs->Get("host_name") != their_attrs->Get("host_name"))
			continue;

		if (entry->Get("command") != change->Get("command"))
			continue;

		/* only works for add/remove commands (no set) */
		if (change->Get("command") == "add" || change->Get("command") == "remove")
			return true;
	}

	return false;
}

bool RepositoryUtility::ClearChangeLog(void)
{
	GetChangeLog(boost::bind(RepositoryUtility::ClearChange, _1, _2));

	return true;
}

bool RepositoryUtility::ChangeLogHasPendingChanges(void)
{
	Array::Ptr changelog = new Array();
	GetChangeLog(boost::bind(RepositoryUtility::CollectChange, _1, changelog));

	return changelog->GetLength() > 0;
}

/* commit changelog */
bool RepositoryUtility::CommitChangeLog(void)
{
	GetChangeLog(boost::bind(RepositoryUtility::CommitChange, _1, _2));

	return true;
}

/* write/read from changelog repository */
bool RepositoryUtility::WriteObjectToRepositoryChangeLog(const String& path, const Dictionary::Ptr& item)
{
	Log(LogInformation, "cli", "Dumping changelog items to file '" + path + "'");

	CreateRepositoryPath(Utility::DirName(path));

	std::fstream fp;
	String tempFilename = Utility::CreateTempFile(path + ".XXXXXX", 0600, fp);

	fp << JsonEncode(item);
	fp.close();

#ifdef _WIN32
	_unlink(path.CStr());
#endif /* _WIN32 */

	if (rename(tempFilename.CStr(), path.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(tempFilename));
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

/* internal implementation when changes are committed */
bool RepositoryUtility::AddObjectInternal(const String& name, const String& type, const Dictionary::Ptr& attrs)
{
	String path = GetRepositoryObjectConfigPath(type, attrs) + "/" + EscapeName(name) + ".conf";

	return WriteObjectToRepository(path, name, type, attrs);
}

bool RepositoryUtility::RemoveObjectInternal(const String& name, const String& type, const Dictionary::Ptr& attrs)
{
	String path = GetRepositoryObjectConfigPath(type, attrs) + "/" + EscapeName(name) + ".conf";

	if (!Utility::PathExists(path)) {
		Log(LogWarning, "cli")
		    << type << " '" << name << "' does not exist.";
		return true;
	}

	bool success = RemoveObjectFileInternal(path);

	if (success)
		Log(LogInformation, "cli")
		    << "Removing config object '" << name << "' in file '" << path << "'";

	/* special treatment for hosts -> remove the services too */
	if (type == "Host") {
		path = GetRepositoryObjectConfigPath(type, attrs) + "/" + name;

		/* if path does not exist, this host does not have any services */
		if (!Utility::PathExists(path)) {
			Log(LogNotice, "cli")
			    << type << " '" << name << "' does not have any services configured.";
			return success;
		}

		std::vector<String> files;

		Utility::GlobRecursive(path, "*.conf",
		    boost::bind(&RepositoryUtility::CollectObjects, _1, boost::ref(files)), GlobFile);


		for (const String& file : files) {
			RemoveObjectFileInternal(file);
		}
#ifndef _WIN32
		rmdir(path.CStr());
#else
		_rmdir(path.CStr());
#endif /* _WIN32 */

	}

	return success;
}

bool RepositoryUtility::RemoveObjectFileInternal(const String& path)
{
	if (!Utility::PathExists(path) ) {
		Log(LogCritical, "cli", "Cannot remove '" + path + "'. Does not exist.");
		return false;
	}

	if (unlink(path.CStr()) < 0) {
		Log(LogCritical, "cli", "Cannot remove path '" + path +
		    "'. Failed with error code " + Convert::ToString(errno) + ", \"" + Utility::FormatErrorNumber(errno) + "\".");
		return false;
	}

	return true;
}

bool RepositoryUtility::SetObjectAttributeInternal(const String& name, const String& type, const String& key, const Value& val, const Dictionary::Ptr& attrs)
{
	//TODO
	String path = GetRepositoryObjectConfigPath(type, attrs) + "/" + EscapeName(name) + ".conf";

	Dictionary::Ptr obj = GetObjectFromRepository(path); //TODO

	if (!obj) {
		Log(LogCritical, "cli")
		    << "Can't get object " << name << " from repository.\n";
		return false;
	}

	obj->Set(key, val);

	std::cout << "Writing object '" << name << "' to path '" << path << "'.\n";

	//TODO: Create a patch file
	if (!WriteObjectToRepository(path, name, type, obj)) {
		Log(LogCritical, "cli")
		    << "Can't write object " << name << " to repository.\n";
		return false;
	}

	return true;
}

bool RepositoryUtility::WriteObjectToRepository(const String& path, const String& name, const String& type, const Dictionary::Ptr& item)
{
	Log(LogInformation, "cli")
	    << "Writing config object '" << name << "' to file '" << path << "'";

	CreateRepositoryPath(Utility::DirName(path));

	std::fstream fp;
	String tempFilename = Utility::CreateTempFile(path + ".XXXXXX", 0644, fp);

	SerializeObject(fp, name, type, item);
	fp << std::endl;
	fp.close();

#ifdef _WIN32
	_unlink(path.CStr());
#endif /* _WIN32 */

	if (rename(tempFilename.CStr(), path.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(tempFilename));
	}

	return true;
}

Dictionary::Ptr RepositoryUtility::GetObjectFromRepository(const String& filename)
{
	//TODO: Parse existing configuration objects
	return Dictionary::Ptr();
}

String RepositoryUtility::EscapeName(const String& name)
{
	return Utility::EscapeString(name, "<>:\"/\\|?*", true);
}

String RepositoryUtility::UnescapeName(const String& name)
{
	return Utility::UnescapeString(name);
}

/*
 * collect functions
 */
std::vector<String> RepositoryUtility::GetObjects(void)
{
	std::vector<String> objects;
	String path = GetRepositoryConfigPath();

	Utility::GlobRecursive(path, "*.conf",
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

	Utility::MkDirP(path, 0700);

	Utility::Glob(path + "/*.change",
	    boost::bind(&RepositoryUtility::CollectChangeLog, _1, boost::ref(changelog)), GlobFile);

	/* sort by timestamp ascending */
	std::sort(changelog.begin(), changelog.end());

	for (const String& entry : changelog) {
		String file = path + entry + ".change";
		Dictionary::Ptr change = GetObjectFromRepositoryChangeLog(file);

		Log(LogDebug, "cli")
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

	Log(LogDebug, "cli")
	    << "Adding change file: '" << file << "'.";

	changelog.push_back(file);
}

void RepositoryUtility::CollectChange(const Dictionary::Ptr& change, Array::Ptr& changes)
{
	changes->Add(change);
}

/*
 * Commit Changelog entry
 */
void RepositoryUtility::CommitChange(const Dictionary::Ptr& change, const String& path)
{
	Log(LogDebug, "cli")
	   << "Got change " << change->Get("name");

	String name = change->Get("name");
	String type = change->Get("type");
	String command = change->Get("command");
	Dictionary::Ptr attrs;

	if (change->Contains("attrs"))
		attrs = change->Get("attrs");

	bool success = false;

	if (command == "add")
		success = AddObjectInternal(name, type, attrs);
	else if (command == "remove")
		success = RemoveObjectInternal(name, type, attrs);

	if (success) {
		Log(LogNotice, "cli")
		    << "Removing changelog file '" << path << "'.";
		RemoveObjectFileInternal(path);
	}
}

/*
 * Clear Changelog entry
 */
void RepositoryUtility::ClearChange(const Dictionary::Ptr& change, const String& path)
{
	Log(LogDebug, "cli")
	   << "Clearing change " << change->Get("name");

	Log(LogInformation, "cli")
	   << "Removing changelog file '" << path << "'.";

	RemoveObjectFileInternal(path);
}

/*
 * Print Changelog helpers
 */
void RepositoryUtility::FormatChangelogEntry(std::ostream& fp, const Dictionary::Ptr& change)
{
	if (!change)
		return;

	if (change->Get("command") == "add")
		fp << "Adding";
	if (change->Get("command") == "remove")
		fp << "Removing";

	String type = change->Get("type");
	boost::algorithm::to_lower(type);
	Dictionary::Ptr attrs = change->Get("attrs");

	fp << " " << ConsoleColorTag(Console_ForegroundMagenta | Console_Bold) << type << ConsoleColorTag(Console_Normal) << " '";
	fp << ConsoleColorTag(Console_ForegroundBlue | Console_Bold) << change->Get("name") << ConsoleColorTag(Console_Normal) << "'\n";

	ObjectLock olock(attrs);
	for (const Dictionary::Pair& kv : attrs) {
		/* skip the name */
		if (kv.first == "name" || kv.first == "__name")
			continue;

		fp << std::setw(4) << " " << ConsoleColorTag(Console_ForegroundGreen) << kv.first << ConsoleColorTag(Console_Normal) << " = ";
		ConfigWriter::EmitValue(fp, 0, kv.second);
		fp << "\n";
	}
}

/*
 * print helpers for configuration
 * TODO: Move into a separate class
 */
void RepositoryUtility::SerializeObject(std::ostream& fp, const String& name, const String& type, const Dictionary::Ptr& object)
{
	fp << "object " << type << " ";
	ConfigWriter::EmitString(fp, name);
 	fp << " {\n";

	if (!object) {
		fp << "}\n";
		return;
	}

	if (object->Contains("import")) {
		Array::Ptr imports = object->Get("import");

		ObjectLock olock(imports);
		for (const String& import : imports) {
			fp << "\t" << "import ";
			ConfigWriter::EmitString(fp, import);
			fp << '\n';
		}
	}

	ObjectLock xlock(object);
	for (const Dictionary::Pair& kv : object) {
		if (kv.first == "import" || kv.first == "name" || kv.first == "__name") {
			continue;
		} else {
			fp << "\t";
			ConfigWriter::EmitIdentifier(fp, kv.first, true);
			fp << " = ";
			ConfigWriter::EmitValue(fp, 1, kv.second);
		}
		fp << "\n";
	}
	fp << "}\n";
}
