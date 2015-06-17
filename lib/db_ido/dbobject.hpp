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

#ifndef DBOBJECT_H
#define DBOBJECT_H

#include "db_ido/i2-db_ido.hpp"
#include "db_ido/dbreference.hpp"
#include "db_ido/dbquery.hpp"
#include "db_ido/dbtype.hpp"
#include "icinga/customvarobject.hpp"
#include "base/dynamicobject.hpp"

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
class I2_DB_IDO_API DbObject : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(DbObject);

	static void StaticInitialize(void);

	void SetObject(const DynamicObject::Ptr& object);
	DynamicObject::Ptr GetObject(void) const;

	String GetName1(void) const;
	String GetName2(void) const;
	intrusive_ptr<DbType> GetType(void) const;

	virtual Dictionary::Ptr GetConfigFields(void) const = 0;
	virtual Dictionary::Ptr GetStatusFields(void) const = 0;

	static DbObject::Ptr GetOrCreateByObject(const DynamicObject::Ptr& object);

	static boost::signals2::signal<void (const DbQuery&)> OnQuery;

	void SendConfigUpdate(void);
	void SendStatusUpdate(void);
	void SendVarsConfigUpdate(void);
	void SendVarsStatusUpdate(void);

	double GetLastConfigUpdate(void) const;
	double GetLastStatusUpdate(void) const;

protected:
	DbObject(const intrusive_ptr<DbType>& type, const String& name1, const String& name2);

	virtual bool IsStatusAttribute(const String& attribute) const;

	virtual void OnConfigUpdate(void);
	virtual void OnStatusUpdate(void);

private:
	String m_Name1;
	String m_Name2;
	intrusive_ptr<DbType> m_Type;
	DynamicObject::Ptr m_Object;
	double m_LastConfigUpdate;
	double m_LastStatusUpdate;

	static void StateChangedHandler(const DynamicObject::Ptr& object);
	static void VarsChangedHandler(const CustomVarObject::Ptr& object);
	static boost::mutex& GetStaticMutex(void);

	friend class DbType;
};

}

#endif /* DBOBJECT_H */
