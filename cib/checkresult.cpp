#include "i2-cib.h"

using namespace icinga;

CheckResult::CheckResult(void)
	: MessagePart()
{ }

CheckResult::CheckResult(const MessagePart& message)
	: MessagePart(message)
{ }

void CheckResult::SetScheduleStart(time_t ts)
{
	Set("schedule_start", static_cast<long>(ts));
}

time_t CheckResult::GetScheduleStart(void) const
{
	long value = 0;
	Get("schedule_start", &value);
	return static_cast<time_t>(value);
}

void CheckResult::SetScheduleEnd(time_t ts)
{
	Set("schedule_end", static_cast<long>(ts));
}

time_t CheckResult::GetScheduleEnd(void) const
{
	long value = 0;
	Get("schedule_end", &value);
	return static_cast<time_t>(value);
}

void CheckResult::SetExecutionStart(time_t ts)
{
	Set("execution_start", static_cast<long>(ts));
}

time_t CheckResult::GetExecutionStart(void) const
{
	long value = 0;
	Get("execution_start", &value);
	return static_cast<time_t>(value);
}

void CheckResult::SetExecutionEnd(time_t ts)
{
	Set("execution_end", static_cast<long>(ts));
}

time_t CheckResult::GetExecutionEnd(void) const
{
	long value = 0;
	Get("execution_end", &value);
	return value;
}

void CheckResult::SetState(ServiceState state)
{
	Set("state", static_cast<long>(state));
}

ServiceState CheckResult::GetState(void) const
{
	long value = StateUnknown;
	Get("state", &value);
	return static_cast<ServiceState>(value);
}

void CheckResult::SetOutput(string output)
{
	Set("output", output);
}

string CheckResult::GetOutput(void) const
{
	string value;
	Get("output", &value);
	return value;
}

void CheckResult::SetPerformanceDataRaw(const string& pd)
{
	Set("performance_data_raw", pd);
}

string CheckResult::GetPerformanceDataRaw(void) const
{
	string value;
	Get("performance_data_raw", &value);
	return value;
}

void CheckResult::SetPerformanceData(const Dictionary::Ptr& pd)
{
	Set("performance_data", pd);
}

Dictionary::Ptr CheckResult::GetPerformanceData(void) const
{
	Dictionary::Ptr value;
	Get("performance_data", &value);
	return value;
}
