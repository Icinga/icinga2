/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/host.hpp"
#include "icinga/host-ti.cpp"
#include "icinga/service.hpp"
#include "icinga/hostgroup.hpp"
#include "icinga/pluginutility.hpp"
#include "icinga/scheduleddowntime.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/debug.hpp"
#include "base/json.hpp"

using namespace icinga;

REGISTER_TYPE(Host);

void Host::OnAllConfigLoaded()
{
	ObjectImpl<Host>::OnAllConfigLoaded();

	String zoneName = GetZoneName();

	if (!zoneName.IsEmpty()) {
		Zone::Ptr zone = Zone::GetByName(zoneName);

		if (zone && zone->IsGlobal())
			BOOST_THROW_EXCEPTION(std::invalid_argument("Host '" + GetName() + "' cannot be put into global zone '" + zone->GetName() + "'."));
	}

	HostGroup::EvaluateObjectRules(this);

	Array::Ptr groups = GetGroups();

	if (groups) {
		groups = groups->ShallowClone();

		ObjectLock olock(groups);

		for (const String& name : groups) {
			HostGroup::Ptr hg = HostGroup::GetByName(name);

			if (hg)
				hg->ResolveGroupMembership(this, true);
		}
	}
}

void Host::CreateChildObjects(const Type::Ptr& childType)
{
	if (childType == ScheduledDowntime::TypeInstance)
		ScheduledDowntime::EvaluateApplyRules(this);

	if (childType == Notification::TypeInstance)
		Notification::EvaluateApplyRules(this);

	if (childType == Dependency::TypeInstance)
		Dependency::EvaluateApplyRules(this);

	if (childType == Service::TypeInstance)
		Service::EvaluateApplyRules(this);
}

void Host::Stop(bool runtimeRemoved)
{
	ObjectImpl<Host>::Stop(runtimeRemoved);

	Array::Ptr groups = GetGroups();

	if (groups) {
		ObjectLock olock(groups);

		for (const String& name : groups) {
			HostGroup::Ptr hg = HostGroup::GetByName(name);

			if (hg)
				hg->ResolveGroupMembership(this, false);
		}
	}

	// TODO: unregister slave services/notifications?
}

std::vector<Service::Ptr> Host::GetServices() const
{
	std::unique_lock<std::mutex> lock(m_ServicesMutex);

	std::vector<Service::Ptr> services;
	services.reserve(m_Services.size());
	typedef std::pair<String, Service::Ptr> ServicePair;
	for (const ServicePair& kv : m_Services) {
		services.push_back(kv.second);
	}

	return services;
}

void Host::AddService(const Service::Ptr& service)
{
	std::unique_lock<std::mutex> lock(m_ServicesMutex);

	m_Services[service->GetShortName()] = service;
}

void Host::RemoveService(const Service::Ptr& service)
{
	std::unique_lock<std::mutex> lock(m_ServicesMutex);

	m_Services.erase(service->GetShortName());
}

int Host::GetTotalServices() const
{
	return GetServices().size();
}

Service::Ptr Host::GetServiceByShortName(const Value& name)
{
	if (name.IsScalar()) {
		{
			std::unique_lock<std::mutex> lock(m_ServicesMutex);

			auto it = m_Services.find(name);

			if (it != m_Services.end())
				return it->second;
		}

		return nullptr;
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

HostState Host::GetState() const
{
	return CalculateState(GetStateRaw());
}

HostState Host::GetLastState() const
{
	return CalculateState(GetLastStateRaw());
}

HostState Host::GetLastHardState() const
{
	return CalculateState(GetLastHardStateRaw());
}

/* keep in sync with Service::GetSeverity()
 * One could think it may be smart to use an enum and some bitmask math here.
 * But the only thing the consuming icingaweb2 cares about is being able to
 * sort by severity. It is therefore easier to keep them seperated here. */
int Host::GetSeverity() const
{
	int severity = 0;

	ObjectLock olock(this);
	HostState state = GetState();

	if (!HasBeenChecked()) {
		severity = 16;
	} else if (state == HostUp) {
		severity = 0;
	} else {
		if (IsReachable())
			severity = 64;
		else
			severity = 32;

		if (IsAcknowledged())
			severity += 512;
		else if (IsInDowntime())
			severity += 256;
		else
			severity += 2048;
	}

	olock.Unlock();

	return severity;

}

bool Host::IsStateOK(ServiceState state) const
{
	return Host::CalculateState(state) == HostUp;
}

void Host::SaveLastState(ServiceState state, double timestamp)
{
	if (state == ServiceOK || state == ServiceWarning)
		SetLastStateUp(timestamp);
	else if (state == ServiceCritical)
		SetLastStateDown(timestamp);
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

			for (const Service::Ptr& service : GetServices()) {
				if (filter != -1 && service->GetState() != filter)
					continue;

				count++;
			}

			*result = count;
			return true;
	}

	CheckResult::Ptr cr = GetLastCheckResult();

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
