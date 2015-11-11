/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "base/configtype.hpp"
#include "base/serializer.hpp"
#include "base/debug.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"

using namespace icinga;

ConfigType::ConfigType(const String& name)
	: m_Name(name)
{ }

ConfigType::Ptr ConfigType::GetByName(const String& name)
{
	boost::mutex::scoped_lock lock(GetStaticMutex());

	ConfigType::TypeMap::const_iterator tt = InternalGetTypeMap().find(name);

	if (tt == InternalGetTypeMap().end()) {
		Type::Ptr type = Type::GetByName(name);

		if (!type || !ConfigObject::TypeInstance->IsAssignableFrom(type)
		    || type->IsAbstract())
			return ConfigType::Ptr();

		ConfigType::Ptr dtype = new ConfigType(name);

		InternalGetTypeMap()[type->GetName()] = dtype;
		InternalGetTypeVector().push_back(dtype);

		return dtype;
	}

	return tt->second;
}

/**
 * Note: Caller must hold ConfigType::GetStaticMutex() while using the map.
 */
ConfigType::TypeMap& ConfigType::InternalGetTypeMap(void)
{
	static ConfigType::TypeMap typemap;
	return typemap;
}

ConfigType::TypeVector& ConfigType::InternalGetTypeVector(void)
{
	static ConfigType::TypeVector typevector;
	return typevector;
}

ConfigType::TypeVector ConfigType::GetTypes(void)
{
	boost::mutex::scoped_lock lock(GetStaticMutex());
	return InternalGetTypeVector(); /* Making a copy of the vector here. */
}

std::pair<ConfigTypeIterator<ConfigObject>, ConfigTypeIterator<ConfigObject> > ConfigType::GetObjects(void)
{
	return std::make_pair(
	    ConfigTypeIterator<ConfigObject>(this, 0),
	    ConfigTypeIterator<ConfigObject>(this, -1)
	);
}

String ConfigType::GetName(void) const
{
	return m_Name;
}

void ConfigType::RegisterObject(const ConfigObject::Ptr& object)
{
	String name = object->GetName();

	{
		ObjectLock olock(this);

		ObjectMap::iterator it = m_ObjectMap.find(name);

		if (it != m_ObjectMap.end()) {
			if (it->second == object)
				return;

			BOOST_THROW_EXCEPTION(ScriptError("An object with type '" + m_Name + "' and name '" + name + "' already exists (" +
			    Convert::ToString(it->second->GetDebugInfo()) + "), new declaration: " + Convert::ToString(object->GetDebugInfo()),
			    object->GetDebugInfo()));
		}

		m_ObjectMap[name] = object;
		m_ObjectVector.push_back(object);
	}
}

void ConfigType::UnregisterObject(const ConfigObject::Ptr& object)
{
	String name = object->GetName();

	{
		ObjectLock olock(this);

		m_ObjectMap.erase(name);
		m_ObjectVector.erase(std::remove(m_ObjectVector.begin(), m_ObjectVector.end(), object), m_ObjectVector.end());
	}
}

ConfigObject::Ptr ConfigType::GetObject(const String& name) const
{
	ObjectLock olock(this);

	ConfigType::ObjectMap::const_iterator nt = m_ObjectMap.find(name);

	if (nt == m_ObjectMap.end())
		return ConfigObject::Ptr();

	return nt->second;
}

boost::mutex& ConfigType::GetStaticMutex(void)
{
	static boost::mutex mutex;
	return mutex;
}

