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

REGISTER_SCRIPTFUNCTION("native::PluginCheck",  &PluginCheckTask::ScriptFunc);

PluginCheckTask::PluginCheckTask(const ScriptTask::Ptr& task, const Process::Ptr& process)
	: m_Task(task), m_Process(process)
{ }

void PluginCheckTask::ScriptFunc(const ScriptTask::Ptr& task, const vector<Value>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Missing argument: Service must be specified."));

	Value vservice = arguments[0];
	if (!vservice.IsObjectType<DynamicObject>())
		BOOST_THROW_EXCEPTION(invalid_argument("Argument must be a config object."));

	Service::Ptr service = static_cast<Service::Ptr>(vservice);

	String checkCommand = service->GetCheckCommand();

	vector<Dictionary::Ptr> macroDicts;
	macroDicts.push_back(service->GetMacros());
	macroDicts.push_back(service->GetHost()->GetMacros());
	macroDicts.push_back(IcingaApplication::GetInstance()->GetMacros());
	String command = MacroProcessor::ResolveMacros(checkCommand, macroDicts);

	Process::Ptr process = boost::make_shared<Process>(command);

	PluginCheckTask ct(task, process);

	process->Start(boost::bind(&PluginCheckTask::ProcessFinishedHandler, ct));
}

void PluginCheckTask::ProcessFinishedHandler(PluginCheckTask ct)
{
	ProcessResult pr;

	try {
		pr = ct.m_Process->GetResult();
	} catch (...) {
		ct.m_Task->FinishException(boost::current_exception());
		return;
	}

	String output = pr.Output;
	output.Trim();
	Dictionary::Ptr result = ParseCheckOutput(output);
	result->Set("state", ExitStatusToState(pr.ExitStatus));
	result->Set("execution_start", pr.ExecutionStart);
	result->Set("execution_end", pr.ExecutionEnd);

	ct.m_Task->FinishResult(result);
}

ServiceState PluginCheckTask::ExitStatusToState(int exitStatus)
{
	switch (exitStatus) {
		case 0:
			return StateOK;
		case 1:
			return StateWarning;
		case 2:
			return StateCritical;
		default:
			return StateUnknown;
	}
}

Dictionary::Ptr PluginCheckTask::ParseCheckOutput(const String& output)
{
	Dictionary::Ptr result = boost::make_shared<Dictionary>();

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

	return result;
}
