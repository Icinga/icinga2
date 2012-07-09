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
