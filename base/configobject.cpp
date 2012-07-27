/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "i2-base.h"

using namespace icinga;

map<pair<string, string>, Dictionary::Ptr> ConfigObject::m_PersistentTags;
boost::signal<void (const ConfigObject::Ptr&)> ConfigObject::OnCommitted;
boost::signal<void (const ConfigObject::Ptr&)> ConfigObject::OnRemoved;

ConfigObject::ConfigObject(const Dictionary::Ptr& properties)
	: m_Properties(properties), m_Tags(boost::make_shared<Dictionary>())
{
	/* restore the object's tags */
	map<pair<string, string>, Dictionary::Ptr>::iterator it;
	it = m_PersistentTags.find(make_pair(GetType(), GetName()));
	if (it != m_PersistentTags.end()) {
		m_Tags = it->second;
		m_PersistentTags.erase(it);
	}
}

void ConfigObject::SetProperties(const Dictionary::Ptr& properties)
{
	m_Properties = properties;
}

Dictionary::Ptr ConfigObject::GetProperties(void) const
{
	return m_Properties;
}

void ConfigObject::SetTags(const Dictionary::Ptr& tags)
{
	m_Tags = tags;
}

Dictionary::Ptr ConfigObject::GetTags(void) const
{
	return m_Tags;
}

string ConfigObject::GetType(void) const
{
	string type;
	GetProperties()->Get("__type", &type);
	return type;
}

string ConfigObject::GetName(void) const
{
	string name;
	GetProperties()->Get("__name", &name);
	return name;
}

bool ConfigObject::IsLocal(void) const
{
	bool value = false;
	GetProperties()->Get("__local", &value);
	return value;
}

bool ConfigObject::IsAbstract(void) const
{
	bool value = false;
	GetProperties()->Get("__abstract", &value);
	return value;
}

void ConfigObject::SetSource(const string& value)
{
	GetProperties()->Set("__source", value);
}

string ConfigObject::GetSource(void) const
{
	string value;
	GetProperties()->Get("__source", &value);
	return value;
}

void ConfigObject::SetCommitTimestamp(double ts)
{
	GetProperties()->Set("__tx", ts);
}

double ConfigObject::GetCommitTimestamp(void) const
{
	double value = 0;
	GetProperties()->Get("__tx", &value);
	return value;
}

void ConfigObject::Commit(void)
{
	assert(Application::IsMainThread());

	ConfigObject::Ptr dobj = GetObject(GetType(), GetName());
	ConfigObject::Ptr self = GetSelf();
	assert(!dobj || dobj == self);

	pair<ConfigObject::TypeMap::iterator, bool> ti;
	ti = GetAllObjects().insert(make_pair(GetType(), ConfigObject::NameMap()));
	ti.first->second.insert(make_pair(GetName(), GetSelf()));

	SetCommitTimestamp(Utility::GetTime());

	OnCommitted(GetSelf());
}

void ConfigObject::Unregister(void)
{
	assert(Application::IsMainThread());

	ConfigObject::TypeMap::iterator tt;
	tt = GetAllObjects().find(GetType());

	if (tt == GetAllObjects().end())
		return;

	ConfigObject::NameMap::iterator nt = tt->second.find(GetName());

	if (nt == tt->second.end())
		return;

	tt->second.erase(nt);

	OnRemoved(GetSelf());
}

ConfigObject::Ptr ConfigObject::GetObject(const string& type, const string& name)
{
	ConfigObject::TypeMap::iterator tt;
	tt = GetAllObjects().find(type);

	if (tt == GetAllObjects().end())
		return ConfigObject::Ptr();

	ConfigObject::NameMap::iterator nt = tt->second.find(name);              

	if (nt == tt->second.end())
		return ConfigObject::Ptr();

	return nt->second;
}

pair<ConfigObject::TypeMap::iterator, ConfigObject::TypeMap::iterator> ConfigObject::GetTypes(void)
{
	return make_pair(GetAllObjects().begin(), GetAllObjects().end());
}

pair<ConfigObject::NameMap::iterator, ConfigObject::NameMap::iterator> ConfigObject::GetObjects(const string& type)
{
	pair<ConfigObject::TypeMap::iterator, bool> ti;
	ti = GetAllObjects().insert(make_pair(type, ConfigObject::NameMap()));

	return make_pair(ti.first->second.begin(), ti.first->second.end());
}

void ConfigObject::RemoveTag(const string& key)
{
	GetTags()->Remove(key);
}

