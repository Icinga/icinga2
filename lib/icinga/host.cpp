/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/host.hpp"
#include "icinga/host.tcpp"
#include "icinga/service.hpp"
#include "icinga/hostgroup.hpp"
#include "icinga/pluginutility.hpp"
#include "icinga/scheduleddowntime.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/debug.hpp"
#include "base/json.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(Host);

void Host::OnAllConfigLoaded(void)
{
	ObjectImpl<Host>::OnAllConfigLoaded();

	HostGroup::EvaluateObjectRules(this);

	Array::Ptr groups = GetGroups();

	if (groups) {
		groups = groups->ShallowClone();

		ObjectLock olock(groups);

		BOOST_FOREACH(const String& name, groups) {
			HostGroup::Ptr hg = HostGroup::GetByName(name);

			if (hg)
				hg->ResolveGroupMembership(this, true);
		}
	}
}

void Host::CreateChildObjects(const Type::Ptr& childType)
{
	if (childType->GetName() == "ScheduledDowntime")
		ScheduledDowntime::EvaluateApplyRules(this);

	if (childType->GetName() == "Notification")
		Notification::EvaluateApplyRules(this);

	if (childType->GetName() == "Dependency")
		Dependency::EvaluateApplyRules(this);

	if (childType->GetName() == "Service")
		Service::EvaluateApplyRules(this);
}

void Host::Stop(void)
{
	ObjectImpl<Host>::Stop();

	Array::Ptr groups = GetGroups();

	if (groups) {
		ObjectLock olock(groups);

		BOOST_FOREACH(const String& name, groups) {
			HostGroup::Ptr hg = HostGroup::GetByName(name);

			if (hg)
				hg->ResolveGroupMembership(this, false);
		}
	}

	// TODO: unregister slave services/notifications?
}

std::set<Service::Ptr> Host::GetServices(void) const
{
	boost::mutex::scoped_lock lock(m_ServicesMutex);

	std::set<Service::Ptr> services;
	typedef std::pair<String, Service::Ptr> ServicePair;
	BOOST_FOREACH(const ServicePair& kv, m_Services) {
		services.insert(kv.second);
	}

	return services;
}

void Host::AddService(const Service::Ptr& service)
{
	boost::mutex::scoped_lock lock(m_ServicesMutex);

	m_Services[service->GetShortName()] = service;
}

void Host::RemoveService(const Service::Ptr& service)
{
	boost::mutex::scoped_lock lock(m_ServicesMutex);

	m_Services.erase(service->GetShortName());
}

int Host::GetTotalServices(void) const
{
	return GetServices().size();
}

Service::Ptr Host::GetServiceByShortName(const Value& name)
{
	if (name.IsScalar()) {
		{
			boost::mutex::scoped_lock lock(m_ServicesMutex);

			std::map<String, Service::Ptr>::const_iterator it = m_Services.find(name);

			if (it != m_Services.end())
				return it->second;
		}

		return Service::Ptr();
	} else if (name.IsObjectType<Dictionary>()) {
		Dictionary::Ptr dict = name;
		String short_name;

		return Service::GetByNamePair(dict->Get("host"), dict->Get("service"));
	} else {
		BOOST_THROW_EXCEPTION(std::invalid_argument("Host/Service name pair is invalid: " + JsonEncode(name)));
	}
}

HostState Host::CalculateState(ServiceState state)
{
	switch (state) {
		case ServiceOK:
		case ServiceWarning:
			return HostUp;
		default:
			return HostDown;
	}
}

HostState Host::GetState(void) const
{
	return CalculateState(GetStateRaw());
}

HostState Host::GetLastState(void) const
{
	return CalculateState(GetLastStateRaw());
}

HostState Host::GetLastHardState(void) const
{
	return CalculateState(GetLastHardStateRaw());
}

double Host::GetLastStateUp(void) const
{
	if (GetLastStateOK() > GetLastStateWarning())
		return GetLastStateOK();
	else
		return GetLastStateWarning();
}

double Host::GetLastStateDown(void) const
{
	return GetLastStateCritical();
}

HostState Host::StateFromString(const String& state)
{
	if (state == "UP")
		return HostUp;
	else
		return HostDown;
}

String Host::StateToString(HostState state)
{
	switch (state) {
		case HostUp:
			return "UP";
		case HostDown:
			return "DOWN";
		default:
			return "INVALID";
	}
}

StateType Host::StateTypeFromString(const String& type)
{
	if (type == "SOFT")
		return StateTypeSoft;
	else
		return StateTypeHard;
}

String Host::StateTypeToString(StateType type)
{
	if (type == StateTypeSoft)
		return "SOFT";
	else
		return "HARD";
}

bool Host::ResolveMacro(const String& macro, const CheckResult::Ptr&, Value *result) const
{
	if (macro == "state") {
		*result = StateToString(GetState());
		return true;
	} else if (macro == "state_id") {
		*result = Convert::ToString(GetState());
		return true;
	} else if (macro == "state_type") {
		*result = StateTypeToString(GetStateType());
		return true;
	} else if (macro == "last_state") {
		*result = StateToString(GetLastState());
		return true;
	} else if (macro == "last_state_id") {
		*result = Convert::ToString(GetLastState());
		return true;
	} else if (macro == "last_state_type") {
		*result = StateTypeToString(GetLastStateType());
		return true;
	} else if (macro == "last_state_change") {
		*result = Convert::ToString((long)GetLastStateChange());
		return true;
	} else if (macro == "downtime_depth") {
		*result = Convert::ToString((long)GetDowntimeDepth());
		return true;
	} else if (macro == "duration_sec") {
		*result = Convert::ToString((long)(Utility::GetTime() - GetLastStateChange()));
		return true;
	} else if (macro == "num_services" || macro == "num_services_ok" || macro == "num_services_warning"
		    || macro == "num_services_unknown" || macro == "num_services_critical") {
			int filter = -1;
			int count = 0;

			if (macro == "num_services_ok")
				filter = ServiceOK;
			else if (macro == "num_services_warning")
				filter = ServiceWarning;
			else if (macro == "num_services_unknown")
				filter = ServiceUnknown;
			else if (macro == "num_services_critical")
				filter = ServiceCritical;

			BOOST_FOREACH(const Service::Ptr& service, GetServices()) {
				if (filter != -1 && service->GetState() != filter)
					continue;

				count++;
			}

			*result = Convert::ToString(count);
			return true;
	}

	CheckResult::Ptr cr = GetLastCheckResult();

	if (cr) {
		if (macro == "latency") {
			*result = Convert::ToString(Service::CalculateLatency(cr));
			return true;
		} else if (macro == "execution_time") {
			*result = Convert::ToString(Service::CalculateExecutionTime(cr));
			return true;
		} else if (macro == "output") {
			*result = cr->GetOutput();
			return true;
		} else if (macro == "perfdata") {
			*result = PluginUtility::FormatPerfdata(cr->GetPerformanceData());
			return true;
		} else if (macro == "last_check") {
			*result = Convert::ToString((long)cr->GetScheduleStart());
			return true;
		} else if (macro == "check_source") {
			*result = cr->GetCheckSource();
			return true;
		}
	}

	return false;
}
