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

#include "db_ido/hostgroupdbobject.hpp"
#include "db_ido/dbtype.hpp"
#include "db_ido/dbvalue.hpp"
#include "base/objectlock.hpp"
#include "base/initialize.hpp"
#include "base/dynamictype.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_DBTYPE(HostGroup, "hostgroup", DbObjectTypeHostGroup, "hostgroup_object_id", HostGroupDbObject);

HostGroupDbObject::HostGroupDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr HostGroupDbObject::GetConfigFields(void) const
{
	Dictionary::Ptr fields = new Dictionary();
	HostGroup::Ptr group = static_pointer_cast<HostGroup>(GetObject());

	fields->Set("alias", group->GetDisplayName());
	fields->Set("notes", group->GetNotes());
	fields->Set("notes_url", group->GetNotesUrl());
	fields->Set("action_url", group->GetActionUrl());

	return fields;
}

Dictionary::Ptr HostGroupDbObject::GetStatusFields(void) const
{
	return Empty;
}

void HostGroupDbObject::OnConfigUpdate(void)
{
	HostGroup::Ptr group = static_pointer_cast<HostGroup>(GetObject());

	BOOST_FOREACH(const Host::Ptr& host, group->GetMembers()) {
		DbQuery query1;
		query1.Table = DbType::GetByName("HostGroup")->GetTable() + "_members";
		query1.Type = DbQueryInsert;
		query1.Category = DbCatConfig;
		query1.Fields = new Dictionary();
		query1.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */
		query1.Fields->Set("hostgroup_id", DbValue::FromObjectInsertID(group));
		query1.Fields->Set("host_object_id", host);
		OnQuery(query1);
	}
}
