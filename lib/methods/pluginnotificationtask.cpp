/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "methods/pluginnotificationtask.hpp"
#include "icinga/notification.hpp"
#include "icinga/notificationcommand.hpp"
#include "icinga/pluginutility.hpp"
#include "icinga/service.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/function.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include "base/process.hpp"
#include "base/convert.hpp"

using namespace icinga;

REGISTER_SCRIPTFUNCTION_NS(Internal, PluginNotification, &PluginNotificationTask::ScriptFunc, "notification:user:cr:itype:author:comment:resolvedMacros:useResolvedMacros");

void PluginNotificationTask::ScriptFunc(const Notification::Ptr& notification,
    const User::Ptr& user, const CheckResult::Ptr& cr, int itype,
    const String& author, const String& comment, const Dictionary::Ptr& resolvedMacros,
    bool useResolvedMacros)
{
	NotificationCommand::Ptr commandObj = notification->GetCommand();

	NotificationType type = static_cast<NotificationType>(itype);

	Checkable::Ptr checkable = notification->GetCheckable();

	Dictionary::Ptr notificationExtra = new Dictionary();
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

	PluginUtility::ExecuteCommand(commandObj, checkable, cr, resolvers,
	    resolvedMacros, useResolvedMacros,
	    std::bind(&PluginNotificationTask::ProcessFinishedHandler, checkable, _1, _2));
}

void PluginNotificationTask::ProcessFinishedHandler(const Checkable::Ptr& checkable, const Value& commandLine, const ProcessResult& pr)
{
	if (pr.ExitStatus != 0) {
		Process::Arguments parguments = Process::PrepareCommand(commandLine);
		Log(LogWarning, "PluginNotificationTask")
		    << "Notification command for object '" << checkable->GetName() << "' (PID: " << pr.PID
		    << ", arguments: " << Process::PrettyPrintArguments(parguments) << ") terminated with exit code "
		    << pr.ExitStatus << ", output: " << pr.Output;
	}
}
