#include "i2-cib.h"

using namespace icinga;

bool ServiceStatusMessage::GetService(string *service) const
{
	return GetProperty("service", service);
}

void ServiceStatusMessage::SetService(const string& service)
{
	SetProperty("service", service);
}

bool ServiceStatusMessage::GetState(ServiceState *state) const
{
	long value;
	if (GetProperty("state", &value)) {
		*state = static_cast<ServiceState>(value);
		return true;
	}
	return false;
}

void ServiceStatusMessage::SetState(ServiceState state)
{
	SetProperty("state", static_cast<long>(state));
}

bool ServiceStatusMessage::GetStateType(ServiceStateType *type) const
{
	long value;
	if (GetProperty("state_type", &value)) {
		*type = static_cast<ServiceStateType>(value);
		return true;
	}
	return false;
}

void ServiceStatusMessage::SetStateType(ServiceStateType type)
{
	SetProperty("state_type", static_cast<long>(type));
}

bool ServiceStatusMessage::GetCurrentCheckAttempt(long *attempt) const
{
	return GetProperty("current_attempt", attempt);
}

void ServiceStatusMessage::SetCurrentCheckAttempt(long attempt)
{
	SetProperty("current_attempt", attempt);
}

bool ServiceStatusMessage::GetNextCheck(time_t *ts) const
{
	long value;
	if (GetProperty("next_check", &value)) {
		*ts = value;
		return true;
	}
	return false;
}

void ServiceStatusMessage::SetNextCheck(time_t ts)
{
	SetProperty("next_check", static_cast<long>(ts));
}

bool ServiceStatusMessage::GetCheckResult(CheckResult *cr) const
{
	Dictionary::Ptr obj;
	if (GetProperty("result", &obj)) {
		*cr = CheckResult(MessagePart(obj));
		return true;
	}
	return false;
}

void ServiceStatusMessage::SetCheckResult(CheckResult cr)
{
	SetProperty("result", cr.GetDictionary());
}