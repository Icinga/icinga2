// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "methods/pluginnotificationtask.hpp"
#include "icinga/notification.hpp"
#include "icinga/notificationcommand.hpp"
#include "icinga/pluginutility.hpp"
#include "icinga/service.hpp"
#include "icinga/macroprocessor.hpp"
#include "base/function.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include "base/process.hpp"
#include "base/convert.hpp"

#ifdef __linux__
#	include <linux/binfmts.h>
#	include <unistd.h>

#	ifndef PAGE_SIZE
//		MAX_ARG_STRLEN is a multiple of PAGE_SIZE which is missing
#		define PAGE_SIZE getpagesize()
#	endif /* PAGE_SIZE */

// Make e.g. the $host.output$ itself even 10% shorter to leave enough room
// for e.g. --host-output= as in --host-output=$host.output$, but without int overflow
const static auto l_MaxOutLen = MAX_ARG_STRLEN - MAX_ARG_STRLEN / 10u;
#endif /* __linux__ */

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
#ifdef __linux__
		{ "comment", comment.SubStr(0, l_MaxOutLen) }
#else /* __linux__ */
		{ "comment", comment }
#endif /* __linux__ */
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

	if (service) {
#ifdef __linux__
		auto cr (service->GetLastCheckResult());

		if (cr) {
			auto output (cr->GetOutput());

			if (output.GetLength() > l_MaxOutLen) {
				resolvers.emplace_back("service", new Dictionary({{"output", output.SubStr(0, l_MaxOutLen)}}));
			}
		}
#endif /* __linux__ */

		resolvers.emplace_back("service", service);
	}

#ifdef __linux__
	auto hcr (host->GetLastCheckResult());

	if (hcr) {
		auto output (hcr->GetOutput());

		if (output.GetLength() > l_MaxOutLen) {
			resolvers.emplace_back("host", new Dictionary({{"output", output.SubStr(0, l_MaxOutLen)}}));
		}
	}
#endif /* __linux__ */

	resolvers.emplace_back("host", host);
	resolvers.emplace_back("command", commandObj);

	int timeout = commandObj->GetTimeout();
	std::function<void(const Value& commandLine, const ProcessResult&)> callback;

	if (Checkable::ExecuteCommandProcessFinishedHandler) {
		callback = Checkable::ExecuteCommandProcessFinishedHandler;
	} else {
		callback = [checkable](const Value& commandline, const ProcessResult& pr) { ProcessFinishedHandler(checkable, commandline, pr); };
	}

	PluginUtility::ExecuteCommand(commandObj, cr, resolvers,
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
