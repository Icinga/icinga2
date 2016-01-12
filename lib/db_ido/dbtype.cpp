/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "db_ido/dbtype.hpp"
#include "db_ido/dbconnection.hpp"
#include "base/objectlock.hpp"
#include "base/debug.hpp"
#include <boost/thread/once.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

DbType::DbType(const String& table, long tid, const String& idcolumn, const DbType::ObjectFactory& factory)
	: m_Table(table), m_TypeID(tid), m_IDColumn(idcolumn), m_ObjectFactory(factory)
{ }

std::vector<String> DbType::GetNames(void) const
{
	boost::mutex::scoped_lock lock(GetStaticMutex());
	return m_Names;
}

String DbType::GetTable(void) const
{
	return m_Table;
}

long DbType::GetTypeID(void) const
{
	return m_TypeID;
}

String DbType::GetIDColumn(void) const
{
	return m_IDColumn;
}

void DbType::RegisterType(const String& name, const DbType::Ptr& type)
{
	boost::mutex::scoped_lock lock(GetStaticMutex());
	type->m_Names.push_back(name);
	GetTypes()[name] = type;
}

DbType::Ptr DbType::GetByName(const String& name)
{
	boost::mutex::scoped_lock lock(GetStaticMutex());
	DbType::TypeMap::const_iterator it = GetTypes().find(name);

	if (it == GetTypes().end())
		return DbType::Ptr();

	return it->second;
}

DbType::Ptr DbType::GetByID(long tid)
{
	boost::mutex::scoped_lock lock(GetStaticMutex());

	BOOST_FOREACH(const TypeMap::value_type& kv, GetTypes()) {
		if (kv.second->GetTypeID() == tid)
			return kv.second;
	}

	return DbType::Ptr();
}

DbObject::Ptr DbType::GetOrCreateObjectByName(const String& name1, const String& name2)
{
	ObjectLock olock(this);

	DbType::ObjectMap::const_iterator it = m_Objects.find(std::make_pair(name1, name2));

	if (it != m_Objects.end())
		return it->second;

	DbObject::Ptr dbobj = m_ObjectFactory(this, name1, name2);
	m_Objects[std::make_pair(name1, name2)] = dbobj;

	return dbobj;
}

boost::mutex& DbType::GetStaticMutex(void)
{
	static boost::mutex mutex;
	return mutex;
}

/**
 * Caller must hold static mutex.
 */
DbType::TypeMap& DbType::GetTypes(void)
{
	static DbType::TypeMap tm;
	return tm;
}

std::set<DbType::Ptr> DbType::GetAllTypes(void)
{
	std::set<DbType::Ptr> result;

	boost::mutex::scoped_lock lock(GetStaticMutex());
	std::pair<String, DbType::Ptr> kv;
	BOOST_FOREACH(kv, GetTypes()) {
		result.insert(kv.second);
	}

	return result;
}

DbTypeRegistry *DbTypeRegistry::GetInstance(void)
{
	return Singleton<DbTypeRegistry>::GetInstance();
}

