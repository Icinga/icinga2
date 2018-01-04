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

	static boost::mutex& GetStaticMutex();

	friend class DbType;
};

}

#endif /* DBOBJECT_H */
