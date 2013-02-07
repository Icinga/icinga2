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

#include "i2-icinga.h"

using namespace icinga;

static AttributeDescription serviceAttributes[] = {
	{ "scheduling_offset", Attribute_Transient },
	{ "first_check", Attribute_Transient },
	{ "next_check", Attribute_Replicated },
	{ "checker", Attribute_Replicated },
	{ "check_attempt", Attribute_Replicated },
	{ "state", Attribute_Replicated },
	{ "state_type", Attribute_Replicated },
	{ "last_result", Attribute_Replicated },
	{ "last_state_change", Attribute_Replicated },
	{ "last_hard_state_change", Attribute_Replicated },
	{ "enable_active_checks", Attribute_Replicated },
	{ "enable_passive_checks", Attribute_Replicated },
	{ "force_next_check", Attribute_Replicated },
	{ "acknowledgement", Attribute_Replicated },
	{ "acknowledgement_expiry", Attribute_Replicated },
	{ "downtimes", Attribute_Replicated },
	{ "comments", Attribute_Replicated }
};

REGISTER_TYPE(Service, serviceAttributes);

const int Service::DefaultMaxCheckAttempts = 3;
const int Service::DefaultCheckInterval = 5 * 60;
const int Service::MinCheckInterval = 5;
const int Service::CheckIntervalDivisor = 5;

boost::signal<void (const Service::Ptr&, const CheckResultMessage&)> Service::OnCheckResultReceived;
boost::signal<void (const Service::Ptr&, const String&)> Service::OnCheckerChanged;
boost::signal<void (const Service::Ptr&, const Value&)> Service::OnNextCheckChanged;

Service::Service(const Dictionary::Ptr& serializedObject)
	: DynamicObject(serializedObject)
{
	ServiceGroup::InvalidateMembersCache();
	Host::InvalidateServicesCache();
	DowntimeProcessor::InvalidateDowntimeCache();
}

Service::~Service(void)
{
	ServiceGroup::InvalidateMembersCache();
	Host::InvalidateServicesCache();
	DowntimeProcessor::InvalidateDowntimeCache();
}

String Service::GetAlias(void) const
{
	String value = Get("alias");

	if (!value.IsEmpty())
		return value;

	return GetName();
}

bool Service::Exists(const String& name)
{
	return (DynamicObject::GetObject("Service", name));
}

Service::Ptr Service::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("Service", name);

	if (!configObject)
		BOOST_THROW_EXCEPTION(invalid_argument("Service '" + name + "' does not exist."));

	return dynamic_pointer_cast<Service>(configObject);
}

Host::Ptr Service::GetHost(void) const
{
	String hostname = Get("host_name");

	if (hostname.IsEmpty())
		BOOST_THROW_EXCEPTION(runtime_error("Service object is missing the 'host_name' property."));

	return Host::GetByName(hostname);
}

Dictionary::Ptr Service::GetMacros(void) const
{
	return Get("macros");
}

Dictionary::Ptr Service::GetDowntimes(void) const
{
	DowntimeProcessor::ValidateDowntimeCache();

	return Get("downtimes");
}

Dictionary::Ptr Service::GetComments(void) const
{
	CommentProcessor::ValidateCommentCache();

	return Get("comments");
}

String Service::GetCheckCommand(void) const
{
	return Get("check_command");
}

long Service::GetMaxCheckAttempts(void) const
{
	Value value = Get("max_check_attempts");

	if (value.IsEmpty())
		return DefaultMaxCheckAttempts;

	return value;
}

long Service::GetCheckInterval(void) const
{
	Value value = Get("check_interval");

	if (value.IsEmpty())
		return DefaultCheckInterval;

	if (value < MinCheckInterval)
		value = MinCheckInterval;

	return value;
}

long Service::GetRetryInterval(void) const
{
	Value value = Get("retry_interval");

	if (value.IsEmpty())
		return GetCheckInterval() / CheckIntervalDivisor;

	return value;
}

