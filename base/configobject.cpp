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

ConfigObject::ConfigObject(Dictionary::Ptr properties)
	: m_Properties(properties), m_Tags(make_shared<Dictionary>())
{ }

ConfigObject::ConfigObject(string type, string name)
	: m_Properties(make_shared<Dictionary>()), m_Tags(make_shared<Dictionary>())
{
	SetProperty("__type", type);
	SetProperty("__name", name);
}

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
	GetProperties()->GetProperty("__type", &type);
	return type;
}

string ConfigObject::GetName(void) const
{
	string name;
	GetProperties()->GetProperty("__name", &name);
	return name;
}

void ConfigObject::SetLocal(bool value)
{
	GetProperties()->SetProperty("__local", value ? 1 : 0);
}

bool ConfigObject::IsLocal(void) const
{
	bool value;
	GetProperties()->GetProperty("__local", &value);
	return (value != 0);
}

void ConfigObject::SetAbstract(bool value)
{
	GetProperties()->SetProperty("__abstract", value ? 1 : 0);
}

bool ConfigObject::IsAbstract(void) const
{
	long value;
	GetProperties()->GetProperty("__abstract", &value);
	return (value != 0);
}

void ConfigObject::Commit(void)
{
	ConfigObject::Ptr dobj = GetObject(GetType(), GetName());
	ConfigObject::Ptr self = static_pointer_cast<ConfigObject>(shared_from_this());
	assert(!dobj || dobj == self);
	GetAllObjects()->CheckObject(self);
}

void ConfigObject::Unregister(void)
{
	ConfigObject::Ptr self = static_pointer_cast<ConfigObject>(shared_from_this());
	GetAllObjects()->RemoveObject(self);
}

ObjectSet<ConfigObject::Ptr>::Ptr ConfigObject::GetAllObjects(void)
{
	static ObjectSet<ConfigObject::Ptr>::Ptr allObjects;

	if (!allObjects) {
		allObjects = make_shared<ObjectSet<ConfigObject::Ptr> >();
		allObjects->Start();
	}

	return allObjects;
}

ConfigObject::TNMap::Ptr ConfigObject::GetObjectsByTypeAndName(void)
{
	static ConfigObject::TNMap::Ptr tnmap;

	if (!tnmap) {
		tnmap = make_shared<ConfigObject::TNMap>(GetAllObjects(), &ConfigObject::TypeAndNameGetter);
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
	return bind(&ConfigObject::TypePredicate, _1, type);
}

bool ConfigObject::TypePredicate(const ConfigObject::Ptr& object, string type)
{
	return (object->GetType() == type);
}

ConfigObject::TMap::Ptr ConfigObject::GetObjectsByType(void)
{
	static ObjectMap<string, ConfigObject::Ptr>::Ptr tmap;

	if (!tmap) {
		tmap = make_shared<ConfigObject::TMap>(GetAllObjects(), &ConfigObject::TypeGetter);
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