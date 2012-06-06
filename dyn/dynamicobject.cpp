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

DynamicObject::DynamicObject(void)
	: m_Config(make_shared<Dictionary>()), m_Tags(make_shared<Dictionary>())
{
}

void DynamicObject::SetConfig(Dictionary::Ptr config)
{
	m_Config = config;
}

Dictionary::Ptr DynamicObject::GetConfig(void) const
{
	return m_Config;
}

Dictionary::Ptr DynamicObject::GetTags(void) const
{
	return m_Tags;
}

string DynamicObject::GetType(void) const
{
	string type;
	GetConfig()->GetProperty("__type", &type);
	return type;
}

string DynamicObject::GetName(void) const
{
	string name;
	GetConfig()->GetProperty("__name", &name);
	return name;
}

void DynamicObject::Commit(void)
{
	DynamicObject::Ptr dobj = GetObject(GetType(), GetName());
	DynamicObject::Ptr self = static_pointer_cast<DynamicObject>(shared_from_this());
	assert(!dobj || dobj == self);
	GetAllObjects()->CheckObject(self);
}

void DynamicObject::Unregister(void)
{
	DynamicObject::Ptr self = static_pointer_cast<DynamicObject>(shared_from_this());
	GetAllObjects()->RemoveObject(self);
}

ObjectSet<DynamicObject::Ptr>::Ptr DynamicObject::GetAllObjects(void)
{
	static ObjectSet<DynamicObject::Ptr>::Ptr allObjects;

	if (!allObjects) {
		allObjects = make_shared<ObjectSet<DynamicObject::Ptr> >();
		allObjects->Start();
	}

	return allObjects;
}

DynamicObject::TNMap::Ptr DynamicObject::GetObjectsByTypeAndName(void)
{
	static DynamicObject::TNMap::Ptr tnmap;

	if (!tnmap) {
		tnmap = make_shared<DynamicObject::TNMap>(GetAllObjects(), &DynamicObject::GetTypeAndName);
		tnmap->Start();
	}

	return tnmap;
}

DynamicObject::Ptr DynamicObject::GetObject(string type, string name)
{
	DynamicObject::TNMap::Range range;
	range = GetObjectsByTypeAndName()->GetRange(make_pair(type, name));

	assert(distance(range.first, range.second) <= 1);

	if (range.first == range.second)
		return DynamicObject::Ptr();
	else
		return range.first->second;
}

bool DynamicObject::GetTypeAndName(const DynamicObject::Ptr& object, pair<string, string> *key)
{
        *key = make_pair(object->GetType(), object->GetName());

        return true;
}