Dictionary::Ptr Service::GetHostDependencies(void) const
{
	return Get("hostdependencies");
}

Dictionary::Ptr Service::GetServiceDependencies(void) const
{
	return Get("servicedependencies");
}

Dictionary::Ptr Service::GetGroups(void) const
{
	return Get("servicegroups");
}

Dictionary::Ptr Service::GetCheckers(void) const
{
	return Get("checkers");
}

bool Service::IsReachable(void) const
{
	BOOST_FOREACH(const Service::Ptr& service, GetParentServices()) {
		/* ignore pending services */
		if (!service->GetLastCheckResult())
			continue;

		/* ignore soft states */
		if (service->GetStateType() == StateTypeSoft)
			continue;

		/* ignore services states OK and Warning */
		if (service->GetState() == StateOK ||
		    service->GetState() == StateWarning)
			continue;

		return false;
	}

	BOOST_FOREACH(const Host::Ptr& host, GetParentHosts()) {
		/* ignore hosts that are up */
		if (host->IsUp())
			continue;

		return false;
	}

	return true;
}

bool Service::IsInDowntime(void) const
{
	Dictionary::Ptr downtimes = GetDowntimes();

	if (!downtimes)
		return false;

	Dictionary::Ptr downtime;
	BOOST_FOREACH(tie(tuples::ignore, downtime), downtimes) {
		if (DowntimeProcessor::IsDowntimeActive(downtime))
			return true;
	}

	return false;
}

void Service::SetSchedulingOffset(long offset)
{
	Set("scheduling_offset", offset);
}

long Service::GetSchedulingOffset(void)
{
	Value value = Get("scheduling_offset");

	if (value.IsEmpty()) {
		value = rand();
		SetSchedulingOffset(value);
	}

	return value;
}

void Service::SetFirstCheck(bool first)
{
	Set("first_check", first ? 1 : 0);
}

bool Service::GetFirstCheck(void) const
{
	Value value = Get("first_check");

	if (value.IsEmpty())
		return true;

	return static_cast<long>(value);
}


void Service::SetNextCheck(double nextCheck)
{
	Set("next_check", nextCheck);
}

double Service::GetNextCheck(void)
{
	Value value = Get("next_check");

	if (value.IsEmpty()) {
		UpdateNextCheck();

		value = Get("next_check");

		if (value.IsEmpty())
			BOOST_THROW_EXCEPTION(runtime_error("Failed to schedule next check."));
	}

	return value;
}

void Service::UpdateNextCheck(void)
{
	double interval;

	if (GetStateType() == StateTypeSoft)
		interval = GetRetryInterval();
	else
		interval = GetCheckInterval();

	double now = Utility::GetTime();
	double adj = fmod(now + GetSchedulingOffset(), interval);
	SetNextCheck(now - adj + interval);
}

void Service::SetChecker(const String& checker)
{
	Set("checker", checker);
}

String Service::GetChecker(void) const
{
	return Get("checker");
}

void Service::SetCurrentCheckAttempt(long attempt)
{
	Set("check_attempt", attempt);
}

long Service::GetCurrentCheckAttempt(void) const
{
	Value value = Get("check_attempt");

	if (value.IsEmpty())
		return 1;

	return value;
}

void Service::SetState(ServiceState state)
{
	Set("state", static_cast<long>(state));
}

ServiceState Service::GetState(void) const
{
	Value value = Get("state");

	if (value.IsEmpty())
		return StateUnknown;

	int ivalue = static_cast<int>(value);
	return static_cast<ServiceState>(ivalue);
}

void Service::SetStateType(ServiceStateType type)
{
	Set("state_type", static_cast<long>(type));
}

ServiceStateType Service::GetStateType(void) const
{
	Value value = Get("state_type");

	if (value.IsEmpty())
		return StateTypeSoft;

	int ivalue = static_cast<int>(value);
	return static_cast<ServiceStateType>(ivalue);
}

