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

DynamicType::DynamicType(const String& name, const DynamicType::ObjectFactory& factory)
	: m_Name(name), m_ObjectFactory(factory)
{ }

/**
 * @threadsafety Always.
 */
DynamicType::Ptr DynamicType::GetByName(const String& name)
{
	boost::mutex::scoped_lock lock(GetStaticMutex());

	DynamicType::TypeMap::const_iterator tt = GetTypes().find(name);

	if (tt == GetTypes().end())
		return DynamicType::Ptr();

	return tt->second;
}

/**
 * @threadsafety Caller must hold DynamicType::GetStaticMutex() while using the map.
 */
DynamicType::TypeMap& DynamicType::GetTypes(void)
{
	static DynamicType::TypeMap types;
	return types;
}

/**
 * @threadsafety Caller must hold DynamicType::GetStaticMutex() while using the map.
 */
DynamicType::NameMap& DynamicType::GetObjects(void)
{
	return m_Objects;
}

String DynamicType::GetName(void) const
{
	return m_Name;
}

void DynamicType::RegisterObject(const DynamicObject::Ptr& object)
{
	m_Objects[object->GetName()] = object;
}

void DynamicType::UnregisterObject(const DynamicObject::Ptr& object)
{
	m_Objects.erase(object->GetName());
}

DynamicObject::Ptr DynamicType::GetObject(const String& name) const
{
	DynamicType::NameMap::const_iterator nt = m_Objects.find(name);

	if (nt == m_Objects.end())
		return DynamicObject::Ptr();

	return nt->second;
}

/**
 * @threadsafety Always.
 */
void DynamicType::RegisterType(const DynamicType::Ptr& type)
{
	boost::mutex::scoped_lock lock(GetStaticMutex());

	DynamicType::TypeMap::const_iterator tt = GetTypes().find(type->GetName());

	if (tt != GetTypes().end())
		BOOST_THROW_EXCEPTION(runtime_error("Cannot register class for type '" +
		    type->GetName() + "': Objects of this type already exist."));

	GetTypes()[type->GetName()] = type;
}

DynamicObject::Ptr DynamicType::CreateObject(const Dictionary::Ptr& serializedUpdate) const
{
	DynamicObject::Ptr obj = m_ObjectFactory(serializedUpdate);

	/* register attributes */
	String name;
	DynamicAttributeType type;
	BOOST_FOREACH(tuples::tie(name, type), m_Attributes)
		obj->RegisterAttribute(name, type);

	/* apply the object's non-config attributes */
	obj->ApplyUpdate(serializedUpdate, Attribute_All & ~Attribute_Config);

	/* notify the object that it's been fully initialized */
	obj->OnInitCompleted();

	return obj;
}

/**
 * @threadsafety Always.
 */
bool DynamicType::TypeExists(const String& name)
{
	return (GetByName(name));
}

void DynamicType::AddAttribute(const String& name, DynamicAttributeType type)
{
	m_Attributes[name] = type;
}

void DynamicType::RemoveAttribute(const String& name)
{
	m_Attributes.erase(name);
}

bool DynamicType::HasAttribute(const String& name)
{
	return (m_Attributes.find(name) != m_Attributes.end());
}

void DynamicType::AddAttributes(const AttributeDescription *attributes, int attributeCount)
{
	for (int i = 0; i < attributeCount; i++)
		AddAttribute(attributes[i].Name, attributes[i].Type);
}

boost::mutex& DynamicType::GetStaticMutex(void)
{
	static boost::mutex mutex;
	return mutex;
}
