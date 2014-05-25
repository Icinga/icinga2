/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "base/dynamictype.hpp"
#include "base/serializer.hpp"
#include "base/debug.hpp"
#include "base/objectlock.hpp"

using namespace icinga;

DynamicType::DynamicType(const String& name)
	: m_Name(name)
{ }

DynamicType::Ptr DynamicType::GetByName(const String& name)
{
	boost::mutex::scoped_lock lock(GetStaticMutex());

	DynamicType::TypeMap::const_iterator tt = InternalGetTypeMap().find(name);

	if (tt == InternalGetTypeMap().end()) {
		const Type *type = Type::GetByName(name);

		if (!type || !Type::GetByName("DynamicObject")->IsAssignableFrom(type)
		    || type->IsAbstract())
			return DynamicType::Ptr();

		DynamicType::Ptr dtype = make_shared<DynamicType>(name);

		InternalGetTypeMap()[type->GetName()] = dtype;
		InternalGetTypeVector().push_back(dtype);

		return dtype;
	}

	return tt->second;
}

/**
 * Note: Caller must hold DynamicType::GetStaticMutex() while using the map.
 */
DynamicType::TypeMap& DynamicType::InternalGetTypeMap(void)
{
	static DynamicType::TypeMap typemap;
	return typemap;
}

DynamicType::TypeVector& DynamicType::InternalGetTypeVector(void)
{
	static DynamicType::TypeVector typevector;
	return typevector;
}

DynamicType::TypeVector DynamicType::GetTypes(void)
{
	boost::mutex::scoped_lock lock(GetStaticMutex());
	return InternalGetTypeVector(); /* Making a copy of the vector here. */
}

std::pair<DynamicTypeIterator<DynamicObject>, DynamicTypeIterator<DynamicObject> > DynamicType::GetObjects(void)
{
	return std::make_pair(
	    DynamicTypeIterator<DynamicObject>(GetSelf(), 0),
	    DynamicTypeIterator<DynamicObject>(GetSelf(), -1)
	);
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
		m_ObjectVector.push_back(object);
	}
}

DynamicObject::Ptr DynamicType::GetObject(const String& name) const
{
	ObjectLock olock(this);

	DynamicType::ObjectMap::const_iterator nt = m_ObjectMap.find(name);

	if (nt == m_ObjectMap.end())
		return DynamicObject::Ptr();

	return nt->second;
}

DynamicObject::Ptr DynamicType::CreateObject(const Dictionary::Ptr& serializedUpdate)
{
	ASSERT(!OwnsLock());

	const Type *type = Type::GetByName(m_Name);

	Object::Ptr object = type->Instantiate();

	Deserialize(object, serializedUpdate, false, FAConfig);

	return static_pointer_cast<DynamicObject>(object);
}

boost::mutex& DynamicType::GetStaticMutex(void)
{
	static boost::mutex mutex;
	return mutex;
}

