/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef DBOBJECT_H
#define DBOBJECT_H

#include "db_ido/i2-db_ido.hpp"
#include "db_ido/dbreference.hpp"
#include "db_ido/dbquery.hpp"
#include "db_ido/dbtype.hpp"
#include "icinga/customvarobject.hpp"
#include "base/configobject.hpp"

namespace icinga
{

enum DbObjectUpdateType
{
	DbObjectCreated,
	DbObjectRemoved
};

enum DbObjectType
{
	DbObjectTypeHost = 1,
	DbObjectTypeService = 2,
	DbObjectTypeHostGroup = 3,
	DbObjectTypeServiceGroup = 4,
	DbObjectTypeHostEscalation = 5,
	DbObjectTypeServiceEscalation = 6,
	DbObjectTypeHostDependency = 7,
	DbObjectTypeServiceDependency = 8,
	DbObjectTypeTimePeriod = 9,
	DbObjectTypeContact = 10,
	DbObjectTypeContactGroup = 11,
	DbObjectTypeCommand = 12,
	DbObjectTypeEndpoint = 13,
	DbObjectTypeZone = 14,
};

/**
 * A database object.
 *
 * @ingroup ido
 */
class DbObject : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(DbObject);

	static void StaticInitialize();

	void SetObject(const ConfigObject::Ptr& object);
	ConfigObject::Ptr GetObject() const;

	String GetName1() const;
	String GetName2() const;
	intrusive_ptr<DbType> GetType() const;

	virtual Dictionary::Ptr GetConfigFields() const = 0;
	virtual Dictionary::Ptr GetStatusFields() const = 0;

	static DbObject::Ptr GetOrCreateByObject(const ConfigObject::Ptr& object);

	static boost::signals2::signal<void (const DbQuery&)> OnQuery;
	static boost::signals2::signal<void (const std::vector<DbQuery>&)> OnMultipleQueries;

	void SendConfigUpdateHeavy(const Dictionary::Ptr& configFields);
	void SendConfigUpdateLight();
	void SendStatusUpdate();
	void SendVarsConfigUpdateHeavy();
	void SendVarsStatusUpdate();

	double GetLastConfigUpdate() const;
	double GetLastStatusUpdate() const;

	virtual String CalculateConfigHash(const Dictionary::Ptr& configFields) const;

protected:
	DbObject(intrusive_ptr<DbType> type, String name1, String name2);

	virtual void OnConfigUpdateHeavy();
	virtual void OnConfigUpdateLight();
	virtual void OnStatusUpdate();

	static String HashValue(const Value& value);

private:
	String m_Name1;
	String m_Name2;
	intrusive_ptr<DbType> m_Type;
	ConfigObject::Ptr m_Object;
	double m_LastConfigUpdate;
	double m_LastStatusUpdate;

	static void StateChangedHandler(const ConfigObject::Ptr& object);
	static void VarsChangedHandler(const CustomVarObject::Ptr& object);
	static void VersionChangedHandler(const ConfigObject::Ptr& object);

	static std::mutex& GetStaticMutex();

	friend class DbType;
};

}

#endif /* DBOBJECT_H */
