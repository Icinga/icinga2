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

#include "ido/dbtype.h"
#include "base/objectlock.h"
#include <boost/thread/once.hpp>

using namespace icinga;

boost::mutex DbType::m_StaticMutex;

DbType::DbType(const String& name, const String& table, long tid, const DbType::ObjectFactory& factory)
	: m_Name(name), m_Table(table), m_TypeId(tid), m_ObjectFactory(factory)
{
	static boost::once_flag initializeOnce = BOOST_ONCE_INIT;
	boost::call_once(initializeOnce, &DbObject::StaticInitialize);
}

String DbType::GetName(void) const
{
	return m_Name;
}

String DbType::GetTable(void) const
{
	return m_Table;
}

long DbType::GetTypeId(void) const
{
	return m_TypeId;
}

void DbType::RegisterType(const DbType::Ptr& type)
{
	boost::mutex::scoped_lock lock(m_StaticMutex);
	GetTypes()[type->GetName()] = type;
}

DbType::Ptr DbType::GetByName(const String& name)
{
	boost::mutex::scoped_lock lock(m_StaticMutex);
	DbType::TypeMap::const_iterator it = GetTypes().find(name);

	if (it == GetTypes().end())
		return DbType::Ptr();

	return it->second;
}

DbObject::Ptr DbType::GetOrCreateObjectByName(const String& name1, const String& name2)
{
	ASSERT(!OwnsLock());

	ObjectLock olock(this);

	DbType::ObjectMap::const_iterator it = GetObjects().find(std::make_pair(name1, name2));

	if (it != GetObjects().end())
		return it->second;

	DbObject::Ptr dbobj = m_ObjectFactory(name1, name2);
	GetObjects()[std::make_pair(name1, name2)] = dbobj;

	return dbobj;
}

/**
 * Caller must hold m_StaticMutex.
 */
DbType::TypeMap& DbType::GetTypes(void)
{
	static DbType::TypeMap tm;
	return tm;
}

/**
 * Caller must hold object mutex.
 */
DbType::ObjectMap& DbType::GetObjects(void)
{
	static DbType::ObjectMap om;
	return om;
}
