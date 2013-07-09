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

#include "livestatus/servicegroupstable.h"
#include "icinga/servicegroup.h"
#include "base/dynamictype.h"
#include <boost/foreach.hpp>

using namespace icinga;
using namespace livestatus;

ServiceGroupsTable::ServiceGroupsTable(void)
{
	AddColumns(this);
}

void ServiceGroupsTable::AddColumns(Table *table, const String& prefix,
    const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "name", Column(&ServiceGroupsTable::NameAccessor, objectAccessor));
	table->AddColumn(prefix + "alias", Column(&ServiceGroupsTable::AliasAccessor, objectAccessor));
	table->AddColumn(prefix + "members", Column(&ServiceGroupsTable::MembersAccessor, objectAccessor));
}

String ServiceGroupsTable::GetName(void) const
{
	return "servicegroups";
}

void ServiceGroupsTable::FetchRows(const AddRowFunction& addRowFn)
{
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("UserGroup")) {
		addRowFn(object);
	}
}

Value ServiceGroupsTable::NameAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<ServiceGroup>(object)->GetName();
}

Value ServiceGroupsTable::AliasAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<ServiceGroup>(object)->GetName();
}

Value ServiceGroupsTable::MembersAccessor(const Object::Ptr& object)
{
	Array::Ptr members = boost::make_shared<Array>();

	BOOST_FOREACH(const Service::Ptr& service, static_pointer_cast<ServiceGroup>(object)->GetMembers()) {
		members->Add(service->GetName());
	}

	return members;
}
