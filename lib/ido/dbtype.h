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

#ifndef DBTYPE_H
#define DBTYPE_H

#include "base/object.h"
#include "base/registry.h"
#include "ido/dbobject.h"

namespace icinga
{

/**
 * A database object type.
 *
 * @ingroup ido
 */
class DbType : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(DbType);

	typedef boost::function<DbObject::Ptr (const String&, const String&)> ObjectFactory;
	typedef std::map<String, DbType::Ptr, string_iless> TypeMap;
	typedef std::map<std::pair<String, String>, DbObject::Ptr, pair_string_iless> ObjectMap;

	DbType(const String& name, const String& table, long tid, const ObjectFactory& factory);

	String GetName(void) const;
	String GetTable(void) const;
	long GetTypeID(void) const;

	static void RegisterType(const DbType::Ptr& type);

	static DbType::Ptr GetByName(const String& name);
	static DbType::Ptr GetByID(long tid);

	DbObject::Ptr GetOrCreateObjectByName(const String& name1, const String& name2);

private:
	String m_Name;
	String m_Table;
	long m_TypeID;
	ObjectFactory m_ObjectFactory;

	static boost::mutex m_StaticMutex;
	static TypeMap& GetTypes(void);

	ObjectMap& GetObjects(void);

	static void StaticInitialize(void);
};

/**
 * A registry for DbType objects.
 *
 * @ingroup ido
 */
class DbTypeRegistry : public Registry<DbType::Ptr>
{ };

/**
 * Helper class for registering DynamicObject implementation classes.
 *
 * @ingroup ido
 */
class RegisterDbTypeHelper
{
public:
	RegisterDbTypeHelper(const String& name, const String& table, long tid, const DbType::ObjectFactory& factory)
	{
		DbType::Ptr dbtype = boost::make_shared<DbType>(name, table, tid, factory);
		DbType::RegisterType(dbtype);
	}
};

/**
 * Factory function for DbObject-based classes.
 *
 * @ingroup ido
 */
template<typename T>
shared_ptr<T> DbObjectFactory(const String& name1, const String& name2)
{
	return boost::make_shared<T>(name1, name2);
}

#define REGISTER_DBTYPE(name, table, tid, type) \
	I2_EXPORT icinga::RegisterDbTypeHelper g_RegisterDBT_ ## type(name, table, tid, DbObjectFactory<type>);

}

#endif /* DBTYPE_H */
