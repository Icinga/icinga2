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

void NagiosCheckTask::ScriptFunc(const ScriptTask::Ptr& task, const vector<Variant>& arguments)
{
	if (arguments.size() < 1)
		throw invalid_argument("Missing argument: Service must be specified.");

	Variant vservice = arguments[0];
	if (!vservice.IsObjectType<ConfigObject>())
		throw invalid_argument("Argument must be a config object.");

	Service service = static_cast<ConfigObject::Ptr>(vservice);

	string checkCommand = service.GetCheckCommand();

	vector<Dictionary::Ptr> macroDicts;
	macroDicts.push_back(service.GetMacros());
	/* TODO: figure out whether we should replicate hosts to checkers,
	 * for now we just rely on the convenience module to fill in host macros
	 * for inline service definitions. */
	//macroDicts.push_back(service.GetHost().GetMacros());
	macroDicts.push_back(IcingaApplication::GetInstance()->GetMacros());
	string command = MacroProcessor::ResolveMacros(checkCommand, macroDicts);

	CheckResult result;

	time_t now;
	time(&now);
	result.SetScheduleStart(now);

	Process::Ptr process = boost::make_shared<Process>(command, boost::bind(&NagiosCheckTask::ProcessFinishedHandler, task, _1, result));
	process->Start();
}

void NagiosCheckTask::ProcessFinishedHandler(const ScriptTask::Ptr& task, const Process::Ptr& process, CheckResult result)
{
	ProcessResult pr;
	pr = process->GetResult();

	result.SetExecutionStart(pr.ExecutionStart);
	result.SetExecutionEnd(pr.ExecutionEnd);

	string output = pr.Output;
	boost::algorithm::trim(output);
	ProcessCheckOutput(result, output);

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

	result.SetState(state);

	time_t now;
	time(&now);
	result.SetScheduleEnd(now);

	task->Finish(result.GetDictionary());
}

void NagiosCheckTask::ProcessCheckOutput(CheckResult& result, const string& output)
{
	string text;
	string perfdata;

	vector<string> lines;
	boost::algorithm::split(lines, output, is_any_of("\r\n"));

	vector<string>::iterator it;
	for (it = lines.begin(); it != lines.end(); it++) {
		const string& line = *it;

		string::size_type delim = line.find('|');

		if (!text.empty())
			text.append("\n");

		if (delim != string::npos) {
			text.append(line, 0, delim);

			if (!perfdata.empty())
				perfdata.append(" ");

			perfdata.append(line, delim + 1, line.size());
		} else {
			text.append(line);
		}
	}

	result.SetOutput(text);
	result.SetPerformanceDataRaw(perfdata);
}

void NagiosCheckTask::Register(void)
{
	ScriptFunction::Ptr func = boost::make_shared<ScriptFunction>(&NagiosCheckTask::ScriptFunc);
	ScriptFunction::Register("native::NagiosCheck", func);
}
