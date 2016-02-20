/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "db_ido/userdbobject.hpp"
#include "db_ido/usergroupdbobject.hpp"
#include "db_ido/dbtype.hpp"
#include "db_ido/dbvalue.hpp"
#include "icinga/user.hpp"
#include "icinga/notification.hpp"
#include "base/convert.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_DBTYPE(User, "contact", DbObjectTypeContact, "contact_object_id", UserDbObject);

UserDbObject::UserDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr UserDbObject::GetConfigFields(void) const
{
	Dictionary::Ptr fields = new Dictionary();
	User::Ptr user = static_pointer_cast<User>(GetObject());

	fields->Set("alias", user->GetDisplayName());
	fields->Set("email_address", user->GetEmail());
	fields->Set("pager_address", user->GetPager());
	fields->Set("host_timeperiod_object_id", user->GetPeriod());
	fields->Set("service_timeperiod_object_id", user->GetPeriod());
	fields->Set("host_notifications_enabled", user->GetEnableNotifications());
	fields->Set("service_notifications_enabled", user->GetEnableNotifications());
	fields->Set("can_submit_commands", 1);
	fields->Set("notify_service_recovery", (user->GetTypeFilter() & NotificationRecovery) != 0);
	fields->Set("notify_service_warning", (user->GetStateFilter() & StateFilterWarning) != 0);
	fields->Set("notify_service_unknown", (user->GetStateFilter() & StateFilterUnknown) != 0);
	fields->Set("notify_service_critical", (user->GetStateFilter() & StateFilterCritical) != 0);
	fields->Set("notify_service_flapping", (user->GetTypeFilter() & (NotificationFlappingStart | NotificationFlappingEnd)) != 0);
	fields->Set("notify_service_downtime", (user->GetTypeFilter() & (NotificationDowntimeStart | NotificationDowntimeEnd | NotificationDowntimeRemoved)) != 0);
	fields->Set("notify_host_recovery", (user->GetTypeFilter() & NotificationRecovery) != 0);
	fields->Set("notify_host_down", (user->GetStateFilter() & StateFilterDown) != 0);
	fields->Set("notify_host_flapping", (user->GetTypeFilter() & (NotificationFlappingStart | NotificationFlappingEnd)) != 0);
	fields->Set("notify_host_downtime", (user->GetTypeFilter() & (NotificationDowntimeStart | NotificationDowntimeEnd | NotificationDowntimeRemoved)) != 0);

	return fields;
}

Dictionary::Ptr UserDbObject::GetStatusFields(void) const
{
	Dictionary::Ptr fields = new Dictionary();
	User::Ptr user = static_pointer_cast<User>(GetObject());

	fields->Set("host_notifications_enabled", user->GetEnableNotifications());
	fields->Set("service_notifications_enabled", user->GetEnableNotifications());
	fields->Set("last_host_notification", DbValue::FromTimestamp(user->GetLastNotification()));
	fields->Set("last_service_notification", DbValue::FromTimestamp(user->GetLastNotification()));

	return fields;
}

void UserDbObject::OnConfigUpdate(void)
{
	Dictionary::Ptr fields = new Dictionary();
	User::Ptr user = static_pointer_cast<User>(GetObject());

	/* groups */
	Array::Ptr groups = user->GetGroups();

	if (groups) {
		ObjectLock olock(groups);
		BOOST_FOREACH(const String& groupName, groups) {
			UserGroup::Ptr group = UserGroup::GetByName(groupName);

			DbQuery query1;
			query1.Table = DbType::GetByName("UserGroup")->GetTable() + "_members";
			query1.Type = DbQueryDelete;
			query1.Category = DbCatConfig;
			query1.WhereCriteria = new Dictionary();
			query1.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */
			query1.WhereCriteria->Set("contactgroup_id", DbValue::FromObjectInsertID(group));
			query1.WhereCriteria->Set("contact_object_id", user);
			OnQuery(query1);

			DbQuery query2;
			query2.Table = DbType::GetByName("UserGroup")->GetTable() + "_members";
			query2.Type = DbQueryInsert;
			query2.Category = DbCatConfig;
			query2.Fields = new Dictionary();
			query2.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */
			query2.Fields->Set("contactgroup_id", DbValue::FromObjectInsertID(group));
			query2.Fields->Set("contact_object_id", user);
			OnQuery(query2);
		}
	}

	/* contact addresses */
	Log(LogDebug, "UserDbObject")
	    << "contact addresses for '" << user->GetName() << "'";

	Dictionary::Ptr vars = user->GetVars();

	if (vars) { /* This is sparta. */
		for (int i = 1; i <= 6; i++) {
			String key = "address" + Convert::ToString(i);

			if (!vars->Contains(key))
				continue;

			String val = vars->Get(key);

			fields->Set("contact_id", DbValue::FromObjectInsertID(user));
			fields->Set("address_number", i);
			fields->Set("address", val);
			fields->Set("instance_id", 0); /* DbConnection class fills in real ID */

			DbQuery query;
			query.Type = DbQueryInsert;
			query.Table = "contact_addresses";
			query.Category = DbCatConfig;
			query.Fields = fields;
			OnQuery(query);
		}
	}
}

bool UserDbObject::IsStatusAttribute(const String& attribute) const
{
	return (attribute == "last_notification");
}