ScriptTask::Ptr ConfigObject::InvokeMethod(const string& method,
    const vector<Variant>& arguments, ScriptTask::CompletionCallback callback)
{
	Dictionary::Ptr methods;
	string funcName;
	if (!GetProperty("methods", &methods) || !methods->Get(method, &funcName))
		return ScriptTask::Ptr();

	ScriptFunction::Ptr func = ScriptFunction::GetByName(funcName);

	if (!func)
		throw_exception(invalid_argument("Function '" + funcName + "' does not exist."));

	ScriptTask::Ptr task = boost::make_shared<ScriptTask>(func, arguments);
	task->Start(callback);

	return task;
}

void ConfigObject::DumpObjects(const string& filename)
{
	Logger::Write(LogInformation, "base", "Dumping program state to file '" + filename + "'");

	ofstream fp;
	fp.open(filename.c_str());

	if (!fp)
		throw_exception(runtime_error("Could not open '" + filename + "' file"));

	FIFO::Ptr fifo = boost::make_shared<FIFO>();

	ConfigObject::TypeMap::iterator tt;
	for (tt = GetAllObjects().begin(); tt != GetAllObjects().end(); tt++) {
		ConfigObject::NameMap::iterator nt;
		for (nt = tt->second.begin(); nt != tt->second.end(); nt++) {
			ConfigObject::Ptr object = nt->second;

			Dictionary::Ptr persistentObject = boost::make_shared<Dictionary>();

			persistentObject->Set("type", object->GetType());
			persistentObject->Set("name", object->GetName());

			/* only persist properties for replicated objects or for objects
			 * that are marked as persistent */
			if (!object->GetSource().empty() /*|| object->IsPersistent()*/)
				persistentObject->Set("properties", object->GetProperties());

			persistentObject->Set("tags", object->GetTags());

			Variant value = persistentObject;
			string json = value.Serialize();

			/* This is quite ugly, unfortunatelly Netstring requires an IOQueue object */
			Netstring::WriteStringToIOQueue(fifo.get(), json);

			size_t count;
			while ((count = fifo->GetAvailableBytes()) > 0) {
				char buffer[1024];
			
				if (count > sizeof(buffer))
					count = sizeof(buffer);

				fifo->Read(buffer, count);
				fp.write(buffer, count);
			}
		}
	}
}

void ConfigObject::RestoreObjects(const string& filename)
{
	assert(GetAllObjects().empty());

	Logger::Write(LogInformation, "base", "Restoring program state from file '" + filename + "'");

	std::ifstream fp;
	fp.open(filename.c_str());

	/* TODO: Fix this horrible mess. */
	FIFO::Ptr fifo = boost::make_shared<FIFO>();
	while (fp) {
		char buffer[1024];

		fp.read(buffer, sizeof(buffer));
		fifo->Write(buffer, fp.gcount());
	}

	string message;
	while (Netstring::ReadStringFromIOQueue(fifo.get(), &message)) {
		Variant value = Variant::Deserialize(message);

		if (!value.IsObjectType<Dictionary>())
			throw_exception(runtime_error("JSON objects in the program state file must be dictionaries."));

		Dictionary::Ptr persistentObject = value;

		string type;
		if (!persistentObject->Get("type", &type))
			continue;

		string name;
		if (!persistentObject->Get("name", &name))
			continue;

		Dictionary::Ptr tags;
		if (!persistentObject->Get("tags", &tags))
			continue;

		Dictionary::Ptr properties;
		if (persistentObject->Get("properties", &properties)) {
			ConfigObject::Ptr object = Create(type, properties);
			object->SetTags(tags);
			object->Commit();
		} else {
			/* keep non-replicated objects until another config object with
			 * the same name is created (which is when we restore its tags) */
			m_PersistentTags[make_pair(type, name)] = tags;
		}
	}
}

ConfigObject::TypeMap& ConfigObject::GetAllObjects(void)
{
	static TypeMap objects;
	return objects;
}

ConfigObject::ClassMap& ConfigObject::GetClasses(void)
{
	static ClassMap classes;
	return classes;
}

void ConfigObject::RegisterClass(const string& type, ConfigObject::Factory factory)
{
	GetClasses()[type] = factory;

	/* TODO: upgrade existing objects */
}

ConfigObject::Ptr ConfigObject::Create(const string& type, const Dictionary::Ptr& properties)
{
	ConfigObject::ClassMap::iterator it;
	it = GetClasses().find(type);

	if (it != GetClasses().end())
		return it->second(properties);
	else
		return boost::make_shared<ConfigObject>(properties);
}

