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

ObjectSet::ObjectSet(void)
	: m_Parent(), m_Filter()
{
}

ObjectSet::ObjectSet(const ObjectSet::Ptr& parent, ObjectPredicate filter)
	: m_Parent(parent), m_Filter(filter)
{
}

void ObjectSet::Start(void)
{
	if (m_Parent) {
		m_Parent->OnObjectAdded += bind_weak(&ObjectSet::ObjectAddedOrCommittedHandler, shared_from_this());
		m_Parent->OnObjectCommitted += bind_weak(&ObjectSet::ObjectAddedOrCommittedHandler, shared_from_this());
		m_Parent->OnObjectRemoved += bind_weak(&ObjectSet::ObjectRemovedHandler, shared_from_this());

		for (ObjectSet::Iterator it = m_Parent->Begin(); it != m_Parent->End(); it++)
			CheckObject(*it);
	}
}

void ObjectSet::AddObject(const Object::Ptr& object)
{
	m_Objects.insert(object);

	ObjectSetEventArgs ea;
	ea.Source = shared_from_this();
	ea.Target = object;
	OnObjectAdded(ea);
}

void ObjectSet::RemoveObject(const Object::Ptr& object)
{
	ObjectSet::Iterator it = m_Objects.find(object);

	if (it != m_Objects.end()) {
		m_Objects.erase(it);

		ObjectSetEventArgs ea;
		ea.Source = shared_from_this();
		ea.Target = object;
		OnObjectRemoved(ea);
	}
}

bool ObjectSet::Contains(const Object::Ptr& object) const
{
	ObjectSet::Iterator it = m_Objects.find(object);

	return !(it == m_Objects.end());
}

void ObjectSet::CheckObject(const Object::Ptr& object)
{
	if (m_Filter && !m_Filter(object))
		RemoveObject(object);
	else {
		if (!Contains(object))
			AddObject(object);
		else {
			ObjectSetEventArgs ea;
			ea.Source = shared_from_this();
			ea.Target = object;
			OnObjectCommitted(ea);
		}
	}
}

int ObjectSet::ObjectAddedOrCommittedHandler(const ObjectSetEventArgs& ea)
{
	CheckObject(ea.Target);
	return 0;
}

int ObjectSet::ObjectRemovedHandler(const ObjectSetEventArgs& ea)
{
	RemoveObject(ea.Target);
	return 0;
}

ObjectSet::Iterator ObjectSet::Begin(void)
{
	return m_Objects.begin();
}

ObjectSet::Iterator ObjectSet::End(void)
{
	return m_Objects.end();
}

ObjectSet::Ptr ObjectSet::GetAllObjects(void)
{
	static ObjectSet::Ptr allObjects;

	if (!allObjects) {
		allObjects = make_shared<ObjectSet>();
		allObjects->Start();
	}

	return allObjects;
}