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

#include "db_ido/hostgroupdbobject.h"
#include "db_ido/dbtype.h"
#include "db_ido/dbvalue.h"
#include "base/objectlock.h"
#include "base/initialize.h"
#include "base/dynamictype.h"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_DBTYPE(HostGroup, "hostgroup", DbObjectTypeHostGroup, "hostgroup_object_id", HostGroupDbObject);

HostGroupDbObject::HostGroupDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr HostGroupDbObject::GetConfigFields(void) const
{
	Dictionary::Ptr fields = boost::make_shared<Dictionary>();
	HostGroup::Ptr group = static_pointer_cast<HostGroup>(GetObject());

	fields->Set("alias", group->GetDisplayName());

	return fields;
}

Dictionary::Ptr HostGroupDbObject::GetStatusFields(void) const
{
	return Empty;
}

void HostGroupDbObject::OnConfigUpdate(void)
{
	HostGroup::Ptr group = static_pointer_cast<HostGroup>(GetObject());

	DbQuery query1;
	query1.Table = DbType::GetByName("HostGroup")->GetTable() + "_members";
	query1.Type = DbQueryDelete;
	query1.WhereCriteria = boost::make_shared<Dictionary>();
	query1.WhereCriteria->Set("hostgroup_id", DbValue::FromObjectInsertID(group));
	OnQuery(query1);

	BOOST_FOREACH(const Host::Ptr& host, group->GetMembers()) {
		DbQuery query2;
		query2.Table = DbType::GetByName("HostGroup")->GetTable() + "_members";
		query2.Type = DbQueryInsert;
		query2.Fields = boost::make_shared<Dictionary>();
		query2.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */
		query2.Fields->Set("hostgroup_id", DbValue::FromObjectInsertID(group));
		query2.Fields->Set("host_object_id", host);
		OnQuery(query2);
	}
}
