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

#include "livestatus/hostgroupstable.hpp"
#include "icinga/hostgroup.hpp"
#include "icinga/host.hpp"
#include "icinga/service.hpp"
#include "base/configtype.hpp"

using namespace icinga;

HostGroupsTable::HostGroupsTable()
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
	table->AddColumn(prefix + "worst_service_state", Column(&HostGroupsTable::WorstServiceStateAccessor, objectAccessor));
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

String HostGroupsTable::GetName() const
{
	return "hostgroups";
}

String HostGroupsTable::GetPrefix() const
{
	return "hostgroup";
}

void HostGroupsTable::FetchRows(const AddRowFunction& addRowFn)
{
	for (const HostGroup::Ptr& hg : ConfigType::GetObjectsByType<HostGroup>()) {
		if (!addRowFn(hg, LivestatusGroupByNone, Empty))
			return;
	}
}

Value HostGroupsTable::NameAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	return hg->GetName();
}

Value HostGroupsTable::AliasAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	return hg->GetDisplayName();
}

Value HostGroupsTable::NotesAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	return hg->GetNotes();
}

Value HostGroupsTable::NotesUrlAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	return hg->GetNotesUrl();
}

Value HostGroupsTable::ActionUrlAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	return hg->GetActionUrl();
}

Value HostGroupsTable::MembersAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	Array::Ptr members = new Array();

	for (const Host::Ptr& host : hg->GetMembers()) {
		members->Add(host->GetName());
	}

	return members;
}

Value HostGroupsTable::MembersWithStateAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	Array::Ptr members = new Array();

	for (const Host::Ptr& host : hg->GetMembers()) {
		Array::Ptr member_state = new Array();
		member_state->Add(host->GetName());
		member_state->Add(host->GetState());
		members->Add(member_state);
	}

	return members;
}

Value HostGroupsTable::WorstHostStateAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	int worst_host = HostUp;

	for (const Host::Ptr& host : hg->GetMembers()) {
		if (host->GetState() > worst_host)
			worst_host = host->GetState();
	}

	return worst_host;
}

Value HostGroupsTable::NumHostsAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	return hg->GetMembers().size();
}

Value HostGroupsTable::NumHostsPendingAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	int num_hosts = 0;

	for (const Host::Ptr& host : hg->GetMembers()) {
		/* no checkresult */
		if (!host->GetLastCheckResult())
			num_hosts++;
	}

	return num_hosts;
}

Value HostGroupsTable::NumHostsUpAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	int num_hosts = 0;

	for (const Host::Ptr& host : hg->GetMembers()) {
		if (host->GetState() == HostUp)
			num_hosts++;
	}

	return num_hosts;
}

Value HostGroupsTable::NumHostsDownAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	int num_hosts = 0;

	for (const Host::Ptr& host : hg->GetMembers()) {
		if (host->GetState() == HostDown)
			num_hosts++;
	}

	return num_hosts;
}

Value HostGroupsTable::NumHostsUnreachAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	int num_hosts = 0;

	for (const Host::Ptr& host : hg->GetMembers()) {
		if (!host->IsReachable())
			num_hosts++;
	}

	return num_hosts;
}

Value HostGroupsTable::NumServicesAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	int num_services = 0;

	if (hg->GetMembers().size() == 0)
		return 0;

	for (const Host::Ptr& host : hg->GetMembers()) {
		num_services += host->GetServices().size();
	}

	return num_services;
}

Value HostGroupsTable::WorstServiceStateAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	Value worst_service = ServiceOK;

	for (const Host::Ptr& host : hg->GetMembers()) {
		for (const Service::Ptr& service : host->GetServices()) {
			if (service->GetState() > worst_service)
				worst_service = service->GetState();
		}
	}

	return worst_service;
}

Value HostGroupsTable::NumServicesPendingAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	int num_services = 0;

	for (const Host::Ptr& host : hg->GetMembers()) {
		for (const Service::Ptr& service : host->GetServices()) {
			if (!service->GetLastCheckResult())
				num_services++;
		}
	}

	return num_services;
}

Value HostGroupsTable::NumServicesOkAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	int num_services = 0;

	for (const Host::Ptr& host : hg->GetMembers()) {
		for (const Service::Ptr& service : host->GetServices()) {
			if (service->GetState() == ServiceOK)
				num_services++;
		}
	}

	return num_services;
}

Value HostGroupsTable::NumServicesWarnAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	int num_services = 0;

	for (const Host::Ptr& host : hg->GetMembers()) {
		for (const Service::Ptr& service : host->GetServices()) {
			if (service->GetState() == ServiceWarning)
				num_services++;
		}
	}

	return num_services;
}

Value HostGroupsTable::NumServicesCritAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	int num_services = 0;

	for (const Host::Ptr& host : hg->GetMembers()) {
		for (const Service::Ptr& service : host->GetServices()) {
			if (service->GetState() == ServiceCritical)
				num_services++;
		}
	}

	return num_services;
}

Value HostGroupsTable::NumServicesUnknownAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	int num_services = 0;

	for (const Host::Ptr& host : hg->GetMembers()) {
		for (const Service::Ptr& service : host->GetServices()) {
			if (service->GetState() == ServiceUnknown)
				num_services++;
		}
	}

	return num_services;
}

Value HostGroupsTable::WorstServiceHardStateAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	Value worst_service = ServiceOK;

	for (const Host::Ptr& host : hg->GetMembers()) {
		for (const Service::Ptr& service : host->GetServices()) {
			if (service->GetStateType() == StateTypeHard) {
				if (service->GetState() > worst_service)
					worst_service = service->GetState();
			}
		}
	}

	return worst_service;
}

Value HostGroupsTable::NumServicesHardOkAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	int num_services = 0;

	for (const Host::Ptr& host : hg->GetMembers()) {
		for (const Service::Ptr& service : host->GetServices()) {
			if (service->GetStateType() == StateTypeHard && service->GetState() == ServiceOK)
				num_services++;
		}
	}

	return num_services;
}

Value HostGroupsTable::NumServicesHardWarnAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	int num_services = 0;

	for (const Host::Ptr& host : hg->GetMembers()) {
		for (const Service::Ptr& service : host->GetServices()) {
			if (service->GetStateType() == StateTypeHard && service->GetState() == ServiceWarning)
				num_services++;
		}
	}

	return num_services;
}

Value HostGroupsTable::NumServicesHardCritAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	int num_services = 0;

	for (const Host::Ptr& host : hg->GetMembers()) {
		for (const Service::Ptr& service : host->GetServices()) {
			if (service->GetStateType() == StateTypeHard && service->GetState() == ServiceCritical)
				num_services++;
		}
	}

	return num_services;
}

Value HostGroupsTable::NumServicesHardUnknownAccessor(const Value& row)
{
	HostGroup::Ptr hg = static_cast<HostGroup::Ptr>(row);

	if (!hg)
		return Empty;

	int num_services = 0;

	for (const Host::Ptr& host : hg->GetMembers()) {
		for (const Service::Ptr& service : host->GetServices()) {
			if (service->GetStateType() == StateTypeHard && service->GetState() == ServiceUnknown)
				num_services++;
		}
	}

	return num_services;
}
