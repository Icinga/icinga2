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

REGISTER_SCRIPTFUNCTION("PluginNotification",  &PluginNotificationTask::ScriptFunc);

PluginNotificationTask::PluginNotificationTask(const ScriptTask::Ptr& task, const Process::Ptr& process,
    const String& service, const String& command)
	: m_Task(task), m_Process(process), m_ServiceName(service), m_Command(command)
{ }

/**
 * @threadsafety Always.
 */
void PluginNotificationTask::ScriptFunc(const ScriptTask::Ptr& task, const vector<Value>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Missing argument: Notification target must be specified."));

	if (arguments.size() < 2)
		BOOST_THROW_EXCEPTION(invalid_argument("Missing argument: Macros must be specified."));

	if (arguments.size() < 3)
		BOOST_THROW_EXCEPTION(invalid_argument("Missing argument: Notification type must be specified."));

	Notification::Ptr notification = arguments[0];
	Dictionary::Ptr macros = arguments[1];
	NotificationType type = static_cast<NotificationType>(static_cast<int>(arguments[2]));

	Value raw_command = notification->GetNotificationCommand();

	String service_name;

	Service::Ptr service = notification->GetService();
	if (service)
		service_name = service->GetName();

	Value command = MacroProcessor::ResolveMacros(raw_command, macros);

	Process::Ptr process = boost::make_shared<Process>(Process::SplitCommand(command), macros);

	PluginNotificationTask ct(task, process, service_name, command);

	process->Start(boost::bind(&PluginNotificationTask::ProcessFinishedHandler, ct));
}

/**
 * @threadsafety Always.
 */
void PluginNotificationTask::ProcessFinishedHandler(PluginNotificationTask ct)
{
	ProcessResult pr;

	try {
		pr = ct.m_Process->GetResult();

		if (pr.ExitStatus != 0) {
			stringstream msgbuf;
			msgbuf << "Notification command '" << ct.m_Command << "' for service '"
			       << ct.m_ServiceName << "' failed; exit status: "
			       << pr.ExitStatus << ", output: " << pr.Output;
			Logger::Write(LogWarning, "icinga", msgbuf.str());
		}

		ct.m_Task->FinishResult(Empty);
	} catch (...) {
		ct.m_Task->FinishException(boost::current_exception());

		return;
	}
}
