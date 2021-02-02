/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "db_ido/dbtype.hpp"
#include "db_ido/dbconnection.hpp"
#include "base/objectlock.hpp"
#include "base/debug.hpp"
#include <boost/thread/once.hpp>

using namespace icinga;

DbType::DbType(String name, String table, long tid, String idcolumn, DbType::ObjectFactory factory)
	: m_Name(std::move(name)), m_Table(std::move(table)), m_TypeID(tid), m_IDColumn(std::move(idcolumn)), m_ObjectFactory(std::move(factory))
{ }

String DbType::GetName() const
{
	return m_Name;
}

String DbType::GetTable() const
{
	return m_Table;
}

long DbType::GetTypeID() const
{
	return m_TypeID;
}

String DbType::GetIDColumn() const
{
	return m_IDColumn;
}

void DbType::RegisterType(const DbType::Ptr& type)
{
	std::unique_lock<std::mutex> lock(GetStaticMutex());
	GetTypes()[type->GetName()] = type;
}

DbType::Ptr DbType::GetByName(const String& name)
{
	String typeName;

	if (name == "CheckCommand" || name == "NotificationCommand" || name == "EventCommand")
		typeName = "Command";
	else
		typeName = name;

	std::unique_lock<std::mutex> lock(GetStaticMutex());
	auto it = GetTypes().find(typeName);

	if (it == GetTypes().end())
		return nullptr;

	return it->second;
}

DbType::Ptr DbType::GetByID(long tid)
{
	std::unique_lock<std::mutex> lock(GetStaticMutex());

	for (const TypeMap::value_type& kv : GetTypes()) {
		if (kv.second->GetTypeID() == tid)
			return kv.second;
	}

	return nullptr;
}

DbObject::Ptr DbType::GetOrCreateObjectByName(const String& name1, const String& name2)
{
	ObjectLock olock(this);

	auto it = m_Objects.find(std::make_pair(name1, name2));

	if (it != m_Objects.end())
		return it->second;

	DbObject::Ptr dbobj = m_ObjectFactory(this, name1, name2);
	m_Objects[std::make_pair(name1, name2)] = dbobj;

	String objName = name1;

	if (!name2.IsEmpty())
		objName += "!" + name2;

	String objType = m_Name;

	if (m_TypeID == DbObjectTypeCommand) {
		if (objName.SubStr(0, 6) == "check_") {
			objType = "CheckCommand";
			objName = objName.SubStr(6);
		} else if (objName.SubStr(0, 13) == "notification_") {
			objType = "NotificationCommand";
			objName = objName.SubStr(13);
		} else if (objName.SubStr(0, 6) == "event_") {
			objType = "EventCommand";
			objName = objName.SubStr(6);
		}
	}

	dbobj->SetObject(ConfigObject::GetObject(objType, objName));

	return dbobj;
}

std::mutex& DbType::GetStaticMutex()
{
	static std::mutex mutex;
	return mutex;
}

/**
 * Caller must hold static mutex.
 */
DbType::TypeMap& DbType::GetTypes()
{
	static DbType::TypeMap tm;
	return tm;
}

std::set<DbType::Ptr> DbType::GetAllTypes()
{
	std::set<DbType::Ptr> result;

	{
		std::unique_lock<std::mutex> lock(GetStaticMutex());
		for (const auto& kv : GetTypes()) {
			result.insert(kv.second);
		}
	}

	return result;
}

DbTypeRegistry *DbTypeRegistry::GetInstance()
{
	return Singleton<DbTypeRegistry>::GetInstance();
}

