#include "i2-cib.h"

using namespace icinga;

bool Service::m_DependencyCacheValid = false;

string Service::GetAlias(void) const
{
	string value;

	if (GetConfigObject()->GetProperty("alias", &value))
		return value;

	return GetName();
}

bool Service::Exists(const string& name)
{
	return (ConfigObject::GetObject("service", name));
}

Service Service::GetByName(const string& name)
{
	ConfigObject::Ptr configObject = ConfigObject::GetObject("service", name);

	if (!configObject)
		throw invalid_argument("Service '" + name + "' does not exist.");

	return configObject;
}

Host Service::GetHost(void) const
{
	string hostname;
	if (!GetConfigObject()->GetProperty("host_name", &hostname))
		throw runtime_error("Service object is missing the 'host_name' property.");

	return Host::GetByName(hostname);
}

Dictionary::Ptr Service::GetMacros(void) const
{
	Dictionary::Ptr macros;
	GetConfigObject()->GetProperty("macros", &macros);
	return macros;
}

string Service::GetCheckType(void) const
{
	string value = "nagios";
	GetConfigObject()->GetProperty("check_type", &value);
	return value;
}

string Service::GetCheckCommand(void) const
{
	string value;
	GetConfigObject()->GetProperty("check_command", &value);
	return value;
}

long Service::GetMaxCheckAttempts(void) const
{
	long value = 3;
	GetConfigObject()->GetProperty("max_check_attempts", &value);
	return value;
}

long Service::GetCheckInterval(void) const
{
	long value = 300;
	GetConfigObject()->GetProperty("check_interval", &value);

	if (value < 15)
		value = 15;

	return value;
}

long Service::GetRetryInterval(void) const
{
	long value;
	if (!GetConfigObject()->GetProperty("retry_interval", &value))
		value = GetCheckInterval() / 5;

	return value;
}

Dictionary::Ptr Service::GetDependencies(void) const
{
	Dictionary::Ptr value;
	GetConfigObject()->GetProperty("dependencies", &value);
	return value;
}

Dictionary::Ptr Service::GetGroups(void) const
{
	Dictionary::Ptr value;
	GetConfigObject()->GetProperty("servicegroups", &value);
	return value;
}

Dictionary::Ptr Service::GetCheckers(void) const
{
	Dictionary::Ptr value;
	GetConfigObject()->GetProperty("checkers", &value);
	return value;
}

vector<Service> Service::GetParents(void) const
{
	vector<Service> parents;

	Dictionary::Ptr dependencies = GetDependencies();
	if (dependencies) {
		Dictionary::Iterator it;
		for (it = dependencies->Begin(); it != dependencies->End(); it++)
			parents.push_back(Service::GetByName(it->second));
	}
	return parents;
}

vector<Service> Service::GetChildren(void) const
{
	vector<Service> children;

	UpdateDependencyCache();

	Dictionary::Ptr childrenCache;
	GetConfigObject()->GetTag("dependency_children", &childrenCache);

	if (childrenCache) {
		Dictionary::Iterator it;
		for (it = childrenCache->Begin(); it != childrenCache->End(); it++)
			children.push_back(Service::GetByName(it->second));
	}

	return children;
}

void Service::UpdateDependencyCache(void)
{
	static long cacheTx = 0;

	if (m_DependencyCacheValid)
		return;

	cacheTx++;

	ConfigObject::TMap::Range range = ConfigObject::GetObjects("service");
	ConfigObject::TMap::Iterator it;
	for (it = range.first; it != range.second; it++) {
		Service child = it->second;

		vector<Service> parents = child.GetParents();

		vector<Service>::iterator st;
		for (st = parents.begin(); st != parents.end(); st++) {
			Service parent = *st;

			long tx = 0;
			parent.GetConfigObject()->GetTag("dependency_cache_tx", &tx);

			Dictionary::Ptr children;

			/* rather than resetting the dependency dictionary in a separate loop we use the cache_tx
			 * tag to check if the dictionary is from this cache update run. */
			if (tx != cacheTx) {
				children = boost::make_shared<Dictionary>();
				parent.GetConfigObject()->SetTag("dependency_children", children);
				parent.GetConfigObject()->SetTag("dependency_cache_tx", cacheTx);
			} else {
				parent.GetConfigObject()->GetTag("dependency_children", &children);
				assert(children);
			}

			children->AddUnnamedProperty(child.GetName());
		}
	}

	m_DependencyCacheValid = true;
}

void Service::InvalidateDependencyCache(void)
{
	m_DependencyCacheValid = false;
}

ServiceStatusMessage Service::CalculateCombinedStatus(Service *current, ServiceStatusMessage *input, const vector<Service>& parents)
{
	vector<Service> failedServices;

	time_t nextCheck = -1;

	vector<Service>::const_iterator it;
	for (it = parents.begin(); it != parents.end(); it++) {
		Service parent = *it;

		if (current && current->GetName() == parent.GetName())
			continue;

		if (!parent.HasLastCheckResult())
			continue;

		string svcname;
		ServiceState state = StateUnknown;
		ServiceStateType type = StateTypeHard;
		if (input && input->GetService(&svcname) && svcname == parent.GetName()) {
			input->GetState(&state);
			input->GetStateType(&type);
		} else {
			state = parent.GetState();
			type = parent.GetStateType();
		}

		if (state != StateOK && state != StateWarning && type == StateTypeHard)
			failedServices.push_back(parent);

		if (nextCheck == -1 || parent.GetNextCheck() < nextCheck)
			nextCheck = parent.GetNextCheck();
	}

	string message;
	ServiceState state;

	if (failedServices.empty()) {
		if (input)
			return *input;

		state = StateOK;
		message = "Dependant services are available.";
	} else {
		state = StateUnreachable;
		message = "One or more dependant services have failed.";
	}

	ServiceStatusMessage result;
	result.SetState(state);
	result.SetStateType(StateTypeHard);
	result.SetCurrentCheckAttempt(1);
	result.SetNextCheck(nextCheck);

	time_t now;
	time(&now);

	CheckResult cr;
	cr.SetScheduleStart(now);
	cr.SetScheduleEnd(now);
	cr.SetExecutionStart(now);
	cr.SetExecutionEnd(now);
	cr.SetOutput(message);
	cr.SetState(state);

	result.SetCheckResult(cr);

	return result;
}

