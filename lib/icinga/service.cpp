/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "icinga/service.hpp"
#include "icinga/service.tcpp"
#include "icinga/servicegroup.hpp"
#include "icinga/scheduleddowntime.hpp"
#include "icinga/pluginutility.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace icinga;

REGISTER_TYPE(Service);

String ServiceNameComposer::MakeName(const String& shortName, const Object::Ptr& context) const
{
	Service::Ptr service = dynamic_pointer_cast<Service>(context);

	if (!service)
		return "";

	return service->GetHostName() + "!" + shortName;
}

Dictionary::Ptr ServiceNameComposer::ParseName(const String& name) const
{
	std::vector<String> tokens;
	boost::algorithm::split(tokens, name, boost::is_any_of("!"));

	if (tokens.size() < 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid Service name."));

	Dictionary::Ptr result = new Dictionary();
	result->Set("host_name", tokens[0]);
	result->Set("name", tokens[1]);

	return result;
}

void Service::OnAllConfigLoaded(void)
{
	ObjectImpl<Service>::OnAllConfigLoaded();

	String zoneName = GetZoneName();

	if (!zoneName.IsEmpty()) {
		Zone::Ptr zone = Zone::GetByName(zoneName);

		if (zone && zone->IsGlobal())
			BOOST_THROW_EXCEPTION(std::invalid_argument("Service '" + GetName() + "' cannot be put into global zone '" + zone->GetName() + "'."));
	}

	m_Host = Host::GetByName(GetHostName());

	if (m_Host)
		m_Host->AddService(this);

	ServiceGroup::EvaluateObjectRules(this);

	Array::Ptr groups = GetGroups();

	if (groups) {
		groups = groups->ShallowClone();

		ObjectLock olock(groups);

		for (const String& name : groups) {
			ServiceGroup::Ptr sg = ServiceGroup::GetByName(name);

			if (sg)
				sg->ResolveGroupMembership(this, true);
		}
	}
}

void Service::CreateChildObjects(const Type::Ptr& childType)
{
	if (childType == ScheduledDowntime::TypeInstance)
		ScheduledDowntime::EvaluateApplyRules(this);

	if (childType == Notification::TypeInstance)
		Notification::EvaluateApplyRules(this);

	if (childType == Dependency::TypeInstance)
		Dependency::EvaluateApplyRules(this);
}

Service::Ptr Service::GetByNamePair(const String& hostName, const String& serviceName)
{
	if (!hostName.IsEmpty()) {
		Host::Ptr host = Host::GetByName(hostName);

		if (!host)
			return Service::Ptr();

		return host->GetServiceByShortName(serviceName);
	} else {
		return Service::GetByName(serviceName);
	}
}

Host::Ptr Service::GetHost(void) const
{
	return m_Host;
}

/* keep in sync with Host::GetSeverity() */
int Service::GetSeverity(void) const
{
	int severity = 0;

	ObjectLock olock(this);
	ServiceState state = GetStateRaw();

	if (!HasBeenChecked())
		severity |= SeverityFlagPending;
	else if (state == ServiceWarning)
		severity |= SeverityFlagWarning;
	else if (state == ServiceUnknown)
		severity |= SeverityFlagUnknown;
	else if (state == ServiceCritical)
		severity |= SeverityFlagCritical;

	/* TODO: Add host reachability and handled */
	if (IsInDowntime())
		severity |= SeverityFlagDowntime;
	else if (IsAcknowledged())
		severity |= SeverityFlagAcknowledgement;
	else
		severity |= SeverityFlagUnhandled;

	olock.Unlock();

	return severity;
}

bool Service::IsStateOK(ServiceState state)
{
	return state == ServiceOK;
}

void Service::SaveLastState(ServiceState state, double timestamp)
{
	if (state == ServiceOK)
		SetLastStateOK(timestamp);
	else if (state == ServiceWarning)
		SetLastStateWarning(timestamp);
	else if (state == ServiceCritical)
		SetLastStateCritical(timestamp);
	else if (state == ServiceUnknown)
		SetLastStateUnknown(timestamp);
}

ServiceState Service::StateFromString(const String& state)
{
	if (state == "OK")
		return ServiceOK;
	else if (state == "WARNING")
		return ServiceWarning;
	else if (state == "CRITICAL")
		return ServiceCritical;
	else
		return ServiceUnknown;
}

String Service::StateToString(ServiceState state)
{
	switch (state) {
		case ServiceOK:
			return "OK";
		case ServiceWarning:
			return "WARNING";
		case ServiceCritical:
			return "CRITICAL";
		case ServiceUnknown:
		default:
			return "UNKNOWN";
	}
}

StateType Service::StateTypeFromString(const String& type)
{
	if (type == "SOFT")
		return StateTypeSoft;
	else
		return StateTypeHard;
}

String Service::StateTypeToString(StateType type)
{
	if (type == StateTypeSoft)
		return "SOFT";
	else
		return "HARD";
}

bool Service::ResolveMacro(const String& macro, const CheckResult::Ptr& cr, Value *result) const
{
	if (macro == "state") {
		*result = StateToString(GetState());
		return true;
	} else if (macro == "state_id") {
		*result = GetState();
		return true;
	} else if (macro == "state_type") {
		*result = StateTypeToString(GetStateType());
		return true;
	} else if (macro == "last_state") {
		*result = StateToString(GetLastState());
		return true;
	} else if (macro == "last_state_id") {
		*result = GetLastState();
		return true;
	} else if (macro == "last_state_type") {
		*result = StateTypeToString(GetLastStateType());
		return true;
	} else if (macro == "last_state_change") {
		*result = static_cast<long>(GetLastStateChange());
		return true;
	} else if (macro == "downtime_depth") {
		*result = GetDowntimeDepth();
		return true;
	} else if (macro == "duration_sec") {
		*result = Utility::GetTime() - GetLastStateChange();
		return true;
	}

	if (cr) {
		if (macro == "latency") {
			*result = cr->CalculateLatency();
			return true;
		} else if (macro == "execution_time") {
			*result = cr->CalculateExecutionTime();
			return true;
		} else if (macro == "output") {
			*result = cr->GetOutput();
			return true;
		} else if (macro == "perfdata") {
			*result = PluginUtility::FormatPerfdata(cr->GetPerformanceData());
			return true;
		} else if (macro == "check_source") {
			*result = cr->GetCheckSource();
			return true;
		}
	}

	return false;
}

std::pair<Host::Ptr, Service::Ptr> icinga::GetHostService(const Checkable::Ptr& checkable)
{
	Service::Ptr service = dynamic_pointer_cast<Service>(checkable);

	if (service)
		return std::make_pair(service->GetHost(), service);
	else
		return std::make_pair(static_pointer_cast<Host>(checkable), Service::Ptr());
}

