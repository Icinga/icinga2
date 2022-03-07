/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/service.hpp"
#include "icinga/service-ti.cpp"
#include "icinga/servicegroup.hpp"
#include "icinga/scheduleddowntime.hpp"
#include "icinga/pluginutility.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"

using namespace icinga;

REGISTER_TYPE(Service);

boost::signals2::signal<void (const Service::Ptr&, const CheckResult::Ptr&, const MessageOrigin::Ptr&)> Service::OnHostProblemChanged;

String ServiceNameComposer::MakeName(const String& shortName, const Object::Ptr& context) const
{
	Service::Ptr service = dynamic_pointer_cast<Service>(context);

	if (!service)
		return "";

	return service->GetHostName() + "!" + shortName;
}

Dictionary::Ptr ServiceNameComposer::ParseName(const String& name) const
{
	std::vector<String> tokens = name.Split("!");

	if (tokens.size() < 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid Service name."));

	return new Dictionary({
		{ "host_name", tokens[0] },
		{ "name", tokens[1] }
	});
}

void Service::OnAllConfigLoaded()
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
			return nullptr;

		return host->GetServiceByShortName(serviceName);
	} else {
		return Service::GetByName(serviceName);
	}
}

Host::Ptr Service::GetHost() const
{
	return m_Host;
}

/* keep in sync with Host::GetSeverity()
 * One could think it may be smart to use an enum and some bitmask math here.
 * But the only thing the consuming icingaweb2 cares about is being able to
 * sort by severity. It is therefore easier to keep them seperated here. */
int Service::GetSeverity() const
{
	int severity;

	ObjectLock olock(this);
	ServiceState state = GetStateRaw();

	if (!HasBeenChecked()) {
		severity = 16;
	} else if (state == ServiceOK) {
		severity = 0;
	} else {
		switch (state) {
			case ServiceWarning:
				severity = 32;
				break;
			case ServiceUnknown:
				severity = 64;
				break;
			case ServiceCritical:
				severity = 128;
				break;
			default:
				severity = 256;
		}

		Host::Ptr host = GetHost();
		ObjectLock hlock (host);
		if (host->GetState() != HostUp) {
			severity += 1024;
		} else {
			if (IsAcknowledged())
				severity += 512;
			else if (IsInDowntime())
				severity += 256;
			else
				severity += 2048;
		}
		hlock.Unlock();
	}

	olock.Unlock();

	return severity;
}

bool Service::GetHandled() const
{
	return Checkable::GetHandled() || (m_Host && m_Host->GetProblem());
}

bool Service::IsStateOK(ServiceState state) const
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
		return std::make_pair(static_pointer_cast<Host>(checkable), nullptr);
}

