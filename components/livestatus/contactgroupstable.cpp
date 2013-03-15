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

ContactGroupsTable::ContactGroupsTable(void)
{
	AddColumns(this);
}

void ContactGroupsTable::AddColumns(Table *table, const String& prefix,
    const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "name", Column(&ContactGroupsTable::NameAccessor, objectAccessor));
	table->AddColumn(prefix + "alias", Column(&ContactGroupsTable::NameAccessor, objectAccessor));
	table->AddColumn(prefix + "members", Column(&ContactGroupsTable::MembersAccessor, objectAccessor));
}

String ContactGroupsTable::GetName(void) const
{
	return "contactgroups";
}

void ContactGroupsTable::FetchRows(const AddRowFunction& addRowFn)
{
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("UserGroup")) {
		addRowFn(object);
	}
}

Value ContactGroupsTable::NameAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<UserGroup>(object)->GetName();
}

Value ContactGroupsTable::MembersAccessor(const Object::Ptr& object)
{
	Array::Ptr members = boost::make_shared<Array>();

	BOOST_FOREACH(const User::Ptr& user, static_pointer_cast<UserGroup>(object)->GetMembers()) {
		members->Add(user->GetName());
	}

	return members;
}
