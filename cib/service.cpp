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
	String value;

	if (GetAttribute("alias", &value))
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
	String hostname;
	if (!GetAttribute("host_name", &hostname))
		throw_exception(runtime_error("Service object is missing the 'host_name' property."));

	return Host::GetByName(hostname);
}

Dictionary::Ptr Service::GetMacros(void) const
{
	Dictionary::Ptr macros;
	GetAttribute("macros", &macros);
	return macros;
}

String Service::GetCheckCommand(void) const
{
	String value;
	GetAttribute("check_command", &value);
	return value;
}

long Service::GetMaxCheckAttempts(void) const
{
	long value = 3;
	GetAttribute("max_check_attempts", &value);
	return value;
}

long Service::GetCheckInterval(void) const
{
	long value = 300;
	GetAttribute("check_interval", &value);

	if (value < 15)
		value = 15;

	return value;
}

long Service::GetRetryInterval(void) const
{
	long value;
	if (!GetAttribute("retry_interval", &value))
		value = GetCheckInterval() / 5;

	return value;
}

Dictionary::Ptr Service::GetDependencies(void) const
{
	Dictionary::Ptr value;
	GetAttribute("dependencies", &value);
	return value;
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
	Dictionary::Ptr value;
	GetAttribute("servicegroups", &value);
	return value;
}

Dictionary::Ptr Service::GetCheckers(void) const
{
	Dictionary::Ptr value;
	GetAttribute("checkers", &value);
	return value;
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
	SetAttribute("scheduling_offset", offset);
}

long Service::GetSchedulingOffset(void)
{
	long value;
	if (!GetAttribute("scheduling_offset", &value)) {
		value = rand();
		SetSchedulingOffset(value);
	}
	return value;
}

void Service::SetNextCheck(double nextCheck)
{
	SetAttribute("next_check", nextCheck);
}

double Service::GetNextCheck(void)
{
	double value;
	if (!GetAttribute("next_check", &value)) {
		UpdateNextCheck();

		if (!GetAttribute("next_check", &value))
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
	SetAttribute("checker", checker);
}

String Service::GetChecker(void) const
{
	String value;
	GetAttribute("checker", &value);
	return value;
}

void Service::SetCurrentCheckAttempt(long attempt)
{
	SetAttribute("check_attempt", attempt);
}

long Service::GetCurrentCheckAttempt(void) const
{
	long value = 1;
	GetAttribute("check_attempt", &value);
	return value;
}

void Service::SetState(ServiceState state)
{
	SetAttribute("state", static_cast<long>(state));
}

ServiceState Service::GetState(void) const
{
	long value = StateUnknown;
	GetAttribute("state", &value);
	return static_cast<ServiceState>(value);
}

void Service::SetStateType(ServiceStateType type)
{
	SetAttribute("state_type", static_cast<long>(type));
}

ServiceStateType Service::GetStateType(void) const
{
	long value = StateTypeHard;
	GetAttribute("state_type", &value);
	return static_cast<ServiceStateType>(value);
}

void Service::SetLastCheckResult(const Dictionary::Ptr& result)
{
	SetAttribute("last_result", result);
}

Dictionary::Ptr Service::GetLastCheckResult(void) const
{
	Dictionary::Ptr value;
	GetAttribute("last_result", &value);
	return value;
}

void Service::SetLastStateChange(double ts)
{
	SetAttribute("last_state_change", static_cast<long>(ts));
}

double Service::GetLastStateChange(void) const
{
	long value;
	if (!GetAttribute("last_state_change", &value))
		value = IcingaApplication::GetInstance()->GetStartTime();
	return value;
}

void Service::SetLastHardStateChange(double ts)
{
	SetAttribute("last_hard_state_change", ts);
}

double Service::GetLastHardStateChange(void) const
{
	double value;
	if (!GetAttribute("last_hard_state_change", &value))
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
	Dictionary::Ptr services;
	host->GetAttribute("services", &services);

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
