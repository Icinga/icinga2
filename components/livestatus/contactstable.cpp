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

#include "i2-livestatus.h"

using namespace icinga;
using namespace livestatus;

ContactsTable::ContactsTable(void)
{
	AddColumn("name", &ContactsTable::NameAccessor);
	AddColumn("alias", &ContactsTable::NameAccessor);
	AddColumn("email", &Table::EmptyStringAccessor);
	AddColumn("pager", &Table::EmptyStringAccessor);
	AddColumn("host_notification_period", &Table::EmptyStringAccessor);
	AddColumn("service_notification_period", &Table::EmptyStringAccessor);
	AddColumn("can_submit_commands", &Table::OneAccessor);
	AddColumn("host_notifications_enabled", &Table::OneAccessor);
	AddColumn("service_notifications_enabled", &Table::OneAccessor);
	AddColumn("in_host_notification_period", &Table::OneAccessor);
	AddColumn("in_service_notification_period", &Table::OneAccessor);
	AddColumn("custom_variable_names", &Table::EmptyArrayAccessor);
	AddColumn("custom_variable_values", &Table::EmptyArrayAccessor);
	AddColumn("custom_variables", &Table::EmptyDictionaryAccessor);
	AddColumn("modified_attributes", &Table::ZeroAccessor);
	AddColumn("modified_attributes_list", &Table::EmptyArrayAccessor);
}

String ContactsTable::GetName(void) const
{
	return "contacts";
}

void ContactsTable::FetchRows(const function<void (const Object::Ptr&)>& addRowFn)
{
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("User")) {
		addRowFn(object);
	}
}

Value ContactsTable::NameAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<User>(object)->GetName();
}
