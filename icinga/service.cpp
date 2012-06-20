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
	long value = 1;
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
	long value = 15;
	GetConfigObject()->GetProperty("retry_interval", &value);
	return value;
}

void Service::SetNextCheck(time_t nextCheck)
{
	GetConfigObject()->SetProperty("next_check", nextCheck);
}

time_t Service::GetNextCheck(void)
{
	long value = -1;
	GetConfigObject()->GetProperty("next_check", &value);
	if (value == -1) {
		value = time(NULL) + rand() % GetCheckInterval();
		SetNextCheck(value);
	}
	return value;
}

void Service::SetChecker(string checker)
{
	GetConfigObject()->SetProperty("checker", checker);
}

string Service::GetChecker(void) const
{
	string value;
	GetConfigObject()->GetProperty("checker", &value);
	return value;
}

