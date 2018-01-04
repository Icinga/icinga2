/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "livestatus/contactgroupstable.hpp"
#include "icinga/usergroup.hpp"
#include "base/configtype.hpp"

using namespace icinga;

ContactGroupsTable::ContactGroupsTable()
{
	AddColumns(this);
}

void ContactGroupsTable::AddColumns(Table *table, const String& prefix,
	const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "name", Column(&ContactGroupsTable::NameAccessor, objectAccessor));
	table->AddColumn(prefix + "alias", Column(&ContactGroupsTable::AliasAccessor, objectAccessor));
	table->AddColumn(prefix + "members", Column(&ContactGroupsTable::MembersAccessor, objectAccessor));
}

String ContactGroupsTable::GetName() const
{
	return "contactgroups";
}

String ContactGroupsTable::GetPrefix() const
{
	return "contactgroup";
}

void ContactGroupsTable::FetchRows(const AddRowFunction& addRowFn)
{
	for (const UserGroup::Ptr& ug : ConfigType::GetObjectsByType<UserGroup>()) {
		if (!addRowFn(ug, LivestatusGroupByNone, Empty))
			return;
	}
}

Value ContactGroupsTable::NameAccessor(const Value& row)
{
	UserGroup::Ptr user_group = static_cast<UserGroup::Ptr>(row);

	if (!user_group)
		return Empty;

	return user_group->GetName();
}

Value ContactGroupsTable::AliasAccessor(const Value& row)
{
	UserGroup::Ptr user_group = static_cast<UserGroup::Ptr>(row);

	if (!user_group)
		return Empty;

	return user_group->GetName();
}

Value ContactGroupsTable::MembersAccessor(const Value& row)
{
	UserGroup::Ptr user_group = static_cast<UserGroup::Ptr>(row);

	if (!user_group)
		return Empty;

	Array::Ptr members = new Array();

	for (const User::Ptr& user : user_group->GetMembers()) {
		members->Add(user->GetName());
	}

	return members;
}