void Service::SetNextCheck(time_t nextCheck)
{
	GetConfigObject()->SetTag("next_check", (long)nextCheck);
}

time_t Service::GetNextCheck(void)
{
	long value;
	if (!GetConfigObject()->GetTag("next_check", &value)) {
		value = time(NULL) + rand() % GetCheckInterval();
		SetNextCheck(value);
	}
	return value;
}

void Service::UpdateNextCheck(void)
{
	time_t now;
	time(&now);

	if (GetStateType() == StateTypeSoft)
		SetNextCheck(now + GetRetryInterval());
	else
		SetNextCheck(now + GetCheckInterval());
}

void Service::SetChecker(const string& checker)
{
	GetConfigObject()->SetTag("checker", checker);
}

string Service::GetChecker(void) const
{
	string value;
	GetConfigObject()->GetTag("checker", &value);
	return value;
}

void Service::SetCurrentCheckAttempt(long attempt)
{
	GetConfigObject()->SetTag("check_attempt", attempt);
}

long Service::GetCurrentCheckAttempt(void) const
{
	long value = 1;
	GetConfigObject()->GetTag("check_attempt", &value);
	return value;
}

void Service::SetState(ServiceState state)
{
	GetConfigObject()->SetTag("state", static_cast<long>(state));
}

ServiceState Service::GetState(void) const
{
	long value = StateUnknown;
	GetConfigObject()->GetTag("state", &value);
	return static_cast<ServiceState>(value);
}

void Service::SetStateType(ServiceStateType type)
{
	GetConfigObject()->SetTag("state_type", static_cast<long>(type));
}

ServiceStateType Service::GetStateType(void) const
{
	long value = StateTypeHard;
	GetConfigObject()->GetTag("state_type", &value);
	return static_cast<ServiceStateType>(value);
}

void Service::SetLastCheckResult(const CheckResult& result)
{
	GetConfigObject()->SetTag("last_result", result.GetDictionary());
}

bool Service::HasLastCheckResult(void) const
{
	Dictionary::Ptr value;
	return GetConfigObject()->GetTag("last_result", &value) && value;
}

CheckResult Service::GetLastCheckResult(void) const
{
	Dictionary::Ptr value;
	if (!GetConfigObject()->GetTag("last_result", &value))
		throw invalid_argument("Service has no last check result.");
	return CheckResult(value);
}

void Service::SetLastStateChange(time_t ts)
{
	GetConfigObject()->SetTag("last_state_change", static_cast<long>(ts));
}

time_t Service::GetLastStateChange(void) const
{
	long value;
	if (!GetConfigObject()->GetTag("last_state_change", &value))
		value = IcingaApplication::GetInstance()->GetStartTime();
	return value;
}

void Service::SetLastHardStateChange(time_t ts)
{
	GetConfigObject()->SetTag("last_hard_state_change", static_cast<long>(ts));
}

time_t Service::GetLastHardStateChange(void) const
{
	long value;
	if (!GetConfigObject()->GetTag("last_hard_state_change", &value))
		value = IcingaApplication::GetInstance()->GetStartTime();
	return value;
}

void Service::ApplyCheckResult(const CheckResult& cr)
{
	long attempt = GetCurrentCheckAttempt();

	if (cr.GetState() == StateOK) {
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
	SetState(cr.GetState());
}

ServiceState Service::StateFromString(const string& state)
{
	/* TODO: make this thread-safe */
	static map<string, ServiceState> stateLookup;

	if (stateLookup.empty()) {
		stateLookup["ok"] = StateOK;
		stateLookup["warning"] = StateWarning;
		stateLookup["critical"] = StateCritical;
		stateLookup["unreachable"] = StateUnreachable;
		stateLookup["uncheckable"] = StateUncheckable;
		stateLookup["unknown"] = StateUnknown;
	}

	map<string, ServiceState>::iterator it;
	it = stateLookup.find(state);

	if (it == stateLookup.end())
		return StateUnknown;
	else
		return it->second;
}

string Service::StateToString(ServiceState state)
{
	switch (state) {
		case StateOK:
			return "ok";
		case StateWarning:
			return "warning";
		case StateCritical:
			return "critical";
		case StateUnreachable:
			return "unreachable";
		case StateUncheckable:
			return "uncheckable";
		case StateUnknown:
		default:
			return "unknown";
	}
}

ServiceStateType Service::StateTypeFromString(const string& type)
{
	if (type == "soft")
		return StateTypeSoft;
	else
		return StateTypeHard;
}

string Service::StateTypeToString(ServiceStateType type)
{
	if (type == StateTypeSoft)
		return "soft";
	else
		return "hard";
}

bool Service::IsAllowedChecker(const string& checker) const
{
	Dictionary::Ptr checkers = GetCheckers();

	if (!checkers)
		return true;

	Dictionary::Iterator it;
	for (it = checkers->Begin(); it != checkers->End(); it++) {
		string pattern = it->second;

		if (Utility::Match(pattern, checker))
			return true;
	}

	return false;
}
