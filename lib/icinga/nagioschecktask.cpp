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

#include "i2-icinga.h"

using namespace icinga;

NagiosCheckTask::NagiosCheckTask(const ScriptTask::Ptr& task, const Process::Ptr& process)
	: m_Task(task), m_Process(process)
{ }

void NagiosCheckTask::ScriptFunc(const ScriptTask::Ptr& task, const vector<Value>& arguments)
{
	if (arguments.size() < 1)
		throw_exception(invalid_argument("Missing argument: Service must be specified."));

	Value vservice = arguments[0];
	if (!vservice.IsObjectType<DynamicObject>())
		throw_exception(invalid_argument("Argument must be a config object."));

	Service::Ptr service = static_cast<Service::Ptr>(vservice);

	String checkCommand = service->GetCheckCommand();

	vector<Dictionary::Ptr> macroDicts;
	macroDicts.push_back(service->GetMacros());
	macroDicts.push_back(service->GetHost()->GetMacros());
	macroDicts.push_back(IcingaApplication::GetInstance()->GetMacros());
	String command = MacroProcessor::ResolveMacros(checkCommand, macroDicts);

	Process::Ptr process = boost::make_shared<Process>(command);

	NagiosCheckTask ct(task, process);

	ct.m_Result = boost::make_shared<Dictionary>();
	ct.m_Result->Set("schedule_start", Utility::GetTime());

	process->Start(boost::bind(&NagiosCheckTask::ProcessFinishedHandler, ct));
}

void NagiosCheckTask::ProcessFinishedHandler(NagiosCheckTask ct)
{
	ProcessResult pr;

	try {
		pr = ct.m_Process->GetResult();
	} catch (...) {
		ct.m_Task->FinishException(boost::current_exception());
		return;
	}

	ct.m_Result->Set("execution_start", pr.ExecutionStart);
	ct.m_Result->Set("execution_end", pr.ExecutionEnd);

	String output = pr.Output;
	output.Trim();
	ProcessCheckOutput(ct.m_Result, output);

	ServiceState state;

	switch (pr.ExitStatus) {
		case 0:
			state = StateOK;
			break;
		case 1:
			state = StateWarning;
			break;
		case 2:
			state = StateCritical;
			break;
		default:
			state = StateUnknown;
			break;
	}

	ct.m_Result->Set("state", state);

	ct.m_Result->Set("schedule_end", Utility::GetTime());

	ct.m_Task->FinishResult(ct.m_Result);
}

void NagiosCheckTask::ProcessCheckOutput(const Dictionary::Ptr& result, String& output)
{
	String text;
	String perfdata;

	vector<String> lines = output.Split(is_any_of("\r\n"));

	BOOST_FOREACH (const String& line, lines) {
		size_t delim = line.FindFirstOf("|");

		if (!text.IsEmpty())
			text += "\n";

		if (delim != String::NPos) {
			text += line.SubStr(0, delim);

			if (!perfdata.IsEmpty())
				perfdata += " ";

			perfdata += line.SubStr(delim + 1, line.GetLength());
		} else {
			text += line;
		}
	}

	result->Set("output", text);
	result->Set("performance_data_raw", perfdata);
}

void NagiosCheckTask::Register(void)
{
	ScriptFunction::Ptr func = boost::make_shared<ScriptFunction>(&NagiosCheckTask::ScriptFunc);
	ScriptFunction::Register("native::NagiosCheck", func);
}