void Service::SetLastCheckResult(const Dictionary::Ptr& result)
{
	Set("last_result", result);
}

Dictionary::Ptr Service::GetLastCheckResult(void) const
{
	return Get("last_result");
}

void Service::SetLastStateChange(double ts)
{
	Set("last_state_change", static_cast<long>(ts));
}

double Service::GetLastStateChange(void) const
{
	Value value = Get("last_state_change");

	if (value.IsEmpty())
		return IcingaApplication::GetInstance()->GetStartTime();

	return value;
}

void Service::SetLastHardStateChange(double ts)
{
	Set("last_hard_state_change", ts);
}

double Service::GetLastHardStateChange(void) const
{
	Value value = Get("last_hard_state_change");

	if (value.IsEmpty())
		value = IcingaApplication::GetInstance()->GetStartTime();

	return value;
}

bool Service::GetEnableActiveChecks(void) const
{
	Value value = Get("enable_active_checks");

	if (value.IsEmpty())
		return true;

	return static_cast<bool>(value);
}

void Service::SetEnablePassiveChecks(bool enabled)
{
	Set("enable_passive_checks", enabled ? 1 : 0);
}

bool Service::GetEnablePassiveChecks(void) const
{
	Value value = Get("enable_passive_checks");

	if (value.IsEmpty())
		return true;

	return static_cast<bool>(value);
}

void Service::SetEnableActiveChecks(bool enabled)
{
	Set("enable_active_checks", enabled ? 1 : 0);
}

bool Service::GetForceNextCheck(void) const
{
	Value value = Get("force_next_check");

	if (value.IsEmpty())
		return false;

	return static_cast<bool>(value);
}

void Service::SetForceNextCheck(bool forced)
{
	Set("force_next_check", forced ? 1 : 0);
}

AcknowledgementType Service::GetAcknowledgement(void)
{
	Value value = Get("acknowledgement");

	if (value.IsEmpty())
		return AcknowledgementNone;

	int ivalue = static_cast<int>(value);
	AcknowledgementType avalue = static_cast<AcknowledgementType>(ivalue);

	if (avalue != AcknowledgementNone) {
		double expiry = GetAcknowledgementExpiry();

		if (expiry != 0 && expiry < Utility::GetTime()) {
			avalue = AcknowledgementNone;
			SetAcknowledgement(avalue);
			SetAcknowledgementExpiry(0);
		}
	}

	return avalue;
}

void Service::SetAcknowledgement(AcknowledgementType acknowledgement)
{
	Set("acknowledgement", static_cast<long>(acknowledgement));
}

double Service::GetAcknowledgementExpiry(void) const
{
	Value value = Get("acknowledgement_expiry");

	if (value.IsEmpty())
		return 0;

	return static_cast<double>(value);
}

void Service::SetAcknowledgementExpiry(double timestamp)
{
	Set("acknowledgement_expiry", timestamp);
}

