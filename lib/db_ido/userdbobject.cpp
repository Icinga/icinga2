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

#include "db_ido/userdbobject.h"
#include "db_ido/dbtype.h"
#include "db_ido/dbvalue.h"
#include "icinga/user.h"
#include "icinga/notification.h"
#include "base/convert.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_DBTYPE(User, "contact", DbObjectTypeContact, "contact_object_id", UserDbObject);

UserDbObject::UserDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr UserDbObject::GetConfigFields(void) const
{
	Dictionary::Ptr fields = boost::make_shared<Dictionary>();
	User::Ptr user = static_pointer_cast<User>(GetObject());

	fields->Set("alias", user->GetDisplayName());

	Dictionary::Ptr macros = user->GetMacros();

	if (macros) { /* Yuck. */
		fields->Set("email_address", macros->Get("email"));
		fields->Set("pager_address", macros->Get("pager"));
	}

	fields->Set("host_timeperiod_object_id", user->GetNotificationPeriod());
	fields->Set("service_timeperiod_object_id", user->GetNotificationPeriod());
	fields->Set("host_notifications_enabled", user->GetEnableNotifications());
	fields->Set("service_notifications_enabled", user->GetEnableNotifications());
	fields->Set("can_submit_commands", 1);
	fields->Set("notify_service_recovery", user->GetNotificationStateFilter() & NotificationRecovery);
	fields->Set("notify_service_warning", user->GetNotificationStateFilter() & NotificationProblem);
	fields->Set("notify_service_unknown", user->GetNotificationStateFilter() & NotificationProblem);
	fields->Set("notify_service_critical", user->GetNotificationStateFilter() & NotificationProblem);
	fields->Set("notify_service_flapping", user->GetNotificationStateFilter() & (NotificationFlappingStart | NotificationFlappingEnd));
	fields->Set("notify_service_downtime", user->GetNotificationStateFilter() & (NotificationDowntimeStart | NotificationDowntimeEnd | NotificationDowntimeRemoved));
	fields->Set("notify_host_recovery", user->GetNotificationStateFilter() & NotificationRecovery);
	fields->Set("notify_host_down", user->GetNotificationStateFilter() & NotificationProblem);
	fields->Set("notify_host_unreachable", user->GetNotificationStateFilter() & NotificationProblem);
	fields->Set("notify_host_flapping", user->GetNotificationStateFilter() & (NotificationFlappingStart | NotificationFlappingEnd));
	fields->Set("notify_host_downtime", user->GetNotificationStateFilter() & (NotificationDowntimeStart | NotificationDowntimeEnd | NotificationDowntimeRemoved));

	return fields;
}

Dictionary::Ptr UserDbObject::GetStatusFields(void) const
{
	Dictionary::Ptr fields = boost::make_shared<Dictionary>();
	User::Ptr user = static_pointer_cast<User>(GetObject());

	fields->Set("host_notifications_enabled", user->GetEnableNotifications());
	fields->Set("service_notifications_enabled", user->GetEnableNotifications());
	fields->Set("last_host_notification", DbValue::FromTimestamp(user->GetLastNotification()));
	fields->Set("last_service_notification", DbValue::FromTimestamp(user->GetLastNotification()));
	fields->Set("modified_attributes", Empty);
	fields->Set("modified_host_attributes", Empty);
	fields->Set("modified_service_attributes", Empty);

	return fields;
}

void UserDbObject::OnConfigUpdate(void)
{
	Dictionary::Ptr fields = boost::make_shared<Dictionary>();
	User::Ptr user = static_pointer_cast<User>(GetObject());

	/* contact addresses */
	Log(LogDebug, "db_ido", "contact addresses for '" + user->GetName() + "'");

	Dictionary::Ptr macros = user->GetMacros();

	if (macros) { /* This is sparta. */
		for (int i = 1; i <= 6; i++) {
			String key = "address" + Convert::ToString(i);
			String val = macros->Get(key);

			if (val.IsEmpty())
				continue;

			fields->Set("contact_id", DbValue::FromObjectInsertID(user));
			fields->Set("address_number", i);
			fields->Set("address", val);
			fields->Set("instance_id", 0); /* DbConnection class fills in real ID */

			DbQuery query;
			query.Type = DbQueryInsert;
			query.Table = "contact_addresses";
			query.Fields = fields;
			OnQuery(query);
		}
	}
}

bool UserDbObject::IsStatusAttribute(const String& attribute) const
{
	return (attribute == "last_notification");
}
