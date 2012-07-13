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

NagiosCheckTask::NagiosCheckTask(const Service& service)
	: CheckTask(service)
{
	string checkCommand = service.GetCheckCommand();

	vector<Dictionary::Ptr> macroDicts;
	macroDicts.push_back(service.GetMacros());
	macroDicts.push_back(service.GetHost().GetMacros());
	macroDicts.push_back(IcingaApplication::GetInstance()->GetMacros());
	string command = MacroProcessor::ResolveMacros(checkCommand, macroDicts);
	m_Process = boost::make_shared<Process>(command);
}

void NagiosCheckTask::Run(void)
{
	time_t now;
	time(&now);
	GetResult().SetScheduleStart(now);

	m_Process->OnTaskCompleted.connect(boost::bind(&NagiosCheckTask::ProcessFinishedHandler, static_cast<NagiosCheckTask::Ptr>(GetSelf())));
	m_Process->Start();
}

void NagiosCheckTask::ProcessFinishedHandler(void)
{
	time_t now;
	time(&now);
	GetResult().SetExecutionEnd(now);

	string output = m_Process->GetOutput();
	boost::algorithm::trim(output);
	ProcessCheckOutput(output);

	long exitcode = m_Process->GetExitStatus();

	ServiceState state;

	switch (exitcode) {
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

	GetResult().SetState(state);

	time(&now);
	GetResult().SetScheduleEnd(now);

	Finish();
}

void NagiosCheckTask::ProcessCheckOutput(const string& output)
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

	GetResult().SetOutput(text);
	GetResult().SetPerformanceDataRaw(perfdata);
}

CheckTask::Ptr NagiosCheckTask::CreateTask(const Service& service)
{
	return boost::make_shared<NagiosCheckTask>(service);
}

void NagiosCheckTask::Register(void)
{
	CheckTask::RegisterType("nagios", NagiosCheckTask::CreateTask);
}
