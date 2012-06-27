#include "i2-icinga.h"

using namespace icinga;

CheckResult::CheckResult(void)
	: m_Data(boost::make_shared<Dictionary>())
{ }

CheckResult::CheckResult(const Dictionary::Ptr& dictionary)
	: m_Data(dictionary)
{ }

Dictionary::Ptr CheckResult::GetDictionary(void) const
{
	return m_Data;
}

void CheckResult::SetScheduleStart(time_t ts)
{
	m_Data->SetProperty("schedule_start", static_cast<long>(ts));
}

time_t CheckResult::GetScheduleStart(void) const
{
	long value = 0;
	m_Data->GetProperty("schedule_start", &value);
	return static_cast<time_t>(value);
}

void CheckResult::SetScheduleEnd(time_t ts)
{
	m_Data->SetProperty("schedule_end", static_cast<long>(ts));
}

time_t CheckResult::GetScheduleEnd(void) const
{
	long value = 0;
	m_Data->GetProperty("schedule_end", &value);
	return static_cast<time_t>(value);
}

void CheckResult::SetExecutionStart(time_t ts)
{
	m_Data->SetProperty("execution_start", static_cast<long>(ts));
}

time_t CheckResult::GetExecutionStart(void) const
{
	long value = 0;
	m_Data->GetProperty("execution_start", &value);
	return static_cast<time_t>(value);
}

void CheckResult::SetExecutionEnd(time_t ts)
{
	m_Data->SetProperty("execution_end", static_cast<long>(ts));
}

time_t CheckResult::GetExecutionEnd(void) const
{
	long value = 0;
	m_Data->GetProperty("execution_end", &value);
	return value;
}

void CheckResult::SetState(ServiceState state)
{
	m_Data->SetProperty("state", static_cast<long>(state));
}

ServiceState CheckResult::GetState(void) const
{
	long value = StateUnknown;
	m_Data->GetProperty("state", &value);
	return static_cast<ServiceState>(value);
}

void CheckResult::SetOutput(string output)
{
	m_Data->SetProperty("output", output);
}

string CheckResult::GetOutput(void) const
{
	string value;
	m_Data->GetProperty("output", &value);
	return value;
}

void CheckResult::SetPerformanceData(const Dictionary::Ptr& pd)
{
	m_Data->SetProperty("performance_data", pd);
}

Dictionary::Ptr CheckResult::GetPerformanceData(void) const
{
	Dictionary::Ptr value;
	m_Data->GetProperty("performance_data", &value);
	return value;
}
