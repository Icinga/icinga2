/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/compatutility.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/notificationcommand.hpp"
#include "icinga/pluginutility.hpp"
#include "icinga/service.hpp"
#include "base/utility.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/join.hpp>

using namespace icinga;

/* Used in DB IDO and Livestatus. */
String CompatUtility::GetCommandLine(const Command::Ptr& command)
{
	Value commandLine = command->GetCommandLine();

	String result;
	if (commandLine.IsObjectType<Array>()) {
		Array::Ptr args = commandLine;

		ObjectLock olock(args);
		for (const String& arg : args) {
			// This is obviously incorrect for non-trivial cases.
			result += " \"" + EscapeString(arg) + "\"";
		}
	} else if (!commandLine.IsEmpty()) {
		result = EscapeString(Convert::ToString(commandLine));
	} else {
		result = "<internal>";
	}

	return result;
}

String CompatUtility::GetCommandNamePrefix(const Command::Ptr& command)
/* Helper. */
{
	if (!command)
		return Empty;

	String prefix;
	if (command->GetReflectionType() == CheckCommand::TypeInstance)
		prefix = "check_";
	else if (command->GetReflectionType() == NotificationCommand::TypeInstance)
		prefix = "notification_";
	else if (command->GetReflectionType() == EventCommand::TypeInstance)
		prefix = "event_";

	return prefix;
}

String CompatUtility::GetCommandName(const Command::Ptr& command)
/* Used in DB IDO and Livestatus. */
{
	if (!command)
		return Empty;

	return GetCommandNamePrefix(command) + command->GetName();
}

/* Used in DB IDO and Livestatus. */
String CompatUtility::GetCheckableCommandArgs(const Checkable::Ptr& checkable)
{
	CheckCommand::Ptr command = checkable->GetCheckCommand();

	Dictionary::Ptr args = new Dictionary();

	if (command) {
		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(checkable);
		String command_line = GetCommandLine(command);

		Dictionary::Ptr command_vars = command->GetVars();

		if (command_vars) {
			ObjectLock olock(command_vars);
			for (const Dictionary::Pair& kv : command_vars) {
				String macro = "$" + kv.first + "$"; // this is too simple
				if (command_line.Contains(macro))
					args->Set(kv.first, kv.second);

			}
		}

		Dictionary::Ptr host_vars = host->GetVars();

		if (host_vars) {
			ObjectLock olock(host_vars);
			for (const Dictionary::Pair& kv : host_vars) {
				String macro = "$" + kv.first + "$"; // this is too simple
				if (command_line.Contains(macro))
					args->Set(kv.first, kv.second);
				macro = "$host.vars." + kv.first + "$";
				if (command_line.Contains(macro))
					args->Set(kv.first, kv.second);
			}
		}

		if (service) {
			Dictionary::Ptr service_vars = service->GetVars();

			if (service_vars) {
				ObjectLock olock(service_vars);
				for (const Dictionary::Pair& kv : service_vars) {
					String macro = "$" + kv.first + "$"; // this is too simple
					if (command_line.Contains(macro))
						args->Set(kv.first, kv.second);
					macro = "$service.vars." + kv.first + "$";
					if (command_line.Contains(macro))
						args->Set(kv.first, kv.second);
				}
			}
		}

		String arg_string;
		ObjectLock olock(args);
		for (const Dictionary::Pair& kv : args) {
			arg_string += Convert::ToString(kv.first) + "=" + Convert::ToString(kv.second) + "!";
		}
		return arg_string;
	}

	return Empty;
}

/* Used in DB IDO and Livestatus. */
int CompatUtility::GetCheckableNotificationLastNotification(const Checkable::Ptr& checkable)
{
	double last_notification = 0.0;
	for (const Notification::Ptr& notification : checkable->GetNotifications()) {
		if (notification->GetLastNotification() > last_notification)
			last_notification = notification->GetLastNotification();
	}

	return static_cast<int>(last_notification);
}

/* Used in DB IDO and Livestatus. */
int CompatUtility::GetCheckableNotificationNextNotification(const Checkable::Ptr& checkable)
{
	double next_notification = 0.0;
	for (const Notification::Ptr& notification : checkable->GetNotifications()) {
		if (next_notification == 0 || notification->GetNextNotification() < next_notification)
			next_notification = notification->GetNextNotification();
	}

	return static_cast<int>(next_notification);
}

