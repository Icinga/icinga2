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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.            *
 ******************************************************************************/

#include "i2-base.h"

using namespace icinga;

/**
 * SetHive
 *
 * Sets the hive this collection belongs to.
 *
 * @param hive The hive.
 */
void ConfigCollection::SetHive(const ConfigHive::WeakPtr& hive)
{
	m_Hive = hive;
}

/**
 * GetHive
 *
 * Retrieves the hive this collection belongs to.
 *
 * @returns The hive.
 */
ConfigHive::WeakPtr ConfigCollection::GetHive(void) const
{
	return m_Hive;
}

/**
 * AddObject
 *
 * Adds a new object to this collection.
 *
 * @param object The new object.
 */
void ConfigCollection::AddObject(const ConfigObject::Ptr& object)
{
	RemoveObject(object);

	Objects[object->GetName()] = object;
	object->Commit();
}

/**
 * RemoveObject
 *
 * Removes an object from this collection
 *
 * @param object The object that is to be removed.
 */
void ConfigCollection::RemoveObject(const ConfigObject::Ptr& object)
{
	ObjectIterator oi = Objects.find(object->GetName());

	if (oi != Objects.end()) {
		Objects.erase(oi);

		EventArgs ea;
		ea.Source = object;
		OnObjectRemoved(ea);

		ConfigHive::Ptr hive = m_Hive.lock();
		if (hive)
			hive->OnObjectRemoved(ea);
	}
}

/**
 * GetObject
 *
 * Retrieves an object by name.
 *
 * @param name The name of the object.
 * @returns The object or a null pointer if the specified object
 *          could not be found.
 */
ConfigObject::Ptr ConfigCollection::GetObject(const string& name) const
{
	ObjectConstIterator oi = Objects.find(name);

	if (oi == Objects.end())
		return ConfigObject::Ptr();

	return oi->second;
}

/**
 * ForEachObject
 *
 * Invokes the specified callback for each object contained in this collection.
 *
 * @param callback The callback.
 */
void ConfigCollection::ForEachObject(function<int (const EventArgs&)> callback)
{
	EventArgs ea;

	for (ObjectIterator oi = Objects.begin(); oi != Objects.end(); oi++) {
		ea.Source = oi->second;
		callback(ea);
	}
}
