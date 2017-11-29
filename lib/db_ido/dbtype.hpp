/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "db_ido/i2-db_ido.hpp"
#include "base/object.hpp"
#include "base/registry.hpp"
#include "base/singleton.hpp"
#include <set>

namespace icinga
{

class DbObject;

/**
 * A database object type.
 *
 * @ingroup ido
 */
class I2_DB_IDO_API DbType : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(DbType);

	typedef std::function<intrusive_ptr<DbObject> (const intrusive_ptr<DbType>&, const String&, const String&)> ObjectFactory;
	typedef std::map<String, DbType::Ptr> TypeMap;
	typedef std::map<std::pair<String, String>, intrusive_ptr<DbObject> > ObjectMap;

	DbType(const String& name, const String& table, long tid, const String& idcolumn, const ObjectFactory& factory);

	String GetName(void) const;
	String GetTable(void) const;
	long GetTypeID(void) const;
	String GetIDColumn(void) const;

	static void RegisterType(const DbType::Ptr& type);

	static DbType::Ptr GetByName(const String& name);
	static DbType::Ptr GetByID(long tid);

	intrusive_ptr<DbObject> GetOrCreateObjectByName(const String& name1, const String& name2);

	static std::set<DbType::Ptr> GetAllTypes(void);

private:
	String m_Name;
	String m_Table;
	long m_TypeID;
	String m_IDColumn;
	ObjectFactory m_ObjectFactory;

	static boost::mutex& GetStaticMutex(void);
	static TypeMap& GetTypes(void);

	ObjectMap m_Objects;
};

/**
 * A registry for DbType objects.
 *
 * @ingroup ido
 */
class I2_DB_IDO_API DbTypeRegistry : public Registry<DbTypeRegistry, DbType::Ptr>
{
public:
	static DbTypeRegistry *GetInstance(void);
};

/**
 * Factory function for DbObject-based classes.
 *
 * @ingroup ido
 */
template<typename T>
intrusive_ptr<T> DbObjectFactory(const DbType::Ptr& type, const String& name1, const String& name2)
{
	return new T(type, name1, name2);
}

#define REGISTER_DBTYPE(name, table, tid, idcolumn, type) \
	INITIALIZE_ONCE([]() { \
		DbType::Ptr dbtype = new DbType(#name, table, tid, idcolumn, DbObjectFactory<type>); \
		DbType::RegisterType(dbtype); \
	})

}

#endif /* DBTYPE_H */