/* Used in DB IDO and Livestatus. */
int CompatUtility::GetCheckableNotificationNotificationNumber(const Checkable::Ptr& checkable)
{
	int notification_number = 0;
	for (const Notification::Ptr& notification : checkable->GetNotifications()) {
		if (notification->GetNotificationNumber() > notification_number)
			notification_number = notification->GetNotificationNumber();
	}

	return notification_number;
}

/* Used in DB IDO and Livestatus. */
double CompatUtility::GetCheckableNotificationNotificationInterval(const Checkable::Ptr& checkable)
{
	double notification_interval = -1;

	for (const Notification::Ptr& notification : checkable->GetNotifications()) {
		if (notification_interval == -1 || notification->GetInterval() < notification_interval)
			notification_interval = notification->GetInterval();
	}

	if (notification_interval == -1)
		notification_interval = 60;

	return notification_interval / 60.0;
}

/* Helper. */
int CompatUtility::GetCheckableNotificationTypeFilter(const Checkable::Ptr& checkable)
{
	unsigned long notification_type_filter = 0;

	for (const Notification::Ptr& notification : checkable->GetNotifications()) {
		ObjectLock olock(notification);

		notification_type_filter |= notification->GetTypeFilter();
	}

	return notification_type_filter;
}

/* Helper. */
int CompatUtility::GetCheckableNotificationStateFilter(const Checkable::Ptr& checkable)
{
	unsigned long notification_state_filter = 0;

	for (const Notification::Ptr& notification : checkable->GetNotifications()) {
		ObjectLock olock(notification);

		notification_state_filter |= notification->GetStateFilter();
	}

	return notification_state_filter;
}

/* Used in DB IDO and Livestatus. */
std::set<User::Ptr> CompatUtility::GetCheckableNotificationUsers(const Checkable::Ptr& checkable)
{
	/* Service -> Notifications -> (Users + UserGroups -> Users) */
	std::set<User::Ptr> allUsers;
	std::set<User::Ptr> users;

	for (const Notification::Ptr& notification : checkable->GetNotifications()) {
		ObjectLock olock(notification);

		users = notification->GetUsers();

		std::copy(users.begin(), users.end(), std::inserter(allUsers, allUsers.begin()));

		for (const UserGroup::Ptr& ug : notification->GetUserGroups()) {
			std::set<User::Ptr> members = ug->GetMembers();
			std::copy(members.begin(), members.end(), std::inserter(allUsers, allUsers.begin()));
		}
	}

	return allUsers;
}

/* Used in DB IDO and Livestatus. */
std::set<UserGroup::Ptr> CompatUtility::GetCheckableNotificationUserGroups(const Checkable::Ptr& checkable)
{
	std::set<UserGroup::Ptr> usergroups;
	/* Service -> Notifications -> UserGroups */
	for (const Notification::Ptr& notification : checkable->GetNotifications()) {
		ObjectLock olock(notification);

		for (const UserGroup::Ptr& ug : notification->GetUserGroups()) {
			usergroups.insert(ug);
		}
	}

	return usergroups;
}

/* Used in DB IDO, Livestatus, CompatLogger, GelfWriter, IcingaDB. */
String CompatUtility::GetCheckResultOutput(const CheckResult::Ptr& cr)
{
	if (!cr)
		return Empty;

	String output;

	String raw_output = cr->GetOutput();

	size_t line_end = raw_output.Find("\n");

	return raw_output.SubStr(0, line_end);
}

/* Used in DB IDO, Livestatus and IcingaDB. */
String CompatUtility::GetCheckResultLongOutput(const CheckResult::Ptr& cr)
{
	if (!cr)
		return Empty;

	String long_output;
	String output;

	String raw_output = cr->GetOutput();

	size_t line_end = raw_output.Find("\n");

	if (line_end > 0 && line_end != String::NPos) {
		long_output = raw_output.SubStr(line_end+1, raw_output.GetLength());
		return EscapeString(long_output);
	}

	return Empty;
}

/* Helper for DB IDO and Livestatus. */
String CompatUtility::EscapeString(const String& str)
{
	String result = str;
	boost::algorithm::replace_all(result, "\n", "\\n");
	return result;
}

/* Used in ExternalCommandListener. */
String CompatUtility::UnEscapeString(const String& str)
{
	String result = str;
	boost::algorithm::replace_all(result, "\\n", "\n");
	return result;
}