void Service::ApplyCheckResult(const Dictionary::Ptr& cr)
{
	ServiceState old_state = GetState();
	ServiceStateType old_stateType = GetStateType();

	long attempt = GetCurrentCheckAttempt();

	if (cr->Get("state") == StateOK) {
		if (GetState() == StateOK)
			SetStateType(StateTypeHard);

		attempt = 1;
	} else {
		if (attempt >= GetMaxCheckAttempts()) {
			SetStateType(StateTypeHard);
			attempt = 1;
		} else if (GetStateType() == StateTypeSoft || GetState() == StateOK) {
			SetStateType(StateTypeSoft);
			attempt++;
		}
	}

	SetCurrentCheckAttempt(attempt);

	int state = cr->Get("state");
	SetState(static_cast<ServiceState>(state));

	SetLastCheckResult(cr);

	if (old_state != GetState()) {
		double now = Utility::GetTime();

		SetLastStateChange(now);

		if (old_stateType != GetStateType())
			SetLastHardStateChange(now);

		/* remove acknowledgements */
		if (GetAcknowledgement() == AcknowledgementNormal ||
		    (GetAcknowledgement() == AcknowledgementSticky && GetStateType() == StateTypeHard && GetState() == StateOK)) {
			SetAcknowledgement(AcknowledgementNone);
			SetAcknowledgementExpiry(0);
		}

		/* reschedule service dependencies */
		BOOST_FOREACH(const Service::Ptr& parent, GetParentServices()) {
			parent->SetNextCheck(Utility::GetTime());
		}

		/* reschedule host dependencies */
		BOOST_FOREACH(const Host::Ptr& parent, GetParentHosts()) {
			String hostcheck = parent->GetHostCheck();
			if (!hostcheck.IsEmpty()) {
				Service::Ptr service = parent->ResolveService(hostcheck);
				service->SetNextCheck(Utility::GetTime());
			}
		}

		// TODO: notify our child services/hosts that our state has changed
	}

	if (GetState() != StateOK)
		DowntimeProcessor::TriggerDowntimes(GetSelf());
}

ServiceState Service::StateFromString(const String& state)
{
	if (state == "ok")
		return StateOK;
	else if (state == "warning")
		return StateWarning;
	else if (state == "critical")
		return StateCritical;
	else if (state == "uncheckable")
		return StateUncheckable;
	else
		return StateUnknown;
}

String Service::StateToString(ServiceState state)
{
	switch (state) {
		case StateOK:
			return "ok";
		case StateWarning:
			return "warning";
		case StateCritical:
			return "critical";
		case StateUncheckable:
			return "uncheckable";
		case StateUnknown:
		default:
			return "unknown";
	}
}

ServiceStateType Service::StateTypeFromString(const String& type)
{
	if (type == "soft")
		return StateTypeSoft;
	else
		return StateTypeHard;
}

String Service::StateTypeToString(ServiceStateType type)
{
	if (type == StateTypeSoft)
		return "soft";
	else
		return "hard";
}

bool Service::IsAllowedChecker(const String& checker) const
{
	Dictionary::Ptr checkers = GetCheckers();

	if (!checkers)
		return true;

	Value pattern;
	BOOST_FOREACH(tie(tuples::ignore, pattern), checkers) {
		if (Utility::Match(pattern, checker))
			return true;
	}

	return false;
}

Dictionary::Ptr Service::ResolveDependencies(const Host::Ptr& host, const Dictionary::Ptr& dependencies)
{
	Dictionary::Ptr services = host->Get("services");

	Dictionary::Ptr result = boost::make_shared<Dictionary>();

	Value dependency;
	BOOST_FOREACH(tie(tuples::ignore, dependency), dependencies) {
		String name;

		if (services && services->Contains(dependency))
			name = host->GetName() + "-" + static_cast<String>(dependency);
		else
			name = static_cast<String>(dependency);

		result->Set(name, name);
	}

	return result;
}

void Service::OnAttributeChanged(const String& name, const Value& oldValue)
{
	if (name == "checker")
		OnCheckerChanged(GetSelf(), oldValue);
	else if (name == "next_check")
		OnNextCheckChanged(GetSelf(), oldValue);
	else if (name == "servicegroups")
		ServiceGroup::InvalidateMembersCache();
	else if (name == "host_name")
		Host::InvalidateServicesCache();
	else if (name == "downtimes")
		DowntimeProcessor::InvalidateDowntimeCache();
}

