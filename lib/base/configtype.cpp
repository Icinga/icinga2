/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/configobject.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"

using namespace icinga;

ConfigType::~ConfigType()
{ }

ConfigObject::Ptr ConfigType::GetObject(const String& name) const
{
	boost::mutex::scoped_lock lock(m_Mutex);

	auto nt = m_ObjectMap.find(name);

	if (nt == m_ObjectMap.end())
		return nullptr;

	return nt->second;
}

void ConfigType::RegisterObject(const ConfigObject::Ptr& object)
{
	String name = object->GetName();

	{
		boost::mutex::scoped_lock lock(m_Mutex);

		auto it = m_ObjectMap.find(name);

		if (it != m_ObjectMap.end()) {
			if (it->second == object)
				return;

			Type *type = dynamic_cast<Type *>(this);

			BOOST_THROW_EXCEPTION(ScriptError("An object with type '" + type->GetName() + "' and name '" + name + "' already exists (" +
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
		boost::mutex::scoped_lock lock(m_Mutex);

		m_ObjectMap.erase(name);
		m_ObjectVector.erase(std::remove(m_ObjectVector.begin(), m_ObjectVector.end(), object), m_ObjectVector.end());
	}
}

std::vector<ConfigObject::Ptr> ConfigType::GetObjects() const
{
	boost::mutex::scoped_lock lock(m_Mutex);
	return m_ObjectVector;
}

std::vector<ConfigObject::Ptr> ConfigType::GetObjectsHelper(Type *type)
{
	return static_cast<TypeImpl<ConfigObject> *>(type)->GetObjects();
}

int ConfigType::GetObjectCount() const
{
	boost::mutex::scoped_lock lock(m_Mutex);
	return m_ObjectVector.size();
}
