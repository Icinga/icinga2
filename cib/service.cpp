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

#include "i2-cib.h"

using namespace icinga;

REGISTER_CLASS(Service);

boost::signal<void (const Service::Ptr&, const CheckResultMessage&)> Service::OnCheckResultReceived;
boost::signal<void (const Service::Ptr&, const String&)> Service::OnCheckerChanged;

Service::Service(const Dictionary::Ptr& serializedObject)
	: DynamicObject(serializedObject)
{
	RegisterAttribute("alias", Attribute_Config);
	RegisterAttribute("host_name", Attribute_Config);
	RegisterAttribute("macros", Attribute_Config);
	RegisterAttribute("check_command", Attribute_Config);
	RegisterAttribute("max_check_attempts", Attribute_Config);
	RegisterAttribute("check_interval", Attribute_Config);
	RegisterAttribute("retry_interval", Attribute_Config);
	RegisterAttribute("dependencies", Attribute_Config);
	RegisterAttribute("servicegroups", Attribute_Config);
	RegisterAttribute("checkers", Attribute_Config);

	RegisterAttribute("scheduling_offset", Attribute_Transient);
	RegisterAttribute("next_check", Attribute_Replicated);
	RegisterAttribute("checker", Attribute_Replicated);
	RegisterAttribute("check_attempt", Attribute_Replicated);
	RegisterAttribute("state", Attribute_Replicated);
	RegisterAttribute("state_type", Attribute_Replicated);
	RegisterAttribute("last_result", Attribute_Replicated);
	RegisterAttribute("last_state_change", Attribute_Replicated);
	RegisterAttribute("last_hard_state_change", Attribute_Replicated);
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
		throw_exception(invalid_argument("Service '" + name + "' does not exist."));

	return dynamic_pointer_cast<Service>(configObject);
}

Host::Ptr Service::GetHost(void) const
{
	String hostname = Get("host_name");

	if (hostname.IsEmpty())
		throw_exception(runtime_error("Service object is missing the 'host_name' property."));

	return Host::GetByName(hostname);
}

Dictionary::Ptr Service::GetMacros(void) const
{
	return Get("macros");
}

String Service::GetCheckCommand(void) const
{
	return Get("check_command");
}

long Service::GetMaxCheckAttempts(void) const
{
	Value value = Get("max_check_attempts");

	if (value.IsEmpty())
		return 3;

	return value;
}

long Service::GetCheckInterval(void) const
{
	Value value = Get("check_interval");

	if (value.IsEmpty())
		return 300;

	if (value < 15)
		value = 15;

	return value;
}

long Service::GetRetryInterval(void) const
{
	Value value = Get("retry_interval");

	if (value.IsEmpty())
		return GetCheckInterval() / 5;

	return value;
}

Dictionary::Ptr Service::GetDependencies(void) const
{
	return Get("dependencies");
}

void Service::GetDependenciesRecursive(const Dictionary::Ptr& result) const {
	assert(result);

	Dictionary::Ptr dependencies = GetDependencies();

	if (!dependencies)
		return;

	Value dependency;
	BOOST_FOREACH(tie(tuples::ignore, dependency), dependencies) {
		if (result->Contains(dependency))
			continue;

		result->Set(dependency, dependency);

		Service::Ptr service = Service::GetByName(dependency);
		service->GetDependenciesRecursive(result);
	}
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
	Dictionary::Ptr dependencies = boost::make_shared<Dictionary>();
	GetDependenciesRecursive(dependencies);

	Value dependency;
	BOOST_FOREACH(tie(tuples::ignore, dependency), dependencies) {
		Service::Ptr service = Service::GetByName(dependency);

		/* ignore ourselves */
		if (service->GetName() == GetName())
			continue;

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

	return true;
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
			throw_exception(runtime_error("Failed to schedule next check."));
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
		return StateTypeHard;

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
	}
}

ServiceState Service::StateFromString(const String& state)
{
	/* TODO: make this thread-safe */
	static map<String, ServiceState> stateLookup;

	if (stateLookup.empty()) {
		stateLookup["ok"] = StateOK;
		stateLookup["warning"] = StateWarning;
		stateLookup["critical"] = StateCritical;
		stateLookup["uncheckable"] = StateUncheckable;
		stateLookup["unknown"] = StateUnknown;
	}

	map<String, ServiceState>::iterator it;
	it = stateLookup.find(state);

	if (it == stateLookup.end())
		return StateUnknown;
	else
		return it->second;
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
}
