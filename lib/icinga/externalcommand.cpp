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

bool I2_EXPORT ExternalCommand::m_Initialized;
map<String, ExternalCommand::Callback> I2_EXPORT ExternalCommand::m_Commands;

int ExternalCommand::Execute(double time, const String& command, const vector<String>& arguments)
{
	if (!m_Initialized) {
		RegisterCommand("HELLO_WORLD", &ExternalCommand::HelloWorld);
		RegisterCommand("PROCESS_SERVICE_CHECK_RESULT", &ExternalCommand::ProcessServiceCheckResult);

		m_Initialized = true;
	}

	map<String, ExternalCommand::Callback>::iterator it;
	it = m_Commands.find(command);

	if (it == m_Commands.end())
		return -1;

	return it->second(time, arguments);
}

void ExternalCommand::RegisterCommand(const String& command, const ExternalCommand::Callback& callback)
{
	m_Commands[command] = callback;
}

int ExternalCommand::HelloWorld(double time, const vector<String>& arguments)
{
	Logger::Write(LogInformation, "icinga", "HelloWorld external command called.");

	return 0;
}

int ExternalCommand::ProcessServiceCheckResult(double time, const vector<String>& arguments)
{
	if (arguments.size() < 4)
		return -1;

	if (!Service::Exists(arguments[1]))
		return -1;

	Service::Ptr service = Service::GetByName(arguments[1]);

	int exitStatus = arguments[2].ToDouble();
	Dictionary::Ptr result = PluginCheckTask::ParseCheckOutput(arguments[3]);
	result->Set("state", PluginCheckTask::ExitStatusToState(exitStatus));

	double now = Utility::GetTime();
	result->Set("schedule_start", time);
	result->Set("schedule_end", time);
	result->Set("execution_start", time);
	result->Set("execution_end", time);

	service->ProcessCheckResult(result);

	return 0;
}
