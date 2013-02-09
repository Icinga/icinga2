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

REGISTER_SCRIPTFUNCTION("native::PluginNotification",  &PluginNotificationTask::ScriptFunc);

PluginNotificationTask::PluginNotificationTask(const ScriptTask::Ptr& task, const Process::Ptr& process)
	: m_Task(task), m_Process(process)
{ }

void PluginNotificationTask::ScriptFunc(const ScriptTask::Ptr& task, const vector<Value>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Missing argument: Notification target must be specified."));

	if (!arguments[0].IsObjectType<Notification>())
		BOOST_THROW_EXCEPTION(invalid_argument("Argument must be a service."));

	Notification::Ptr notification = arguments[0];

	String notificationCommand = notification->GetNotificationCommand();

	vector<Dictionary::Ptr> macroDicts;
	macroDicts.push_back(notification->GetMacros());
	macroDicts.push_back(notification->GetService()->GetMacros());
	macroDicts.push_back(notification->GetService()->GetHost()->GetMacros());
	macroDicts.push_back(IcingaApplication::GetInstance()->GetMacros());
	String command = MacroProcessor::ResolveMacros(notificationCommand, macroDicts);

	Process::Ptr process = boost::make_shared<Process>(command);

	PluginNotificationTask ct(task, process);

	process->Start(boost::bind(&PluginNotificationTask::ProcessFinishedHandler, ct));
}

void PluginNotificationTask::ProcessFinishedHandler(PluginNotificationTask ct)
{
	ProcessResult pr;

	try {
		pr = ct.m_Process->GetResult();

		if (pr.ExitStatus != 0)
			Logger::Write(LogWarning, "icinga", "Notification command failed; output: " + pr.Output);

		ct.m_Task->FinishResult(Empty);
	} catch (...) {
		ct.m_Task->FinishException(boost::current_exception());
		return;
	}
}
