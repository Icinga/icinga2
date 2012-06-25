#include "i2-icinga.h"

using namespace icinga;

string Service::GetDisplayName(void) const
{
	string value;

	if (GetConfigObject()->GetProperty("displayname", &value))
		return value;

	return GetName();
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
	long value = 60;
	GetConfigObject()->GetProperty("retry_interval", &value);
	return value;
}

void Service::SetNextCheck(time_t nextCheck)
{
	GetConfigObject()->SetTag("next_check", (long)nextCheck);
}

time_t Service::GetNextCheck(void)
{
	long value = -1;
	GetConfigObject()->GetTag("next_check", &value);
	if (value == -1) {
		value = time(NULL) + rand() % GetCheckInterval();
		SetNextCheck(value);
	}
	return value;
}

void Service::SetChecker(string checker)
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
	long value = 0;
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

void Service::ApplyCheckResult(const CheckResult& cr)
{
	long attempt = GetCurrentCheckAttempt();

	if (GetState() == StateOK && cr.GetState() == StateOK) {
		SetStateType(StateTypeHard);
		SetCurrentCheckAttempt(0);
	} else if (GetState() == StateOK && cr.GetState() != StateOK) {
		attempt++;

		if (attempt >= GetMaxCheckAttempts()) {
			SetStateType(StateTypeHard);
			attempt = 0;
		} else {
			SetStateType(StateTypeSoft);
		}

		SetCurrentCheckAttempt(attempt);
	} else if (GetState() != StateOK && cr.GetState() == StateOK) {
		SetState(StateOK);
	}

	SetState(cr.GetState());
}

