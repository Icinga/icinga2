/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "ido/usergroupdbobject.h"
#include "ido/dbtype.h"
#include "ido/dbvalue.h"
#include "icinga/usergroup.h"
#include "base/objectlock.h"
#include "base/initialize.h"
#include "base/dynamictype.h"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_DBTYPE(UserGroup, "contactgroup", DbObjectTypeContactGroup, "contactgroup_object_id", UserGroupDbObject);
INITIALIZE_ONCE(UserGroupDbObject, &UserGroupDbObject::StaticInitialize);

UserGroupDbObject::UserGroupDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

void UserGroupDbObject::StaticInitialize(void)
{
	UserGroup::OnMembersChanged.connect(&UserGroupDbObject::MembersChangedHandler);
}

Dictionary::Ptr UserGroupDbObject::GetConfigFields(void) const
{
	Dictionary::Ptr fields = boost::make_shared<Dictionary>();
	UserGroup::Ptr group = static_pointer_cast<UserGroup>(GetObject());

	fields->Set("alias", Empty);

	return fields;
}

Dictionary::Ptr UserGroupDbObject::GetStatusFields(void) const
{
	return Empty;
}

void UserGroupDbObject::OnConfigUpdate(void)
{
	MembersChangedHandler();
}

void UserGroupDbObject::MembersChangedHandler(void)
{
	DbQuery query1;
	query1.Table = DbType::GetByName("UserGroup")->GetTable() + "_members";
	query1.Type = DbQueryDelete;
	query1.WhereCriteria = boost::make_shared<Dictionary>();
	query1.WhereCriteria->Set("instance_id", 0);
	OnQuery(query1);

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("UserGroup")) {
		UserGroup::Ptr ug = static_pointer_cast<UserGroup>(object);

		BOOST_FOREACH(const User::Ptr& user, ug->GetMembers()) {
			DbQuery query2;
			query2.Table = DbType::GetByName("UserGroup")->GetTable() + "_members";
			query2.Type = DbQueryInsert;
			query2.Fields = boost::make_shared<Dictionary>();
			query2.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */
			query2.Fields->Set("contactgroup_id", DbValue::FromObjectInsertID(ug));
			query2.Fields->Set("contact_object_id", user);
			OnQuery(query2);
		}
	}
}
