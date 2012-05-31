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

ObjectMap::ObjectMap(const ObjectSet::Ptr& parent, ObjectKeyGetter keygetter)
	: m_Parent(parent), m_KeyGetter(keygetter)
{
	assert(m_Parent);
	assert(m_KeyGetter);
}

void ObjectMap::Start(void)
{
	m_Parent->OnObjectAdded += bind_weak(&ObjectMap::ObjectAddedHandler, shared_from_this());
	m_Parent->OnObjectCommitted += bind_weak(&ObjectMap::ObjectCommittedHandler, shared_from_this());
	m_Parent->OnObjectRemoved += bind_weak(&ObjectMap::ObjectRemovedHandler, shared_from_this());

	for (ObjectSet::Iterator it = m_Parent->Begin(); it != m_Parent->End(); it++)
		AddObject(*it);
}

void ObjectMap::AddObject(const Object::Ptr& object)
{
	string key;
	if (!m_KeyGetter(object, &key))
		return;

	m_Objects.insert(pair<string, Object::Ptr>(key, object));
}

void ObjectMap::RemoveObject(const Object::Ptr& object)
{
	string key;
	if (!m_KeyGetter(object, &key))
		return;

	pair<Iterator, Iterator> range = GetRange(key);

	for (Iterator i = range.first; i != range.second; i++) {
		if (i->second == object) {
			m_Objects.erase(i);
			break;
		}
	}
}

void ObjectMap::CheckObject(const Object::Ptr& object)
{
	RemoveObject(object);
	AddObject(object);
}

ObjectMap::Range ObjectMap::GetRange(string key)
{
	return m_Objects.equal_range(key);
}

int ObjectMap::ObjectAddedHandler(const ObjectSetEventArgs& ea)
{
	AddObject(ea.Target);
	return 0;
}

int ObjectMap::ObjectCommittedHandler(const ObjectSetEventArgs& ea)
{
	CheckObject(ea.Target);
	return 0;
}

int ObjectMap::ObjectRemovedHandler(const ObjectSetEventArgs& ea)
{
	RemoveObject(ea.Target);
	return 0;
}