void Service::BeginExecuteCheck(const function<void (void)>& callback)
{
	/* don't run another check if there is one pending */
	if (!Get("current_task").IsEmpty()) {
		/* we need to call the callback anyway */
		callback();

		return;
	}

	/* keep track of scheduling info in case the check type doesn't provide its own information */
	Dictionary::Ptr scheduleInfo = boost::make_shared<Dictionary>();
	scheduleInfo->Set("schedule_start", GetNextCheck());
	scheduleInfo->Set("execution_start", Utility::GetTime());

	try {
		vector<Value> arguments;
		arguments.push_back(static_cast<Service::Ptr>(GetSelf()));
		ScriptTask::Ptr task;
		task = InvokeMethod("check", arguments, boost::bind(&Service::CheckCompletedHandler, this, scheduleInfo, _1, callback));
		Set("current_task", task);
	} catch (...) {
		/* something went wrong while setting up the method call -
		 * reschedule the service and call the callback anyway. */

		UpdateNextCheck();

		callback();

		throw;
	}
}

void Service::CheckCompletedHandler(const Dictionary::Ptr& scheduleInfo,
    const ScriptTask::Ptr& task, const function<void (void)>& callback)
{
	Set("current_task", Empty);

	scheduleInfo->Set("execution_end", Utility::GetTime());
	scheduleInfo->Set("schedule_end", Utility::GetTime());

	Dictionary::Ptr result;

	try {
		Value vresult = task->GetResult();

		if (vresult.IsObjectType<Dictionary>())
			result = vresult;
	} catch (const exception& ex) {
		stringstream msgbuf;
		msgbuf << "Exception occured during check for service '"
		       << GetName() << "': " << diagnostic_information(ex);
		String message = msgbuf.str();

		Logger::Write(LogWarning, "checker", message);

		result = boost::make_shared<Dictionary>();
		result->Set("state", StateUnknown);
		result->Set("output", message);
	}

	if (result) {
		if (!result->Contains("schedule_start"))
			result->Set("schedule_start", scheduleInfo->Get("schedule_start"));

		if (!result->Contains("schedule_end"))
			result->Set("schedule_end", scheduleInfo->Get("schedule_end"));

		if (!result->Contains("execution_start"))
			result->Set("execution_start", scheduleInfo->Get("execution_start"));

		if (!result->Contains("execution_end"))
			result->Set("execution_end", scheduleInfo->Get("execution_end"));

		if (!result->Contains("active"))
			result->Set("active", 1);

		ProcessCheckResult(result);
	}

	/* figure out when the next check is for this service; the call to
	 * ApplyCheckResult() should've already done this but lets do it again
	 * just in case there was no check result. */
	UpdateNextCheck();

	callback();
}

void Service::ProcessCheckResult(const Dictionary::Ptr& cr)
{
	ApplyCheckResult(cr);

	/* flush the current transaction so other instances see the service's
	 * new state when they receive the CheckResult message */
	DynamicObject::FlushTx();

	RequestMessage rm;
	rm.SetMethod("checker::CheckResult");

	/* TODO: add _old_ state to message */
	CheckResultMessage params;
	params.SetService(GetName());
	params.SetCheckResult(cr);

	rm.SetParams(params);

	EndpointManager::GetInstance()->SendMulticastMessage(rm);
}

set<Host::Ptr> Service::GetParentHosts(void) const
{
	set<Host::Ptr> parents;

	/* The service's host is implicitly a parent. */
	parents.insert(GetHost());

	Dictionary::Ptr dependencies = GetHostDependencies();

	if (dependencies) {
		String key;
		BOOST_FOREACH(tie(key, tuples::ignore), dependencies) {
			parents.insert(Host::GetByName(key));
		}
	}

	return parents;
}

set<Service::Ptr> Service::GetParentServices(void) const
{
	set<Service::Ptr> parents;

	Dictionary::Ptr dependencies = GetServiceDependencies();

	if (dependencies) {
		String key;
		BOOST_FOREACH(tie(key, tuples::ignore), dependencies) {
			Service::Ptr service = GetHost()->ResolveService(key);

			if (service->GetName() == GetName())
				continue;

			parents.insert(service);
		}
	}

	return parents;
}
