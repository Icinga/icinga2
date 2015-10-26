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

#include "db_ido/servicegroupdbobject.hpp"
#include "db_ido/dbtype.hpp"
#include "db_ido/dbvalue.hpp"
#include "base/objectlock.hpp"
#include "base/initialize.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_DBTYPE(ServiceGroup, "servicegroup", DbObjectTypeServiceGroup, "servicegroup_object_id", ServiceGroupDbObject);

ServiceGroupDbObject::ServiceGroupDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr ServiceGroupDbObject::GetConfigFields(void) const
{
	Dictionary::Ptr fields = new Dictionary();
	ServiceGroup::Ptr group = static_pointer_cast<ServiceGroup>(GetObject());

	fields->Set("alias", group->GetDisplayName());
	fields->Set("notes", group->GetNotes());
	fields->Set("notes_url", group->GetNotesUrl());
	fields->Set("action_url", group->GetActionUrl());

	return fields;
}

Dictionary::Ptr ServiceGroupDbObject::GetStatusFields(void) const
{
	return Empty;
}

void ServiceGroupDbObject::OnConfigUpdate(void)
{
	ServiceGroup::Ptr group = static_pointer_cast<ServiceGroup>(GetObject());

	DbQuery query1;
	query1.Table = DbType::GetByName("ServiceGroup")->GetTable() + "_members";
	query1.Type = DbQueryDelete;
	query1.Category = DbCatConfig;
	query1.WhereCriteria = new Dictionary();
	query1.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */
	query1.WhereCriteria->Set("servicegroup_id", DbValue::FromObjectInsertID(group));
	OnQuery(query1);

	BOOST_FOREACH(const Service::Ptr& service, group->GetMembers()) {
		DbQuery query2;
		query2.Table = DbType::GetByName("ServiceGroup")->GetTable() + "_members";
		query2.Type = DbQueryInsert;
		query2.Category = DbCatConfig;
		query2.Fields = new Dictionary();
		query2.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */
		query2.Fields->Set("servicegroup_id", DbValue::FromObjectInsertID(group));
		query2.Fields->Set("service_object_id", service);
		OnQuery(query2);
	}
}
