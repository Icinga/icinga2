#include "i2-cib.h"

using namespace icinga;

bool ServiceStatusMessage::GetService(string *service) const
{
	return Get("service", service);
}

void ServiceStatusMessage::SetService(const string& service)
{
	Set("service", service);
}

bool ServiceStatusMessage::GetState(ServiceState *state) const
{
	long value;
	if (Get("state", &value)) {
		*state = static_cast<ServiceState>(value);
		return true;
	}
	return false;
}

void ServiceStatusMessage::SetState(ServiceState state)
{
	Set("state", static_cast<long>(state));
}

bool ServiceStatusMessage::GetStateType(ServiceStateType *type) const
{
	long value;
	if (Get("state_type", &value)) {
		*type = static_cast<ServiceStateType>(value);
		return true;
	}
	return false;
}

void ServiceStatusMessage::SetStateType(ServiceStateType type)
{
	Set("state_type", static_cast<long>(type));
}

bool ServiceStatusMessage::GetCurrentCheckAttempt(long *attempt) const
{
	return Get("current_attempt", attempt);
}

void ServiceStatusMessage::SetCurrentCheckAttempt(long attempt)
{
	Set("current_attempt", attempt);
}

bool ServiceStatusMessage::GetNextCheck(time_t *ts) const
{
	long value;
	if (Get("next_check", &value)) {
		*ts = value;
		return true;
	}
	return false;
}

void ServiceStatusMessage::SetNextCheck(time_t ts)
{
	Set("next_check", static_cast<long>(ts));
}

bool ServiceStatusMessage::GetCheckResult(CheckResult *cr) const
{
	Dictionary::Ptr obj;
	if (Get("result", &obj)) {
		*cr = CheckResult(MessagePart(obj));
		return true;
	}
	return false;
}

void ServiceStatusMessage::SetCheckResult(CheckResult cr)
{
	Set("result", cr.GetDictionary());
}
