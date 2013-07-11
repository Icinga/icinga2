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

#include "livestatus/hostgroupstable.h"
#include "icinga/hostgroup.h"
#include "base/dynamictype.h"
#include <boost/foreach.hpp>

using namespace icinga;
using namespace livestatus;

HostGroupsTable::HostGroupsTable(void)
{
	AddColumns(this);
}

void HostGroupsTable::AddColumns(Table *table, const String& prefix,
    const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "name", Column(&HostGroupsTable::NameAccessor, objectAccessor));
	table->AddColumn(prefix + "alias", Column(&HostGroupsTable::AliasAccessor, objectAccessor));
	table->AddColumn(prefix + "notes", Column(&HostGroupsTable::NotesAccessor, objectAccessor));
	table->AddColumn(prefix + "notes_url", Column(&HostGroupsTable::NotesUrlAccessor, objectAccessor));
	table->AddColumn(prefix + "action_url", Column(&HostGroupsTable::ActionUrlAccessor, objectAccessor));
	table->AddColumn(prefix + "members", Column(&HostGroupsTable::MembersAccessor, objectAccessor));
	table->AddColumn(prefix + "members_with_state", Column(&HostGroupsTable::MembersWithStateAccessor, objectAccessor));
	table->AddColumn(prefix + "worst_host_state", Column(&HostGroupsTable::WorstHostStateAccessor, objectAccessor));
	table->AddColumn(prefix + "num_hosts", Column(&HostGroupsTable::NumHostsAccessor, objectAccessor));
	table->AddColumn(prefix + "num_hosts_pending", Column(&HostGroupsTable::NumHostsPendingAccessor, objectAccessor));
	table->AddColumn(prefix + "num_hosts_up", Column(&HostGroupsTable::NumHostsUpAccessor, objectAccessor));
	table->AddColumn(prefix + "num_hosts_down", Column(&HostGroupsTable::NumHostsDownAccessor, objectAccessor));
	table->AddColumn(prefix + "num_hosts_unreach", Column(&HostGroupsTable::NumHostsUnreachAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services", Column(&HostGroupsTable::NumServicesAccessor, objectAccessor));
	table->AddColumn(prefix + "worst_services_state", Column(&HostGroupsTable::WorstServicesStateAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_pending", Column(&HostGroupsTable::NumServicesPendingAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_ok", Column(&HostGroupsTable::NumServicesOkAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_warn", Column(&HostGroupsTable::NumServicesWarnAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_crit", Column(&HostGroupsTable::NumServicesCritAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_unknown", Column(&HostGroupsTable::NumServicesUnknownAccessor, objectAccessor));
	table->AddColumn(prefix + "worst_service_hard_state", Column(&HostGroupsTable::WorstServiceHardStateAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_hard_ok", Column(&HostGroupsTable::NumServicesHardOkAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_hard_warn", Column(&HostGroupsTable::NumServicesHardWarnAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_hard_crit", Column(&HostGroupsTable::NumServicesHardCritAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_hard_unknown", Column(&HostGroupsTable::NumServicesHardUnknownAccessor, objectAccessor));
}

String HostGroupsTable::GetName(void) const
{
	return "hostgroups";
}

void HostGroupsTable::FetchRows(const AddRowFunction& addRowFn)
{
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("HostGroup")) {
		addRowFn(object);
	}
}

Value HostGroupsTable::NameAccessor(const Value& row)
{
	return static_cast<HostGroup::Ptr>(row)->GetName();
}

Value HostGroupsTable::AliasAccessor(const Value& row)
{
	return static_cast<HostGroup::Ptr>(row)->GetName();
}

Value HostGroupsTable::NotesAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::NotesUrlAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::ActionUrlAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::MembersAccessor(const Value& row)
{
	Array::Ptr members = boost::make_shared<Array>();

	BOOST_FOREACH(const Host::Ptr& host, static_cast<HostGroup::Ptr>(row)->GetMembers()) {
		members->Add(host->GetName());
	}

	return members;
}

Value HostGroupsTable::MembersWithStateAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::WorstHostStateAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::NumHostsAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::NumHostsPendingAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::NumHostsUpAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::NumHostsDownAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::NumHostsUnreachAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::NumServicesAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::WorstServicesStateAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::NumServicesPendingAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::NumServicesOkAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::NumServicesWarnAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::NumServicesCritAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::NumServicesUnknownAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::WorstServiceHardStateAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::NumServicesHardOkAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::NumServicesHardWarnAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::NumServicesHardCritAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value HostGroupsTable::NumServicesHardUnknownAccessor(const Value& row)
{
	/* TODO */
	return Value();
}
