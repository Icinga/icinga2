/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "methods/pluginnotificationtask.h"
#include "icinga/notification.h"
#include "icinga/notificationcommand.h"
#include "icinga/service.h"
#include "icinga/macroprocessor.h"
#include "icinga/icingaapplication.h"
#include "base/scriptfunction.h"
#include "base/logger_fwd.h"
#include "base/utility.h"
#include "base/process.h"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_SCRIPTFUNCTION(PluginNotification, &PluginNotificationTask::ScriptFunc);

void PluginNotificationTask::ScriptFunc(const Notification::Ptr& notification, const User::Ptr& user, const CheckResult::Ptr& cr, int itype,
    const String& author, const String& comment)
{
	NotificationCommand::Ptr commandObj = notification->GetNotificationCommand();

	NotificationType type = static_cast<NotificationType>(itype);

	Service::Ptr service = notification->GetService();

	Value raw_command = commandObj->GetCommandLine();

	StaticMacroResolver::Ptr notificationMacroResolver = make_shared<StaticMacroResolver>();
	notificationMacroResolver->Add("NOTIFICATIONTYPE", Notification::NotificationTypeToString(type));
	notificationMacroResolver->Add("NOTIFICATIONAUTHOR", author);
	notificationMacroResolver->Add("NOTIFICATIONAUTHORNAME", author);
	notificationMacroResolver->Add("NOTIFICATIONCOMMENT", comment);

	std::vector<MacroResolver::Ptr> resolvers;
	resolvers.push_back(user);
	resolvers.push_back(notificationMacroResolver);
	resolvers.push_back(notification);
	resolvers.push_back(service);
	resolvers.push_back(service->GetHost());
	resolvers.push_back(commandObj);
	resolvers.push_back(IcingaApplication::GetInstance());

	Value command = MacroProcessor::ResolveMacros(raw_command, resolvers, cr, Utility::EscapeShellCmd, commandObj->GetEscapeMacros());

	Dictionary::Ptr envMacros = make_shared<Dictionary>();

	Array::Ptr export_macros = commandObj->GetExportMacros();

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

	Process::Ptr process = make_shared<Process>(Process::SplitCommand(command), envMacros);

	process->SetTimeout(commandObj->GetTimeout());

	ProcessResult pr = process->Run();

	if (pr.ExitStatus != 0) {
		std::ostringstream msgbuf;
		msgbuf << "Notification command '" << command << "' for service '"
		       << service->GetName() << "' failed; exit status: "
		       << pr.ExitStatus << ", output: " << pr.Output;
		Log(LogWarning, "icinga", msgbuf.str());
	}
}
