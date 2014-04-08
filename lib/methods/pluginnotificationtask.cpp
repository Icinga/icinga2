/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

	Checkable::Ptr checkable = notification->GetCheckable();

	Value raw_command = commandObj->GetCommandLine();

	Dictionary::Ptr notificationExtra = make_shared<Dictionary>();
	notificationExtra->Set("type", Notification::NotificationTypeToString(type));
	notificationExtra->Set("author", author);
	notificationExtra->Set("comment", comment);

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;
	resolvers.push_back(std::make_pair("user", user));
	resolvers.push_back(std::make_pair("notification", notificationExtra));
	resolvers.push_back(std::make_pair("notification", notification));
	if (service)
		resolvers.push_back(std::make_pair("service", service));
	resolvers.push_back(std::make_pair("host", host));
	resolvers.push_back(std::make_pair("command", commandObj));
	resolvers.push_back(std::make_pair("icinga", IcingaApplication::GetInstance()));

	Value command = MacroProcessor::ResolveMacros(raw_command, resolvers, cr, Utility::EscapeShellCmd);

	Dictionary::Ptr envMacros = make_shared<Dictionary>();

	Dictionary::Ptr env = commandObj->GetEnv();

	if (env) {
		BOOST_FOREACH(const Dictionary::Pair& kv, env) {
			String name = kv.second;

			Value value = MacroProcessor::ResolveMacros(name, resolvers, checkable->GetLastCheckResult());

			envMacros->Set(kv.first, value);
		}
	}

	Process::Ptr process = make_shared<Process>(Process::SplitCommand(command), envMacros);

	process->SetTimeout(commandObj->GetTimeout());

	process->Run(boost::bind(&PluginNotificationTask::ProcessFinishedHandler, checkable, command, _1));
}

void PluginNotificationTask::ProcessFinishedHandler(const Checkable::Ptr& checkable, const Value& command, const ProcessResult& pr)
{
	if (pr.ExitStatus != 0) {
		std::ostringstream msgbuf;
		msgbuf << "Notification command '" << command << "' for object '"
		       << checkable->GetName() << "' failed; exit status: "
		       << pr.ExitStatus << ", output: " << pr.Output;
		Log(LogWarning, "icinga", msgbuf.str());
	}
}
