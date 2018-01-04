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

#include "livestatus/servicegroupstable.hpp"
#include "icinga/servicegroup.hpp"
#include "base/configtype.hpp"

using namespace icinga;

ServiceGroupsTable::ServiceGroupsTable()
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

String ServiceGroupsTable::GetName() const
{
	return "servicegroups";
}

String ServiceGroupsTable::GetPrefix() const
{
	return "servicegroup";
}

void ServiceGroupsTable::FetchRows(const AddRowFunction& addRowFn)
{
	for (const ServiceGroup::Ptr& sg : ConfigType::GetObjectsByType<ServiceGroup>()) {
		if (!addRowFn(sg, LivestatusGroupByNone, Empty))
			return;
	}
}

Value ServiceGroupsTable::NameAccessor(const Value& row)
{
	ServiceGroup::Ptr sg = static_cast<ServiceGroup::Ptr>(row);

	if (!sg)
		return Empty;

	return sg->GetName();
}

Value ServiceGroupsTable::AliasAccessor(const Value& row)
{
	ServiceGroup::Ptr sg = static_cast<ServiceGroup::Ptr>(row);

	if (!sg)
		return Empty;

	return sg->GetDisplayName();
}

Value ServiceGroupsTable::NotesAccessor(const Value& row)
{
	ServiceGroup::Ptr sg = static_cast<ServiceGroup::Ptr>(row);

	if (!sg)
		return Empty;

	return sg->GetNotes();
}

Value ServiceGroupsTable::NotesUrlAccessor(const Value& row)
{
	ServiceGroup::Ptr sg = static_cast<ServiceGroup::Ptr>(row);

	if (!sg)
		return Empty;

	return sg->GetNotesUrl();
}

Value ServiceGroupsTable::ActionUrlAccessor(const Value& row)
{
	ServiceGroup::Ptr sg = static_cast<ServiceGroup::Ptr>(row);

	if (!sg)
		return Empty;

	return sg->GetActionUrl();
}

Value ServiceGroupsTable::MembersAccessor(const Value& row)
{
	ServiceGroup::Ptr sg = static_cast<ServiceGroup::Ptr>(row);

	if (!sg)
		return Empty;

	Array::Ptr members = new Array();

	for (const Service::Ptr& service : sg->GetMembers()) {
		Array::Ptr host_svc = new Array();
		host_svc->Add(service->GetHost()->GetName());
		host_svc->Add(service->GetShortName());
		members->Add(host_svc);
	}

	return members;
}

Value ServiceGroupsTable::MembersWithStateAccessor(const Value& row)
{
	ServiceGroup::Ptr sg = static_cast<ServiceGroup::Ptr>(row);

	if (!sg)
		return Empty;

	Array::Ptr members = new Array();

	for (const Service::Ptr& service : sg->GetMembers()) {
		Array::Ptr host_svc = new Array();
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
	ServiceGroup::Ptr sg = static_cast<ServiceGroup::Ptr>(row);

	if (!sg)
		return Empty;

	Value worst_service = ServiceOK;

	for (const Service::Ptr& service : sg->GetMembers()) {
		if (service->GetState() > worst_service)
			worst_service = service->GetState();
	}

	return worst_service;
}

Value ServiceGroupsTable::NumServicesAccessor(const Value& row)
{
	ServiceGroup::Ptr sg = static_cast<ServiceGroup::Ptr>(row);

	if (!sg)
		return Empty;

	return sg->GetMembers().size();
}

Value ServiceGroupsTable::NumServicesOkAccessor(const Value& row)
{
	ServiceGroup::Ptr sg = static_cast<ServiceGroup::Ptr>(row);

	if (!sg)
		return Empty;

	int num_services = 0;

	for (const Service::Ptr& service : sg->GetMembers()) {
		if (service->GetState() == ServiceOK)
			num_services++;
	}

	return num_services;
}

Value ServiceGroupsTable::NumServicesWarnAccessor(const Value& row)
{
	ServiceGroup::Ptr sg = static_cast<ServiceGroup::Ptr>(row);

	if (!sg)
		return Empty;

	int num_services = 0;

	for (const Service::Ptr& service : sg->GetMembers()) {
		if (service->GetState() == ServiceWarning)
			num_services++;
	}

	return num_services;
}

Value ServiceGroupsTable::NumServicesCritAccessor(const Value& row)
{
	ServiceGroup::Ptr sg = static_cast<ServiceGroup::Ptr>(row);

	if (!sg)
		return Empty;

	int num_services = 0;

	for (const Service::Ptr& service : sg->GetMembers()) {
		if (service->GetState() == ServiceCritical)
			num_services++;
	}

	return num_services;
}

Value ServiceGroupsTable::NumServicesUnknownAccessor(const Value& row)
{
	ServiceGroup::Ptr sg = static_cast<ServiceGroup::Ptr>(row);

	if (!sg)
		return Empty;

	int num_services = 0;

	for (const Service::Ptr& service : sg->GetMembers()) {
		if (service->GetState() == ServiceUnknown)
			num_services++;
	}

	return num_services;
}

Value ServiceGroupsTable::NumServicesPendingAccessor(const Value& row)
{
	ServiceGroup::Ptr sg = static_cast<ServiceGroup::Ptr>(row);

	if (!sg)
		return Empty;

	int num_services = 0;

	for (const Service::Ptr& service : sg->GetMembers()) {
		if (!service->GetLastCheckResult())
			num_services++;
	}

	return num_services;
}

Value ServiceGroupsTable::NumServicesHardOkAccessor(const Value& row)
{
	ServiceGroup::Ptr sg = static_cast<ServiceGroup::Ptr>(row);

	if (!sg)
		return Empty;

	int num_services = 0;

	for (const Service::Ptr& service : sg->GetMembers()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == ServiceOK)
			num_services++;
	}

	return num_services;
}

Value ServiceGroupsTable::NumServicesHardWarnAccessor(const Value& row)
{
	ServiceGroup::Ptr sg = static_cast<ServiceGroup::Ptr>(row);

	if (!sg)
		return Empty;

	int num_services = 0;

	for (const Service::Ptr& service : sg->GetMembers()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == ServiceWarning)
			num_services++;
	}

	return num_services;
}

Value ServiceGroupsTable::NumServicesHardCritAccessor(const Value& row)
{
	ServiceGroup::Ptr sg = static_cast<ServiceGroup::Ptr>(row);

	if (!sg)
		return Empty;

	int num_services = 0;

	for (const Service::Ptr& service : sg->GetMembers()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == ServiceCritical)
			num_services++;
	}

	return num_services;
}

Value ServiceGroupsTable::NumServicesHardUnknownAccessor(const Value& row)
{
	ServiceGroup::Ptr sg = static_cast<ServiceGroup::Ptr>(row);

	if (!sg)
		return Empty;

	int num_services = 0;

	for (const Service::Ptr& service : sg->GetMembers()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == ServiceUnknown)
			num_services++;
	}

	return num_services;
}
