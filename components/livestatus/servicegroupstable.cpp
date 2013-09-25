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
	table->AddColumn(prefix + "notes", Column(&ServiceGroupsTable::NotesAccessor, objectAccessor));
	table->AddColumn(prefix + "notes_url", Column(&ServiceGroupsTable::NotesUrlAccessor, objectAccessor));
	table->AddColumn(prefix + "action_url", Column(&ServiceGroupsTable::ActionUrlAccessor, objectAccessor));
	table->AddColumn(prefix + "members", Column(&ServiceGroupsTable::MembersAccessor, objectAccessor));
	table->AddColumn(prefix + "members_with_state", Column(&ServiceGroupsTable::MembersWithStateAccessor, objectAccessor));
	table->AddColumn(prefix + "worst_service_state", Column(&ServiceGroupsTable::WorstServiceStateAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services", Column(&ServiceGroupsTable::NumServicesAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_ok", Column(&ServiceGroupsTable::NumServicesOkAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_warn", Column(&ServiceGroupsTable::NumServicesWarnAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_crit", Column(&ServiceGroupsTable::NumServicesCritAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_unknown", Column(&ServiceGroupsTable::NumServicesUnknownAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_pending", Column(&ServiceGroupsTable::NumServicesPendingAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_hard_ok", Column(&ServiceGroupsTable::NumServicesHardOkAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_hard_warn", Column(&ServiceGroupsTable::NumServicesHardWarnAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_hard_crit", Column(&ServiceGroupsTable::NumServicesHardCritAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_hard_unknown", Column(&ServiceGroupsTable::NumServicesHardUnknownAccessor, objectAccessor));
}

String ServiceGroupsTable::GetName(void) const
{
	return "servicegroups";
}

void ServiceGroupsTable::FetchRows(const AddRowFunction& addRowFn)
{
	BOOST_FOREACH(const ServiceGroup::Ptr& sg, DynamicType::GetObjects<ServiceGroup>()) {
		addRowFn(sg);
	}
}

Value ServiceGroupsTable::NameAccessor(const Value& row)
{
	return static_cast<ServiceGroup::Ptr>(row)->GetName();
}

Value ServiceGroupsTable::AliasAccessor(const Value& row)
{
	return static_cast<ServiceGroup::Ptr>(row)->GetDisplayName();
}

Value ServiceGroupsTable::NotesAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<ServiceGroup::Ptr>(row)->GetCustom();

	if (!custom)
		return Empty;

	return custom->Get("notes");
}

Value ServiceGroupsTable::NotesUrlAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<ServiceGroup::Ptr>(row)->GetCustom();

	if (!custom)
		return Empty;

	return custom->Get("notes_url");
}

Value ServiceGroupsTable::ActionUrlAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<ServiceGroup::Ptr>(row)->GetCustom();

	if (!custom)
		return Empty;

	return custom->Get("action_url");
}

Value ServiceGroupsTable::MembersAccessor(const Value& row)
{
	Array::Ptr members = boost::make_shared<Array>();

	BOOST_FOREACH(const Service::Ptr& service, static_cast<ServiceGroup::Ptr>(row)->GetMembers()) {
		Array::Ptr host_svc = boost::make_shared<Array>();
		host_svc->Add(service->GetHost()->GetName());
		host_svc->Add(service->GetShortName());
		members->Add(host_svc);
	}

	return members;
}

Value ServiceGroupsTable::MembersWithStateAccessor(const Value& row)
{
	Array::Ptr members = boost::make_shared<Array>();

	BOOST_FOREACH(const Service::Ptr& service, static_cast<ServiceGroup::Ptr>(row)->GetMembers()) {
		Array::Ptr host_svc = boost::make_shared<Array>();
		host_svc->Add(service->GetHost()->GetName());
		host_svc->Add(service->GetShortName());
		host_svc->Add(service->GetHost()->GetState());
		host_svc->Add(service->GetState());
		members->Add(host_svc);
	}

	return members;
}

Value ServiceGroupsTable::WorstServiceStateAccessor(const Value& row)
{
	Value worst_service = StateOK;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<ServiceGroup::Ptr>(row)->GetMembers()) {
		if (service->GetState() > worst_service)
			worst_service = service->GetState();
	}

	return worst_service;
}

Value ServiceGroupsTable::NumServicesAccessor(const Value& row)
{
	return static_cast<long>(static_cast<ServiceGroup::Ptr>(row)->GetMembers().size());
}

Value ServiceGroupsTable::NumServicesOkAccessor(const Value& row)
{
	int num_services = 0;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<ServiceGroup::Ptr>(row)->GetMembers()) {
		if (service->GetState() == StateOK)
			num_services++;
	}

	return num_services;
}

Value ServiceGroupsTable::NumServicesWarnAccessor(const Value& row)
{
	int num_services = 0;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<ServiceGroup::Ptr>(row)->GetMembers()) {
		if (service->GetState() == StateWarning)
			num_services++;
	}

	return num_services;
}

Value ServiceGroupsTable::NumServicesCritAccessor(const Value& row)
{
	int num_services = 0;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<ServiceGroup::Ptr>(row)->GetMembers()) {
		if (service->GetState() == StateCritical)
			num_services++;
	}

	return num_services;
}

Value ServiceGroupsTable::NumServicesUnknownAccessor(const Value& row)
{
	int num_services = 0;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<ServiceGroup::Ptr>(row)->GetMembers()) {
		if (service->GetState() == StateUnknown)
			num_services++;
	}

	return num_services;
}

Value ServiceGroupsTable::NumServicesPendingAccessor(const Value& row)
{
	int num_services = 0;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<ServiceGroup::Ptr>(row)->GetMembers()) {
		if (!service->GetLastCheckResult())
			num_services++;
	}

	return num_services;
}

Value ServiceGroupsTable::NumServicesHardOkAccessor(const Value& row)
{
	int num_services = 0;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<ServiceGroup::Ptr>(row)->GetMembers()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == StateOK)
			num_services++;
	}

	return num_services;
}

Value ServiceGroupsTable::NumServicesHardWarnAccessor(const Value& row)
{
	int num_services = 0;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<ServiceGroup::Ptr>(row)->GetMembers()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == StateWarning)
			num_services++;
	}

	return num_services;
}

Value ServiceGroupsTable::NumServicesHardCritAccessor(const Value& row)
{
	int num_services = 0;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<ServiceGroup::Ptr>(row)->GetMembers()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == StateCritical)
			num_services++;
	}

	return num_services;
}

Value ServiceGroupsTable::NumServicesHardUnknownAccessor(const Value& row)
{
	int num_services = 0;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<ServiceGroup::Ptr>(row)->GetMembers()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == StateUnknown)
			num_services++;
	}

	return num_services;
}
