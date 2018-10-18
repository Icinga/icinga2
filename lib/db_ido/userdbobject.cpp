/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

using namespace icinga;

REGISTER_DBTYPE(User, "contact", DbObjectTypeContact, "contact_object_id", UserDbObject);

UserDbObject::UserDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr UserDbObject::GetConfigFields() const
{
	User::Ptr user = static_pointer_cast<User>(GetObject());

	int typeFilter = user->GetTypeFilter();
	int stateFilter = user->GetStateFilter();

	return new Dictionary({
		{ "alias", user->GetDisplayName() },
		{ "email_address", user->GetEmail() },
		{ "pager_address", user->GetPager() },
		{ "host_timeperiod_object_id", user->GetPeriod() },
		{ "service_timeperiod_object_id", user->GetPeriod() },
		{ "host_notifications_enabled", user->GetEnableNotifications() },
		{ "service_notifications_enabled", user->GetEnableNotifications() },
		{ "can_submit_commands", 1 },
		{ "notify_service_recovery", (typeFilter & NotificationRecovery) ? 1 : 0 },
		{ "notify_service_warning", (stateFilter & StateFilterWarning) ? 1 : 0 },
		{ "notify_service_unknown", (stateFilter & StateFilterUnknown) ? 1 : 0 },
		{ "notify_service_critical", (stateFilter & StateFilterCritical) ? 1 : 0 },
		{ "notify_service_flapping", (typeFilter & (NotificationFlappingStart | NotificationFlappingEnd)) ? 1 : 0 },
		{ "notify_service_downtime", (typeFilter & (NotificationDowntimeStart | NotificationDowntimeEnd | NotificationDowntimeRemoved)) ? 1 : 0 },
		{ "notify_host_recovery", (typeFilter & NotificationRecovery) ? 1 : 0 },
		{ "notify_host_down", (stateFilter & StateFilterDown) ? 1 : 0 },
		{ "notify_host_flapping", (typeFilter & (NotificationFlappingStart | NotificationFlappingEnd)) ? 1 : 0 },
		{ "notify_host_downtime", (typeFilter & (NotificationDowntimeStart | NotificationDowntimeEnd | NotificationDowntimeRemoved)) ? 1 : 0 }
	});
}

Dictionary::Ptr UserDbObject::GetStatusFields() const
{
	User::Ptr user = static_pointer_cast<User>(GetObject());

	return new Dictionary({
		{ "host_notifications_enabled", user->GetEnableNotifications() },
		{ "service_notifications_enabled", user->GetEnableNotifications() },
		{ "last_host_notification", DbValue::FromTimestamp(user->GetLastNotification()) },
		{ "last_service_notification", DbValue::FromTimestamp(user->GetLastNotification()) }
	});
}

void UserDbObject::OnConfigUpdateHeavy()
{
	User::Ptr user = static_pointer_cast<User>(GetObject());

	/* groups */
	Array::Ptr groups = user->GetGroups();

	std::vector<DbQuery> queries;

	DbQuery query1;
	query1.Table = DbType::GetByName("UserGroup")->GetTable() + "_members";
	query1.Type = DbQueryDelete;
	query1.Category = DbCatConfig;
	query1.WhereCriteria = new Dictionary({
		{ "contact_object_id", user }
	});
	queries.emplace_back(std::move(query1));

	if (groups) {
		ObjectLock olock(groups);
		for (const String& groupName : groups) {
			UserGroup::Ptr group = UserGroup::GetByName(groupName);

			DbQuery query2;
			query2.Table = DbType::GetByName("UserGroup")->GetTable() + "_members";
			query2.Type = DbQueryInsert | DbQueryUpdate;
			query2.Category = DbCatConfig;
			query2.Fields = new Dictionary({
				{ "instance_id", 0 }, /* DbConnection class fills in real ID */
				{ "contactgroup_id", DbValue::FromObjectInsertID(group) },
				{ "contact_object_id", user }
			});
			query2.WhereCriteria = new Dictionary({
				{ "instance_id", 0 }, /* DbConnection class fills in real ID */
				{ "contactgroup_id", DbValue::FromObjectInsertID(group) },
				{ "contact_object_id", user }
			});
			queries.emplace_back(std::move(query2));
		}
	}

	DbObject::OnMultipleQueries(queries);

	queries.clear();

	DbQuery query2;
	query2.Table = "contact_addresses";
	query2.Type = DbQueryDelete;
	query2.Category = DbCatConfig;
	query2.WhereCriteria = new Dictionary({
		{ "contact_id", DbValue::FromObjectInsertID(user) }
	});
	queries.emplace_back(std::move(query2));

	Dictionary::Ptr vars = user->GetVars();

	if (vars) { /* This is sparta. */
		for (int i = 1; i <= 6; i++) {
			String key = "address" + Convert::ToString(i);

			if (!vars->Contains(key))
				continue;

			String val = vars->Get(key);

			DbQuery query;
			query.Type = DbQueryInsert;
			query.Table = "contact_addresses";
			query.Category = DbCatConfig;
			query.Fields = new Dictionary({
				{ "contact_id", DbValue::FromObjectInsertID(user) },
				{ "address_number", i },
				{ "address", val },
				{ "instance_id", 0 } /* DbConnection class fills in real ID */

			});
			queries.emplace_back(std::move(query));
		}
	}

	DbObject::OnMultipleQueries(queries);
}

String UserDbObject::CalculateConfigHash(const Dictionary::Ptr& configFields) const
{
	String hashData = DbObject::CalculateConfigHash(configFields);

	User::Ptr user = static_pointer_cast<User>(GetObject());

	Array::Ptr groups = user->GetGroups();

	if (groups)
		hashData += DbObject::HashValue(groups);

	return SHA256(hashData);
}
