/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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

REGISTER_FUNCTION_NONCONST(Internal, PluginNotification, &PluginNotificationTask::ScriptFunc, "notification:user:cr:itype:author:comment:resolvedMacros:useResolvedMacros");

void PluginNotificationTask::ScriptFunc(const Notification::Ptr& notification,
	const User::Ptr& user, const CheckResult::Ptr& cr, int itype,
	const String& author, const String& comment, const Dictionary::Ptr& resolvedMacros,
	bool useResolvedMacros)
{
	REQUIRE_NOT_NULL(notification);
	REQUIRE_NOT_NULL(user);

	NotificationCommand::Ptr commandObj = NotificationCommand::ExecuteOverride ? NotificationCommand::ExecuteOverride : notification->GetCommand();

	auto type = static_cast<NotificationType>(itype);

	Checkable::Ptr checkable = notification->GetCheckable();

	Dictionary::Ptr notificationExtra = new Dictionary({
		{ "type", Notification::NotificationTypeToStringCompat(type) }, //TODO: Change that to our types.
		{ "author", author },
		{ "comment", comment }
	});

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;

	if (MacroResolver::OverrideMacros)
		resolvers.emplace_back("override", MacroResolver::OverrideMacros);

	resolvers.emplace_back("user", user);
	resolvers.emplace_back("notification", notificationExtra);
	resolvers.emplace_back("notification", notification);
	if (service)
		resolvers.emplace_back("service", service);
	resolvers.emplace_back("host", host);
	resolvers.emplace_back("command", commandObj);
	resolvers.emplace_back("icinga", IcingaApplication::GetInstance());

	int timeout = commandObj->GetTimeout();
	std::function<void(const Value& commandLine, const ProcessResult&)> callback;

	if (Checkable::ExecuteCommandProcessFinishedHandler) {
		callback = Checkable::ExecuteCommandProcessFinishedHandler;
	} else {
		callback = [checkable](const Value& commandline, const ProcessResult& pr) { ProcessFinishedHandler(checkable, commandline, pr); };
	}

	PluginUtility::ExecuteCommand(commandObj, checkable, cr, resolvers,
		resolvedMacros, useResolvedMacros, timeout, callback);
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
