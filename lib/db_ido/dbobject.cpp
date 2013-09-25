/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "db_ido/dbobject.h"
#include "db_ido/dbtype.h"
#include "db_ido/dbvalue.h"
#include "icinga/service.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/utility.h"
#include "base/initialize.h"
#include "base/logger_fwd.h"
#include <boost/foreach.hpp>

using namespace icinga;

boost::signals2::signal<void (const DbQuery&)> DbObject::OnQuery;

INITIALIZE_ONCE(DbObject, &DbObject::StaticInitialize);

DbObject::DbObject(const shared_ptr<DbType>& type, const String& name1, const String& name2)
	: m_Name1(name1), m_Name2(name2), m_Type(type), m_LastConfigUpdate(0), m_LastStatusUpdate(0)
{ }

void DbObject::StaticInitialize(void)
{
	DynamicObject::OnStateChanged.connect(boost::bind(&DbObject::StateChangedHandler, _1));
}

void DbObject::SetObject(const DynamicObject::Ptr& object)
{
	m_Object = object;
}

DynamicObject::Ptr DbObject::GetObject(void) const
{
	return m_Object;
}

String DbObject::GetName1(void) const
{
	return m_Name1;
}

String DbObject::GetName2(void) const
{
	return m_Name2;
}

DbType::Ptr DbObject::GetType(void) const
{
	return m_Type;
}

void DbObject::SendConfigUpdate(void)
{
	Dictionary::Ptr fields = GetConfigFields();

	if (!fields)
		return;

	DbQuery query;
	query.Table = GetType()->GetTable() + "s";
	query.Type = DbQueryInsert | DbQueryUpdate;
	query.Fields = fields;
	query.Fields->Set(GetType()->GetIDColumn(), GetObject());
	query.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */
	query.Fields->Set("config_type", 1);
	query.WhereCriteria = boost::make_shared<Dictionary>();
	query.WhereCriteria->Set(GetType()->GetIDColumn(), GetObject());
	query.Object = GetSelf();
	query.ConfigUpdate = true;
	OnQuery(query);

	m_LastConfigUpdate = Utility::GetTime();

	OnConfigUpdate();
}

void DbObject::SendStatusUpdate(void)
{
	Dictionary::Ptr fields = GetStatusFields();

	if (!fields)
		return;

	DbQuery query;
	query.Table = GetType()->GetTable() + "status";
	query.Type = DbQueryInsert | DbQueryUpdate;
	query.Fields = fields;
	query.Fields->Set(GetType()->GetIDColumn(), GetObject());
	query.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */
	query.Fields->Set("status_update_time", DbValue::FromTimestamp(Utility::GetTime()));
	query.WhereCriteria = boost::make_shared<Dictionary>();
	query.WhereCriteria->Set(GetType()->GetIDColumn(), GetObject());
	query.Object = GetSelf();
	query.StatusUpdate = true;
	OnQuery(query);

	m_LastStatusUpdate = Utility::GetTime();

	OnStatusUpdate();
}

double DbObject::GetLastConfigUpdate(void) const
{
	return m_LastConfigUpdate;
}

double DbObject::GetLastStatusUpdate(void) const
{
	return m_LastStatusUpdate;
}

bool DbObject::IsStatusAttribute(const String&) const
{
	return false;
}

void DbObject::OnConfigUpdate(void)
{
	/* Default handler does nothing. */
}

void DbObject::OnStatusUpdate(void)
{
	/* Default handler does nothing. */
}

DbObject::Ptr DbObject::GetOrCreateByObject(const DynamicObject::Ptr& object)
{
	DbObject::Ptr dbobj = static_pointer_cast<DbObject>(object->GetExtension("DbObject"));

	if (dbobj)
		return dbobj;

	DbType::Ptr dbtype = DbType::GetByName(object->GetType()->GetName());

	if (!dbtype)
		return DbObject::Ptr();

	Service::Ptr service;
	String name1, name2;

	service = dynamic_pointer_cast<Service>(object);

	if (service) {
		Host::Ptr host = service->GetHost();

		if (!host)
			return DbObject::Ptr();

		name1 = service->GetHost()->GetName();
		name2 = service->GetShortName();
	} else {
		name1 = object->GetName();
	}

	dbobj = dbtype->GetOrCreateObjectByName(name1, name2);

	{
		ObjectLock olock(object);
		dbobj->SetObject(object);
		object->SetExtension("DbObject", dbobj);
	}

	return dbobj;
}

void DbObject::StateChangedHandler(const DynamicObject::Ptr& object)
{
	DbObject::Ptr dbobj = GetOrCreateByObject(object);

	if (!dbobj)
		return;

	dbobj->SendStatusUpdate();
}
