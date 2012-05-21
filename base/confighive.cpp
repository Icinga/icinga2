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

/**
 * Adds a new object to this hive.
 *
 * @param object The new object.
 */
void ConfigHive::AddObject(const ConfigObject::Ptr& object)
{
	object->SetHive(static_pointer_cast<ConfigHive>(shared_from_this()));
	GetCollection(object->GetType())->AddObject(object);
}

/**
 * Removes an object from this hive.
 *
 * @param object The object that is to be removed.
 */
void ConfigHive::RemoveObject(const ConfigObject::Ptr& object)
{
	GetCollection(object->GetType())->RemoveObject(object);
}

/**
 * Retrieves an object by type and name.
 *
 * @param type The type of the object.
 * @param name The name of the object.
 * @returns The object or a null pointer if the specified object
 *          could not be found.
 */
ConfigObject::Ptr ConfigHive::GetObject(const string& type, const string& name)
{
	return GetCollection(type)->GetObject(name);
}

/**
 * Retrieves a collection by name. Creates an empty collection
 * if the collection doesn't already exist.
 *
 * @param collection The name of the collection.
 * @returns The collection or a null pointer if the specified collection
 *          could not be found.
 */
ConfigCollection::Ptr ConfigHive::GetCollection(const string& collection)
{
	CollectionConstIterator ci = Collections.find(collection);

	if (ci == Collections.end()) {
		Collections[collection] = make_shared<ConfigCollection>();
		ci = Collections.find(collection);
	}

	return ci->second;
}

/**
 * Invokes the specified callback for each object contained in this hive.
 *
 * @param type The config object type.
 * @param callback The callback.
 */
void ConfigHive::ForEachObject(const string& type,
    function<int (const EventArgs&)> callback)
{
	CollectionIterator ci = Collections.find(type);

	if (ci == Collections.end())
		return;

	ci->second->ForEachObject(callback);
}
