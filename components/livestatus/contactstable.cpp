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
	AddColumns(this);
}

void ContactsTable::AddColumns(Table *table, const String& prefix,
	const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "name", Column(&ContactsTable::NameAccessor, objectAccessor));
	table->AddColumn(prefix + "alias", Column(&ContactsTable::NameAccessor, objectAccessor));
	table->AddColumn(prefix + "email", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "pager", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "host_notification_period", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "service_notification_period", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "can_submit_commands", Column(&Table::OneAccessor, objectAccessor));
	table->AddColumn(prefix + "host_notifications_enabled", Column(&Table::OneAccessor, objectAccessor));
	table->AddColumn(prefix + "service_notifications_enabled", Column(&Table::OneAccessor, objectAccessor));
	table->AddColumn(prefix + "in_host_notification_period", Column(&Table::OneAccessor, objectAccessor));
	table->AddColumn(prefix + "in_service_notification_period", Column(&Table::OneAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variable_names", Column(&Table::EmptyArrayAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variable_values", Column(&Table::EmptyArrayAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variables", Column(&Table::EmptyDictionaryAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes_list", Column(&Table::EmptyArrayAccessor, objectAccessor));
}

String ContactsTable::GetName(void) const
{
	return "contacts";
}

void ContactsTable::FetchRows(const AddRowFunction& addRowFn)
{
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("User")) {
		addRowFn(object);
	}
}

Value ContactsTable::NameAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<User>(object)->GetName();
}
