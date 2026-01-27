// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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

	ArrayData result;

	for (const Service::Ptr& service : sg->GetMembers()) {
		result.push_back(new Array({
			service->GetHost()->GetName(),
			service->GetShortName()
		}));
	}

	return new Array(std::move(result));
}

Value ServiceGroupsTable::MembersWithStateAccessor(const Value& row)
{
	ServiceGroup::Ptr sg = static_cast<ServiceGroup::Ptr>(row);

	if (!sg)
		return Empty;

	ArrayData result;

	for (const Service::Ptr& service : sg->GetMembers()) {
		result.push_back(new Array({
			service->GetHost()->GetName(),
			service->GetShortName(),
			service->GetHost()->GetState(),
			service->GetState()
		}));
	}

	return new Array(std::move(result));
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
