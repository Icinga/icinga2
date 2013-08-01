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

#include "ido/userdbobject.h"
#include "ido/dbtype.h"
#include "ido/dbvalue.h"
#include "icinga/user.h"
#include "base/objectlock.h"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_DBTYPE(User, "contact", 10, UserDbObject);

UserDbObject::UserDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr UserDbObject::GetConfigFields(void) const
{
	Dictionary::Ptr fields = boost::make_shared<Dictionary>();
	User::Ptr user = static_pointer_cast<User>(GetObject());

	fields->Set("alias", Empty);
	fields->Set("email_address", Empty);
	fields->Set("pager_address", Empty);
	fields->Set("host_timeperiod_object_id", Empty);
	fields->Set("service_timeperiod_object_id", Empty);
	fields->Set("host_notifications_enabled", Empty);
	fields->Set("service_notifications_enabled", Empty);
	fields->Set("can_submit_commands", Empty);
	fields->Set("notify_service_recovery", Empty);
	fields->Set("notify_service_warning", Empty);
	fields->Set("notify_service_unknown", Empty);
	fields->Set("notify_service_critical", Empty);
	fields->Set("notify_service_flapping", Empty);
	fields->Set("notify_service_downtime", Empty);
	fields->Set("notify_host_recovery", Empty);
	fields->Set("notify_host_down", Empty);
	fields->Set("notify_host_unreachable", Empty);
	fields->Set("notify_host_flapping", Empty);
	fields->Set("notify_host_downtime", Empty);

	return fields;
}

Dictionary::Ptr UserDbObject::GetStatusFields(void) const
{
	Dictionary::Ptr fields = boost::make_shared<Dictionary>();
	User::Ptr user = static_pointer_cast<User>(GetObject());

	fields->Set("host_notifications_enabled", Empty);
	fields->Set("service_notifications_enabled", Empty);
	fields->Set("last_host_notification", Empty);
	fields->Set("last_service_notification", Empty);
	fields->Set("modified_attributes", Empty);
	fields->Set("modified_host_attributes", Empty);
	fields->Set("modified_service_attributes", Empty);

	return fields;
}
