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

#include "i2-dyn.h"

using namespace icinga;

void ObjectSpace::RegisterClass(string name, DynamicObjectFactory factory)
{
	m_Classes[name] = factory;
}

void ObjectSpace::UnregisterClass(string name)
{
	map<string, DynamicObjectFactory>::iterator ci = m_Classes.find(name);

	if (ci != m_Classes.end())
		m_Classes.erase(ci);
}

void ObjectSpace::RegisterObject(DynamicObject::Ptr object)
{
	m_Objects.insert(object);
}

void ObjectSpace::UnregisterObject(DynamicObject::Ptr object)
{
	set<DynamicObject::Ptr>::iterator di = m_Objects.find(object);

	if (di != m_Objects.end())
		m_Objects.erase(di);
}

Dictionary::Ptr ObjectSpace::SerializeObject(DynamicObject::Ptr object)
{
	DynamicDictionary::Ptr properties = object->GetProperties();
	if (!properties)
		throw InvalidArgumentException();

	Dictionary::Ptr data = make_shared<Dictionary>();
	data->SetProperty("type", object->GetType());

	Dictionary::Ptr serializedProperties = properties->Serialize();
	data->SetProperty("properties", serializedProperties);
	return data;
}

DynamicObject::Ptr ObjectSpace::UnserializeObject(Dictionary::Ptr data)
{
	string type;
	if (!data->GetProperty("type", &type))
		throw InvalidArgumentException();

	Dictionary::Ptr serializedProperties;
	if (!data->GetProperty("properties", &serializedProperties))
		throw InvalidArgumentException();
	DynamicDictionary::Ptr properties = make_shared<DynamicDictionary>(serializedProperties);

	map<string, DynamicObjectFactory>::iterator di = m_Classes.find(type);
	DynamicObject::Ptr object;
	if (di != m_Classes.end())
		object = di->second();
	else
		object = make_shared<DynamicObject>();

	object->SetProperties(properties);
	object->Commit();

	return object;
}

vector<DynamicObject::Ptr> FindObjects(function<bool (DynamicObject::Ptr)> predicate)
{
	return vector<DynamicObject::Ptr>();
}
