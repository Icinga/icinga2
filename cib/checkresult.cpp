#include "i2-cib.h"

using namespace icinga;

void CheckResult::SetScheduleStart(time_t ts)
{
	SetProperty("schedule_start", static_cast<long>(ts));
}

time_t CheckResult::GetScheduleStart(void) const
{
	long value = 0;
	GetProperty("schedule_start", &value);
	return static_cast<time_t>(value);
}

void CheckResult::SetScheduleEnd(time_t ts)
{
	SetProperty("schedule_end", static_cast<long>(ts));
}

time_t CheckResult::GetScheduleEnd(void) const
{
	long value = 0;
	GetProperty("schedule_end", &value);
	return static_cast<time_t>(value);
}

void CheckResult::SetExecutionStart(time_t ts)
{
	SetProperty("execution_start", static_cast<long>(ts));
}

time_t CheckResult::GetExecutionStart(void) const
{
	long value = 0;
	GetProperty("execution_start", &value);
	return static_cast<time_t>(value);
}

void CheckResult::SetExecutionEnd(time_t ts)
{
	SetProperty("execution_end", static_cast<long>(ts));
}

time_t CheckResult::GetExecutionEnd(void) const
{
	long value = 0;
	GetProperty("execution_end", &value);
	return value;
}

void CheckResult::SetState(ServiceState state)
{
	SetProperty("state", static_cast<long>(state));
}

ServiceState CheckResult::GetState(void) const
{
	long value = StateUnknown;
	GetProperty("state", &value);
	return static_cast<ServiceState>(value);
}

void CheckResult::SetOutput(string output)
{
	SetProperty("output", output);
}

string CheckResult::GetOutput(void) const
{
	string value;
	GetProperty("output", &value);
	return value;
}

void CheckResult::SetPerformanceDataRaw(const string& pd)
{
	SetProperty("performance_data_raw", pd);
}

string CheckResult::GetPerformanceDataRaw(void) const
{
	string value;
	GetProperty("performance_data_raw", &value);
	return value;
}

void CheckResult::SetPerformanceData(const Dictionary::Ptr& pd)
{
	SetProperty("performance_data", pd);
}

Dictionary::Ptr CheckResult::GetPerformanceData(void) const
{
	Dictionary::Ptr value;
	GetProperty("performance_data", &value);
	return value;
}
