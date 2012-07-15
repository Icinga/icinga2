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

ConfigObject::ConfigObject(Dictionary::Ptr properties, const ConfigObject::Set::Ptr& container)
	: m_Container(container ? container : GetAllObjects()),
	m_Properties(properties), m_Tags(boost::make_shared<Dictionary>())
{ }

void ConfigObject::SetProperties(Dictionary::Ptr properties)
{
	m_Properties = properties;
}

Dictionary::Ptr ConfigObject::GetProperties(void) const
{
	return m_Properties;
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

void ConfigObject::SetCommitTimestamp(time_t ts)
{
	GetProperties()->Set("__tx", static_cast<long>(ts));
}

time_t ConfigObject::GetCommitTimestamp(void) const
{
	long value = false;
	GetProperties()->Get("__tx", &value);
	return value;
}

void ConfigObject::Commit(void)
{
	assert(Application::IsMainThread());

	ConfigObject::Ptr dobj = GetObject(GetType(), GetName());
	ConfigObject::Ptr self = GetSelf();
	assert(!dobj || dobj == self);
	m_Container->CheckObject(self);

	time_t now;
	time(&now);
	SetCommitTimestamp(now);
}

void ConfigObject::Unregister(void)
{
	assert(Application::IsMainThread());

	ConfigObject::Ptr self = GetSelf();
	m_Container->RemoveObject(self);
}

ObjectSet<ConfigObject::Ptr>::Ptr ConfigObject::GetAllObjects(void)
{
	static ObjectSet<ConfigObject::Ptr>::Ptr allObjects;

	if (!allObjects) {
		allObjects = boost::make_shared<ObjectSet<ConfigObject::Ptr> >();
		allObjects->Start();
	}

	return allObjects;
}

ConfigObject::TNMap::Ptr ConfigObject::GetObjectsByTypeAndName(void)
{
	static ConfigObject::TNMap::Ptr tnmap;

	if (!tnmap) {
		tnmap = boost::make_shared<ConfigObject::TNMap>(GetAllObjects(), &ConfigObject::TypeAndNameGetter);
		tnmap->Start();
	}

	return tnmap;
}

ConfigObject::Ptr ConfigObject::GetObject(string type, string name)
{
	ConfigObject::TNMap::Range range;
	range = GetObjectsByTypeAndName()->GetRange(make_pair(type, name));

	assert(distance(range.first, range.second) <= 1);

	if (range.first == range.second)
		return ConfigObject::Ptr();
	else
		return range.first->second;
}

bool ConfigObject::TypeAndNameGetter(const ConfigObject::Ptr& object, pair<string, string> *key)
{
	*key = make_pair(object->GetType(), object->GetName());

	return true;
}

function<bool (ConfigObject::Ptr)> ConfigObject::MakeTypePredicate(string type)
{
	return boost::bind(&ConfigObject::TypePredicate, _1, type);
}

bool ConfigObject::TypePredicate(const ConfigObject::Ptr& object, string type)
{
	return (object->GetType() == type);
}

ConfigObject::TMap::Ptr ConfigObject::GetObjectsByType(void)
{
	static ObjectMap<string, ConfigObject::Ptr>::Ptr tmap;

	if (!tmap) {
		tmap = boost::make_shared<ConfigObject::TMap>(GetAllObjects(), &ConfigObject::TypeGetter);
		tmap->Start();
	}

	return tmap;
}

bool ConfigObject::TypeGetter(const ConfigObject::Ptr& object, string *key)
{
	*key = object->GetType();
	return true;
}

ConfigObject::TMap::Range ConfigObject::GetObjects(string type)
{
	return GetObjectsByType()->GetRange(type);
}

void ConfigObject::RemoveTag(const string& key)
{
	GetTags()->Remove(key);
}

ScriptTask::Ptr ConfigObject::InvokeHook(const string& hook,
    const vector<Variant>& arguments, ScriptTask::CompletionCallback callback)
{
	Dictionary::Ptr hooks;
	string funcName;
	if (!GetProperty("hooks", &hooks) || !hooks->Get(hook, &funcName))
		return ScriptTask::Ptr();

	ScriptFunction::Ptr func = ScriptFunction::GetByName(funcName);

	if (!func)
		throw invalid_argument("Function '" + funcName + "' does not exist.");

	ScriptTask::Ptr task = boost::make_shared<ScriptTask>(func, arguments);
	task->Start(callback);

	return task;
}
