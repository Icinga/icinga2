/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
class DbType final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(DbType);

	typedef std::function<intrusive_ptr<DbObject> (const intrusive_ptr<DbType>&, const String&, const String&)> ObjectFactory;
	typedef std::map<String, DbType::Ptr> TypeMap;
	typedef std::map<std::pair<String, String>, intrusive_ptr<DbObject> > ObjectMap;

	DbType(String name, String table, long tid, String idcolumn, ObjectFactory factory);

	String GetName() const;
	String GetTable() const;
	long GetTypeID() const;
	String GetIDColumn() const;

	static void RegisterType(const DbType::Ptr& type);

	static DbType::Ptr GetByName(const String& name);
	static DbType::Ptr GetByID(long tid);

	intrusive_ptr<DbObject> GetOrCreateObjectByName(const String& name1, const String& name2);

	static std::set<DbType::Ptr> GetAllTypes();

private:
	String m_Name;
	String m_Table;
	long m_TypeID;
	String m_IDColumn;
	ObjectFactory m_ObjectFactory;

	static boost::mutex& GetStaticMutex();
	static TypeMap& GetTypes();

	ObjectMap m_Objects;
};

/**
 * A registry for DbType objects.
 *
 * @ingroup ido
 */
class DbTypeRegistry : public Registry<DbTypeRegistry, DbType::Ptr>
{
public:
	static DbTypeRegistry *GetInstance();
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
