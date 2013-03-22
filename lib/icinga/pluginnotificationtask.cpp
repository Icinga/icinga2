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

#include "icinga/pluginnotificationtask.h"
#include "icinga/notification.h"
#include "icinga/service.h"
#include "icinga/macroprocessor.h"
#include "icinga/icingaapplication.h"
#include "base/scriptfunction.h"
#include "base/logger_fwd.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_SCRIPTFUNCTION(PluginNotification,  &PluginNotificationTask::ScriptFunc);

PluginNotificationTask::PluginNotificationTask(const ScriptTask::Ptr& task, const Process::Ptr& process,
    const String& service, const String& command)
	: m_Task(task), m_Process(process), m_ServiceName(service), m_Command(command)
{ }

void PluginNotificationTask::ScriptFunc(const ScriptTask::Ptr& task, const std::vector<Value>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Missing argument: Notification target must be specified."));

	if (arguments.size() < 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Missing argument: User must be specified."));

	if (arguments.size() < 3)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Missing argument: CheckResult must be specified."));

	if (arguments.size() < 4)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Missing argument: Notification type must be specified."));

	Notification::Ptr notification = arguments[0];
	User::Ptr user = arguments[1];
	Dictionary::Ptr cr = arguments[2];
	NotificationType type = static_cast<NotificationType>(static_cast<int>(arguments[3]));

	Value raw_command = notification->GetNotificationCommand();

	String service_name;

	Service::Ptr service = notification->GetService();
	if (service)
		service_name = service->GetName();

	StaticMacroResolver::Ptr notificationMacroResolver = boost::make_shared<StaticMacroResolver>();
	notificationMacroResolver->Add("NOTIFICATIONTYPE", Notification::NotificationTypeToString(type));

	std::vector<MacroResolver::Ptr> resolvers;
	resolvers.push_back(user);
	resolvers.push_back(notificationMacroResolver);
	resolvers.push_back(notification);
	resolvers.push_back(service);
	resolvers.push_back(service->GetHost());
	resolvers.push_back(IcingaApplication::GetInstance());

	Value command = MacroProcessor::ResolveMacros(raw_command, resolvers, cr, Utility::EscapeShellCmd);

	Dictionary::Ptr envMacros = boost::make_shared<Dictionary>();

	Array::Ptr export_macros = notification->GetExportMacros();

	if (export_macros) {
		BOOST_FOREACH(const String& macro, export_macros) {
			String value;

			if (!MacroProcessor::ResolveMacro(macro, resolvers, cr, &value)) {
				Log(LogWarning, "icinga", "export_macros for notification '" + notification->GetName() + "' refers to unknown macro '" + macro + "'");
				continue;
			}

			envMacros->Set(macro, value);
		}
	}

	Process::Ptr process = boost::make_shared<Process>(Process::SplitCommand(command), envMacros);

	PluginNotificationTask ct(task, process, service_name, command);

	process->Start(boost::bind(&PluginNotificationTask::ProcessFinishedHandler, ct));
}

void PluginNotificationTask::ProcessFinishedHandler(PluginNotificationTask ct)
{
	ProcessResult pr;

	try {
		pr = ct.m_Process->GetResult();

		if (pr.ExitStatus != 0) {
			std::ostringstream msgbuf;
			msgbuf << "Notification command '" << ct.m_Command << "' for service '"
			       << ct.m_ServiceName << "' failed; exit status: "
			       << pr.ExitStatus << ", output: " << pr.Output;
			Log(LogWarning, "icinga", msgbuf.str());
		}

		ct.m_Task->FinishResult(Empty);
	} catch (...) {
		ct.m_Task->FinishException(boost::current_exception());

		return;
	}
}
