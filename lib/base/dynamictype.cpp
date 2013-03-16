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

#include "base/dynamictype.h"
#include "base/objectlock.h"

using namespace icinga;

DynamicType::DynamicType(const String& name, const DynamicType::ObjectFactory& factory)
	: m_Name(name), m_ObjectFactory(factory)
{ }

/**
 * @threadsafety Always.
 */
DynamicType::Ptr DynamicType::GetByName(const String& name)
{
	boost::mutex::scoped_lock lock(GetStaticMutex());

	DynamicType::TypeMap::const_iterator tt = InternalGetTypeMap().find(name);

	if (tt == InternalGetTypeMap().end())
		return DynamicType::Ptr();

	return tt->second;
}

/**
 * @threadsafety Caller must hold DynamicType::GetStaticMutex() while using the map.
 */
DynamicType::TypeMap& DynamicType::InternalGetTypeMap(void)
{
	static DynamicType::TypeMap typemap;
	return typemap;
}

DynamicType::TypeSet& DynamicType::InternalGetTypeSet(void)
{
	static DynamicType::TypeSet typeset;
	return typeset;
}

DynamicType::TypeSet DynamicType::GetTypes(void)
{
	boost::mutex::scoped_lock lock(GetStaticMutex());
	return InternalGetTypeSet(); /* Making a copy of the set here. */
}

std::set<DynamicObject::Ptr> DynamicType::GetObjects(const String& type)
{
	DynamicType::Ptr dt = GetByName(type);
	return dt->GetObjects();
}

std::set<DynamicObject::Ptr> DynamicType::GetObjects(void) const
{
	ObjectLock olock(this);

	return m_ObjectSet; /* Making a copy of the set here. */
}

String DynamicType::GetName(void) const
{
	return m_Name;
}

void DynamicType::RegisterObject(const DynamicObject::Ptr& object)
{
	String name = object->GetName();

	{
		ObjectLock olock(this);

		ObjectMap::iterator it = m_ObjectMap.find(name);

		if (it != m_ObjectMap.end()) {
			if (it->second == object)
				return;

			BOOST_THROW_EXCEPTION(std::runtime_error("RegisterObject() found existing object with the same name: " + name));
		}

		m_ObjectMap[name] = object;
		m_ObjectSet.insert(object);

		object->m_Registered = true;
	}

	object->OnRegistrationCompleted();
}

void DynamicType::UnregisterObject(const DynamicObject::Ptr& object)
{
	{
		ObjectLock olock(this);

		m_ObjectMap.erase(object->GetName());
		m_ObjectSet.erase(object);

		object->m_Registered = false;
	}

	object->OnUnregistrationCompleted();
}

/**
 * @threadsafety Always.
 */
DynamicObject::Ptr DynamicType::GetObject(const String& name) const
{
	ObjectLock olock(this);

	DynamicType::ObjectMap::const_iterator nt = m_ObjectMap.find(name);

	if (nt == m_ObjectMap.end())
		return DynamicObject::Ptr();

	return nt->second;
}

/**
 * @threadsafety Always.
 */
void DynamicType::RegisterType(const DynamicType::Ptr& type)
{
	boost::mutex::scoped_lock lock(GetStaticMutex());

	DynamicType::TypeMap::const_iterator tt = InternalGetTypeMap().find(type->GetName());

	if (tt != InternalGetTypeMap().end())
		BOOST_THROW_EXCEPTION(std::runtime_error("Cannot register class for type '" +
		    type->GetName() + "': Objects of this type already exist."));

	InternalGetTypeMap()[type->GetName()] = type;
	InternalGetTypeSet().insert(type);
}

DynamicObject::Ptr DynamicType::CreateObject(const Dictionary::Ptr& serializedUpdate)
{
	ASSERT(!OwnsLock());

	ObjectFactory factory;

	{
		ObjectLock olock(this);
		factory = m_ObjectFactory;
	}

	DynamicObject::Ptr object = factory(serializedUpdate);

	/* apply the object's non-config attributes */
	object->ApplyUpdate(serializedUpdate, Attribute_All & ~Attribute_Config);

	return object;
}

boost::mutex& DynamicType::GetStaticMutex(void)
{
	static boost::mutex mutex;
	return mutex;
}
