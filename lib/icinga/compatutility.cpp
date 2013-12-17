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

#include "icinga/compatutility.h"
#include "icinga/checkcommand.h"
#include "icinga/eventcommand.h"
#include "icinga/pluginutility.h"
#include "base/utility.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/debug.h"
#include "base/convert.h"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>

using namespace icinga;

/* command */
String CompatUtility::GetCommandLine(const Command::Ptr& command)
{
	Value commandLine = command->GetCommandLine();

	String result;
	if (commandLine.IsObjectType<Array>()) {
		Array::Ptr args = commandLine;

		ObjectLock olock(args);
		String arg;
		BOOST_FOREACH(arg, args) {
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

/* host */
String CompatUtility::GetHostAlias(const Host::Ptr& host)
{
	ASSERT(host->OwnsLock());

	if (!host->GetDisplayName().IsEmpty())
		return host->GetName();
	else
		return host->GetDisplayName();
}

String CompatUtility::GetHostAddress(const Host::Ptr& host)
{
	ASSERT(host->OwnsLock());

	Dictionary::Ptr macros = host->GetMacros();

	String address;

	if (macros)
		address = macros->Get("address");

	if (address.IsEmpty())
		address = host->GetName();

	return address;
}

String CompatUtility::GetHostAddress6(const Host::Ptr& host)
{
	ASSERT(host->OwnsLock());

	Dictionary::Ptr macros = host->GetMacros();

	String address6;

	if (macros)
		address6 = macros->Get("address6");

	if (address6.IsEmpty())
		address6 = host->GetName();

	return address6;
}

Host2dCoords CompatUtility::GetHost2dCoords(const Host::Ptr& host)
{
	ASSERT(host->OwnsLock());

	Dictionary::Ptr custom = host->GetCustom();
	Host2dCoords bag;

	if (custom) {
		bag.have_2d_coords = (custom->Get("x_2d") && custom->Get("y_2d") ? 1 : 0);

		if (bag.have_2d_coords == 1) {
			bag.x_2d = custom->Get("x_2d");
			bag.y_2d = custom->Get("y_2d");
		}
	} else {
		bag.have_2d_coords = 0;
	}

	return bag;
}

int CompatUtility::GetHostNotifyOnDown(const Host::Ptr& host)
{
	ASSERT(host->OwnsLock());

	Service::Ptr service = host->GetCheckService();

	if (!service)
		return 0;

	unsigned long notification_state_filter = GetServiceNotificationStateFilter(service);

	if (notification_state_filter & (1<<StateCritical) ||
	    notification_state_filter & (1<<StateWarning))
		return 1;

	return 0;
}

int CompatUtility::GetHostNotifyOnUnreachable(const Host::Ptr& host)
{
	ASSERT(host->OwnsLock());

	Service::Ptr service = host->GetCheckService();

	if (!service)
		return 0;

	unsigned long notification_state_filter = GetServiceNotificationStateFilter(service);

	if (notification_state_filter & (1<<StateUnknown))
		return 1;

	return 0;
}

/* service */
int CompatUtility::GetServiceCurrentState(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	int state = service->GetState();

	if (state > StateUnknown)
		return StateUnknown;

	return state;
}

int CompatUtility::GetServiceShouldBeScheduled(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return (service->GetEnableActiveChecks() ? 1 : 0);
}

int CompatUtility::GetServiceCheckType(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return (service->GetEnableActiveChecks() ? 0 : 1);
}

double CompatUtility::GetServiceCheckInterval(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return service->GetCheckInterval() / 60.0;
}

double CompatUtility::GetServiceRetryInterval(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return service->GetRetryInterval() / 60.0;
}

String CompatUtility::GetServiceCheckPeriod(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	TimePeriod::Ptr check_period = service->GetCheckPeriod();
	if (check_period)
		return check_period->GetName();
	else
		return "24x7";
}

int CompatUtility::GetServiceHasBeenChecked(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return (service->GetLastCheckResult() ? 1 : 0);
}


int CompatUtility::GetServiceProblemHasBeenAcknowledged(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return (service->GetAcknowledgement() != AcknowledgementNone ? 1 : 0);
}

int CompatUtility::GetServiceAcknowledgementType(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return static_cast<int>(service->GetAcknowledgement());
}

int CompatUtility::GetServicePassiveChecksEnabled(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return (service->GetEnablePassiveChecks() ? 1 : 0);
}

int CompatUtility::GetServiceActiveChecksEnabled(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return (service->GetEnableActiveChecks() ? 1 : 0);
}

int CompatUtility::GetServiceEventHandlerEnabled(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return (service->GetEventCommand() ? 1 : 0);
}

int CompatUtility::GetServiceFlapDetectionEnabled(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return (service->GetEnableFlapping() ? 1 : 0);
}

int CompatUtility::GetServiceIsFlapping(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return (service->IsFlapping() ? 1 : 0);
}

String CompatUtility::GetServicePercentStateChange(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return Convert::ToString(service->GetFlappingCurrent());
}

int CompatUtility::GetServiceProcessPerformanceData(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return (service->GetEnablePerfdata() ? 1 : 0);
}

String CompatUtility::GetServiceEventHandler(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	String event_command_str;
	EventCommand::Ptr eventcommand = service->GetEventCommand();

	if (eventcommand)
		event_command_str = eventcommand->GetName();

	return event_command_str;
}

String CompatUtility::GetServiceCheckCommand(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	String check_command_str;
	CheckCommand::Ptr checkcommand = service->GetCheckCommand();

	if (checkcommand)
		check_command_str = checkcommand->GetName();

	return check_command_str;
}

int CompatUtility::GetServiceIsVolatile(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return (service->GetVolatile() ? 1 : 0);
}

double CompatUtility::GetServiceLowFlapThreshold(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return service->GetFlappingThreshold();
}

double CompatUtility::GetServiceHighFlapThreshold(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return service->GetFlappingThreshold();
}

int CompatUtility::GetServiceFreshnessChecksEnabled(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return (service->GetCheckInterval() > 0 ? 1 : 0);
}

int CompatUtility::GetServiceFreshnessThreshold(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return static_cast<int>(service->GetCheckInterval());
}

double CompatUtility::GetServiceStaleness(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	if (service->HasBeenChecked() && service->GetLastCheck() > 0)
		return (Utility::GetTime() - service->GetLastCheck()) / (service->GetCheckInterval() * 3600);

	return 0.0;
}

int CompatUtility::GetServiceIsAcknowledged(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return (service->IsAcknowledged() ? 1 : 0);
}

int CompatUtility::GetServiceNoMoreNotifications(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

        if (CompatUtility::GetServiceNotificationNotificationInterval(service) == 0 && !service->GetVolatile())
                return 1;

	return 0;
}

int CompatUtility::GetServiceInCheckPeriod(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	TimePeriod::Ptr timeperiod = service->GetCheckPeriod();

	/* none set means always checked */
	if (!timeperiod)
		return 1;

	return (timeperiod->IsInside(Utility::GetTime()) ? 1 : 0);
}

int CompatUtility::GetServiceInNotificationPeriod(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
		ObjectLock olock(notification);

		TimePeriod::Ptr timeperiod = notification->GetNotificationPeriod();

		/* first notification wins */
		if (timeperiod)
			return (timeperiod->IsInside(Utility::GetTime()) ? 1 : 0);
	}

	/* none set means always notified */
	return 1;
}

/* custom attr */
Dictionary::Ptr CompatUtility::GetCustomVariableConfig(const DynamicObject::Ptr& object)
{
	ASSERT(object->OwnsLock());

	Dictionary::Ptr custom;

	if (object->GetType() == DynamicType::GetByName("Host")) {
		custom = static_pointer_cast<Host>(object)->GetCustom();
	} else if (object->GetType() == DynamicType::GetByName("Service")) {
		custom = static_pointer_cast<Service>(object)->GetCustom();
	} else if (object->GetType() == DynamicType::GetByName("User")) {
		custom = static_pointer_cast<User>(object)->GetCustom();
	} else {
		Log(LogDebug, "icinga", "unknown object type for custom vars");
		return Dictionary::Ptr();
	}

	Dictionary::Ptr customvars = make_shared<Dictionary>();

	if (!custom)
		return Dictionary::Ptr();

	ObjectLock olock(custom);
	String key;
	Value value;
	BOOST_FOREACH(const Dictionary::Pair& kv, custom) {
		if (kv.first == "notes" ||
		    kv.first == "action_url" ||
		    kv.first == "notes_url" ||
		    kv.first == "icon_image" ||
		    kv.first == "icon_image_alt" ||
		    kv.first == "statusmap_image" ||
		    kv.first == "2d_coords")
			continue;

		customvars->Set(key, value);
	}

	return customvars;
}

String CompatUtility::GetCustomAttributeConfig(const DynamicObject::Ptr& object, const String& name)
{
	ASSERT(object->OwnsLock());

	Dictionary::Ptr custom;

	if (object->GetType() == DynamicType::GetByName("Host")) {
		custom = static_pointer_cast<Host>(object)->GetCustom();
	} else if (object->GetType() == DynamicType::GetByName("Service")) {
		custom = static_pointer_cast<Service>(object)->GetCustom();
	} else if (object->GetType() == DynamicType::GetByName("User")) {
		custom = static_pointer_cast<User>(object)->GetCustom();
	} else {
		Log(LogDebug, "icinga", "unknown object type for custom attributes");
		return Empty;
	}

	if (!custom)
		return Empty;

	return custom->Get(name);
}

/* notifications */
int CompatUtility::GetServiceNotificationsEnabled(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	return (service->GetEnableNotifications() ? 1 : 0);
}

int CompatUtility::GetServiceNotificationLastNotification(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	double last_notification = 0.0;
	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
		if (notification->GetLastNotification() > last_notification)
			last_notification = notification->GetLastNotification();
	}

	return static_cast<int>(last_notification);
}

int CompatUtility::GetServiceNotificationNextNotification(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	double next_notification = 0.0;
	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
		if (notification->GetNextNotification() < next_notification)
			next_notification = notification->GetNextNotification();
	}

	return static_cast<int>(next_notification);
}

int CompatUtility::GetServiceNotificationNotificationNumber(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	int notification_number = 0;
	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
		if (notification->GetNotificationNumber() > notification_number)
			notification_number = notification->GetNotificationNumber();
	}

	return notification_number;
}

double CompatUtility::GetServiceNotificationNotificationInterval(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	double notification_interval = -1;

	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
		if (notification_interval == -1 || notification->GetNotificationInterval() < notification_interval)
			notification_interval = notification->GetNotificationInterval();
	}

	if (notification_interval == -1)
		notification_interval = 60;

	return notification_interval / 60.0;
}

String CompatUtility::GetServiceNotificationNotificationPeriod(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	TimePeriod::Ptr notification_period;

	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {

		if (notification->GetNotificationPeriod())
			notification_period = notification->GetNotificationPeriod();
	}

	if (!notification_period)
		return Empty;

	return notification_period->GetName();
}

String CompatUtility::GetServiceNotificationNotificationOptions(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	unsigned long notification_type_filter = 0;
	unsigned long notification_state_filter = 0;

	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
		if (notification->GetNotificationTypeFilter())
			notification_type_filter = notification->GetNotificationTypeFilter();

		if (notification->GetNotificationStateFilter())
			notification_state_filter = notification->GetNotificationStateFilter();
	}

	std::vector<String> notification_options;

	/* notification state filters */
	if (notification_state_filter & (1<<StateWarning)) {
		notification_options.push_back("w");
	}
	if (notification_state_filter & (1<<StateUnknown)) {
		notification_options.push_back("u");
	}
	if (notification_state_filter & (1<<StateCritical)) {
		notification_options.push_back("c");
	}

	/* notification type filters */
	if (notification_type_filter & (1<<NotificationRecovery)) {
		notification_options.push_back("r");
	}
	if (notification_type_filter & (1<<NotificationFlappingStart) ||
			notification_type_filter & (1<<NotificationFlappingEnd)) {
		notification_options.push_back("f");
	}
	if (notification_type_filter & (1<<NotificationDowntimeStart) ||
			notification_type_filter & (1<<NotificationDowntimeEnd) ||
			notification_type_filter & (1<<NotificationDowntimeRemoved)) {
		notification_options.push_back("s");
	}

	return boost::algorithm::join(notification_options, ",");
}

int CompatUtility::GetServiceNotificationTypeFilter(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	unsigned long notification_type_filter = 0;

	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
		ObjectLock olock(notification);

		if (notification->GetNotificationTypeFilter())
			notification_type_filter = notification->GetNotificationTypeFilter();
	}

	return notification_type_filter;
}

int CompatUtility::GetServiceNotificationStateFilter(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	unsigned long notification_state_filter = 0;

	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
		ObjectLock olock(notification);

		if (notification->GetNotificationStateFilter())
			notification_state_filter = notification->GetNotificationStateFilter();
	}

	return notification_state_filter;
}

int CompatUtility::GetServiceNotifyOnWarning(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	if (GetServiceNotificationStateFilter(service) & (1<<StateWarning))
		return 1;

	return 0;
}

int CompatUtility::GetServiceNotifyOnCritical(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	if (GetServiceNotificationStateFilter(service) & (1<<StateCritical))
		return 1;

	return 0;
}

int CompatUtility::GetServiceNotifyOnUnknown(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	if (GetServiceNotificationStateFilter(service) & (1<<StateUnknown))
		return 1;

	return 0;
}

int CompatUtility::GetServiceNotifyOnRecovery(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	if (GetServiceNotificationTypeFilter(service) & (1<<NotificationRecovery))
		return 1;

	return 0;
}

int CompatUtility::GetServiceNotifyOnFlapping(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	unsigned long notification_type_filter = GetServiceNotificationTypeFilter(service);

	if (notification_type_filter & (1<<NotificationFlappingStart) ||
	    notification_type_filter & (1<<NotificationFlappingEnd))
		return 1;

	return 0;
}

int CompatUtility::GetServiceNotifyOnDowntime(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	unsigned long notification_type_filter = GetServiceNotificationTypeFilter(service);

	if (notification_type_filter & (1<<NotificationDowntimeStart) ||
	    notification_type_filter & (1<<NotificationDowntimeEnd) ||
	    notification_type_filter & (1<<NotificationDowntimeRemoved))
		return 1;

	return 0;
}

std::set<User::Ptr> CompatUtility::GetServiceNotificationUsers(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	/* Service -> Notifications -> (Users + UserGroups -> Users) */
	std::set<User::Ptr> allUsers;
	std::set<User::Ptr> users;

	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
		ObjectLock olock(notification);

		users = notification->GetUsers();

		std::copy(users.begin(), users.end(), std::inserter(allUsers, allUsers.begin()));

		BOOST_FOREACH(const UserGroup::Ptr& ug, notification->GetUserGroups()) {
			std::set<User::Ptr> members = ug->GetMembers();
			std::copy(members.begin(), members.end(), std::inserter(allUsers, allUsers.begin()));
		}
	}

	return allUsers;
}

std::set<UserGroup::Ptr> CompatUtility::GetServiceNotificationUserGroups(const Service::Ptr& service)
{
	ASSERT(service->OwnsLock());

	std::set<UserGroup::Ptr> usergroups;
	/* Service -> Notifications -> UserGroups */
	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
		ObjectLock olock(notification);

		BOOST_FOREACH(const UserGroup::Ptr& ug, notification->GetUserGroups()) {
			usergroups.insert(ug);
		}
	}

	return usergroups;
}

String CompatUtility::GetCheckResultOutput(const CheckResult::Ptr& cr)
{
	if (!cr)
		return Empty;

	String output;

	String raw_output = cr->GetOutput();

	/*
	 * replace semi-colons with colons in output
	 * semi-colon is used as delimiter in various interfaces
	 */
	boost::algorithm::replace_all(raw_output, ";", ":");

	size_t line_end = raw_output.Find("\n");

	return raw_output.SubStr(0, line_end);
}

String CompatUtility::GetCheckResultLongOutput(const CheckResult::Ptr& cr)
{
	if (!cr)
		return Empty;

	String long_output;
	String output;

	String raw_output = cr->GetOutput();

	/*
	 * replace semi-colons with colons in output
	 * semi-colon is used as delimiter in various interfaces
	 */
	boost::algorithm::replace_all(raw_output, ";", ":");

	size_t line_end = raw_output.Find("\n");

	if (line_end > 0 && line_end != String::NPos) {
		long_output = raw_output.SubStr(line_end+1, raw_output.GetLength());
		return EscapeString(long_output);
	}

	return Empty;
}

String CompatUtility::GetCheckResultPerfdata(const CheckResult::Ptr& cr)
{
	if (!cr)
		return String();

	return PluginUtility::FormatPerfdata(cr->GetPerformanceData());
}

String CompatUtility::EscapeString(const String& str)
{
	String result = str;
	boost::algorithm::replace_all(result, "\n", "\\n");
	return result;
}

std::pair<unsigned long, unsigned long> CompatUtility::ConvertTimestamp(double time)
{
	unsigned long time_sec = static_cast<long>(time);
	unsigned long time_usec = (time - time_sec) * 1000 * 1000;

	return std::make_pair(time_sec, time_usec);
}

int CompatUtility::MapNotificationReasonType(NotificationType type)
{
	switch (type) {
		case NotificationDowntimeStart:
			return 5;
		case NotificationDowntimeEnd:
			return 6;
		case NotificationDowntimeRemoved:
			return 7;
		case NotificationCustom:
			return 8;
		case NotificationAcknowledgement:
			return 1;
		case NotificationProblem:
			return 0;
		case NotificationRecovery:
			return 0;
		case NotificationFlappingStart:
			return 2;
		case NotificationFlappingEnd:
			return 3;
		default:
			return 0;
	}
}

int CompatUtility::MapExternalCommandType(const String& name)
{
	if (name == "NONE")
		return 0;
	if (name == "ADD_HOST_COMMENT")
		return 1;
	if (name == "DEL_HOST_COMMENT")
		return 2;
	if (name == "ADD_SVC_COMMENT")
		return 3;
	if (name == "DEL_SVC_COMMENT")
		return 4;
	if (name == "ENABLE_SVC_CHECK")
		return 5;
	if (name == "DISABLE_SVC_CHECK")
		return 6;
	if (name == "SCHEDULE_SVC_CHECK")
		return 7;
	if (name == "DELAY_SVC_NOTIFICATION")
		return 9;
	if (name == "DELAY_HOST_NOTIFICATION")
		return 10;
	if (name == "DISABLE_NOTIFICATIONS")
		return 11;
	if (name == "ENABLE_NOTIFICATIONS")
		return 12;
	if (name == "RESTART_PROCESS")
		return 13;
	if (name == "SHUTDOWN_PROCESS")
		return 14;
	if (name == "ENABLE_HOST_SVC_CHECKS")
		return 15;
	if (name == "DISABLE_HOST_SVC_CHECKS")
		return 16;
	if (name == "SCHEDULE_HOST_SVC_CHECKS")
		return 17;
	if (name == "DELAY_HOST_SVC_NOTIFICATIONS")
		return 19;
	if (name == "DEL_ALL_HOST_COMMENTS")
		return 20;
	if (name == "DEL_ALL_SVC_COMMENTS")
		return 21;
	if (name == "ENABLE_SVC_NOTIFICATIONS")
		return 22;
	if (name == "DISABLE_SVC_NOTIFICATIONS")
		return 23;
	if (name == "ENABLE_HOST_NOTIFICATIONS")
		return 24;
	if (name == "DISABLE_HOST_NOTIFICATIONS")
		return 25;
	if (name == "ENABLE_ALL_NOTIFICATIONS_BEYOND_HOST")
		return 26;
	if (name == "DISABLE_ALL_NOTIFICATIONS_BEYOND_HOST")
		return 27;
	if (name == "ENABLE_HOST_SVC_NOTIFICATIONS")
		return 28;
	if (name == "DISABLE_HOST_SVC_NOTIFICATIONS")
		return 29;
	if (name == "PROCESS_SERVICE_CHECK_RESULT")
		return 30;
	if (name == "SAVE_STATE_INFORMATION")
		return 31;
	if (name == "READ_STATE_INFORMATION")
		return 32;
	if (name == "ACKNOWLEDGE_HOST_PROBLEM")
		return 33;
	if (name == "ACKNOWLEDGE_SVC_PROBLEM")
		return 34;
	if (name == "START_EXECUTING_SVC_CHECKS")
		return 35;
	if (name == "STOP_EXECUTING_SVC_CHECKS")
		return 36;
	if (name == "START_ACCEPTING_PASSIVE_SVC_CHECKS")
		return 37;
	if (name == "STOP_ACCEPTING_PASSIVE_SVC_CHECKS")
		return 38;
	if (name == "ENABLE_PASSIVE_SVC_CHECKS")
		return 39;
	if (name == "DISABLE_PASSIVE_SVC_CHECKS")
		return 40;
	if (name == "ENABLE_EVENT_HANDLERS")
		return 41;
	if (name == "DISABLE_EVENT_HANDLERS")
		return 42;
	if (name == "ENABLE_HOST_EVENT_HANDLER")
		return 43;
	if (name == "DISABLE_HOST_EVENT_HANDLER")
		return 44;
	if (name == "ENABLE_SVC_EVENT_HANDLER")
		return 45;
	if (name == "DISABLE_SVC_EVENT_HANDLER")
		return 46;
	if (name == "ENABLE_HOST_CHECK")
		return 47;
	if (name == "DISABLE_HOST_CHECK")
		return 48;
	if (name == "START_OBSESSING_OVER_SVC_CHECKS")
		return 49;
	if (name == "STOP_OBSESSING_OVER_SVC_CHECKS")
		return 50;
	if (name == "REMOVE_HOST_ACKNOWLEDGEMENT")
		return 51;
	if (name == "REMOVE_SVC_ACKNOWLEDGEMENT")
		return 52;
	if (name == "SCHEDULE_FORCED_HOST_SVC_CHECKS")
		return 53;
	if (name == "SCHEDULE_FORCED_SVC_CHECK")
		return 54;
	if (name == "SCHEDULE_HOST_DOWNTIME")
		return 55;
	if (name == "SCHEDULE_SVC_DOWNTIME")
		return 56;
	if (name == "ENABLE_HOST_FLAP_DETECTION")
		return 57;
	if (name == "DISABLE_HOST_FLAP_DETECTION")
		return 58;
	if (name == "ENABLE_SVC_FLAP_DETECTION")
		return 59;
	if (name == "DISABLE_SVC_FLAP_DETECTION")
		return 60;
	if (name == "ENABLE_FLAP_DETECTION")
		return 61;
	if (name == "DISABLE_FLAP_DETECTION")
		return 62;
	if (name == "ENABLE_HOSTGROUP_SVC_NOTIFICATIONS")
		return 63;
	if (name == "DISABLE_HOSTGROUP_SVC_NOTIFICATIONS")
		return 64;
	if (name == "ENABLE_HOSTGROUP_HOST_NOTIFICATIONS")
		return 65;
	if (name == "DISABLE_HOSTGROUP_HOST_NOTIFICATIONS")
		return 66;
	if (name == "ENABLE_HOSTGROUP_SVC_CHECKS")
		return 67;
	if (name == "DISABLE_HOSTGROUP_SVC_CHECKS")
		return 68;
	if (name == "CANCEL_HOST_DOWNTIME")
		return 69;
	if (name == "CANCEL_SVC_DOWNTIME")
		return 70;
	if (name == "CANCEL_ACTIVE_HOST_DOWNTIME")
		return 71;
	if (name == "CANCEL_PENDING_HOST_DOWNTIME")
		return 72;
	if (name == "CANCEL_ACTIVE_SVC_DOWNTIME")
		return 73;
	if (name == "CANCEL_PENDING_SVC_DOWNTIME")
		return 74;
	if (name == "CANCEL_ACTIVE_HOST_SVC_DOWNTIME")
		return 75;
	if (name == "CANCEL_PENDING_HOST_SVC_DOWNTIME")
		return 76;
	if (name == "FLUSH_PENDING_COMMANDS")
		return 77;
	if (name == "DEL_HOST_DOWNTIME")
		return 78;
	if (name == "DEL_SVC_DOWNTIME")
		return 79;
	if (name == "ENABLE_FAILURE_PREDICTION")
		return 80;
	if (name == "DISABLE_FAILURE_PREDICTION")
		return 81;
	if (name == "ENABLE_PERFORMANCE_DATA")
		return 82;
	if (name == "DISABLE_PERFORMANCE_DATA")
		return 83;
	if (name == "SCHEDULE_HOSTGROUP_HOST_DOWNTIME")
		return 84;
	if (name == "SCHEDULE_HOSTGROUP_SVC_DOWNTIME")
		return 85;
	if (name == "SCHEDULE_HOST_SVC_DOWNTIME")
		return 86;
	if (name == "PROCESS_HOST_CHECK_RESULT")
		return 87;
	if (name == "START_EXECUTING_HOST_CHECKS")
		return 88;
	if (name == "STOP_EXECUTING_HOST_CHECKS")
		return 89;
	if (name == "START_ACCEPTING_PASSIVE_HOST_CHECKS")
		return 90;
	if (name == "STOP_ACCEPTING_PASSIVE_HOST_CHECKS")
		return 91;
	if (name == "ENABLE_PASSIVE_HOST_CHECKS")
		return 92;
	if (name == "DISABLE_PASSIVE_HOST_CHECKS")
		return 93;
	if (name == "START_OBSESSING_OVER_HOST_CHECKS")
		return 94;
	if (name == "STOP_OBSESSING_OVER_HOST_CHECKS")
		return 95;
	if (name == "SCHEDULE_HOST_CHECK")
		return 96;
	if (name == "SCHEDULE_FORCED_HOST_CHECK")
		return 98;
	if (name == "START_OBSESSING_OVER_SVC")
		return 99;
	if (name == "STOP_OBSESSING_OVER_SVC")
		return 100;
	if (name == "START_OBSESSING_OVER_HOST")
		return 101;
	if (name == "STOP_OBSESSING_OVER_HOST")
		return 102;
	if (name == "ENABLE_HOSTGROUP_HOST_CHECKS")
		return 103;
	if (name == "DISABLE_HOSTGROUP_HOST_CHECKS")
		return 104;
	if (name == "ENABLE_HOSTGROUP_PASSIVE_SVC_CHECKS")
		return 105;
	if (name == "DISABLE_HOSTGROUP_PASSIVE_SVC_CHECKS")
		return 106;
	if (name == "ENABLE_HOSTGROUP_PASSIVE_HOST_CHECKS")
		return 107;
	if (name == "DISABLE_HOSTGROUP_PASSIVE_HOST_CHECKS")
		return 108;
	if (name == "ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS")
		return 109;
	if (name == "DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS")
		return 110;
	if (name == "ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS")
		return 111;
	if (name == "DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS")
		return 112;
	if (name == "ENABLE_SERVICEGROUP_SVC_CHECKS")
		return 113;
	if (name == "DISABLE_SERVICEGROUP_SVC_CHECKS")
		return 114;
	if (name == "ENABLE_SERVICEGROUP_HOST_CHECKS")
		return 115;
	if (name == "DISABLE_SERVICEGROUP_HOST_CHECKS")
		return 116;
	if (name == "ENABLE_SERVICEGROUP_PASSIVE_SVC_CHECKS")
		return 117;
	if (name == "DISABLE_SERVICEGROUP_PASSIVE_SVC_CHECKS")
		return 118;
	if (name == "ENABLE_SERVICEGROUP_PASSIVE_HOST_CHECKS")
		return 119;
	if (name == "DISABLE_SERVICEGROUP_PASSIVE_HOST_CHECKS")
		return 120;
	if (name == "SCHEDULE_SERVICEGROUP_HOST_DOWNTIME")
		return 121;
	if (name == "SCHEDULE_SERVICEGROUP_SVC_DOWNTIME")
		return 122;
	if (name == "CHANGE_GLOBAL_HOST_EVENT_HANDLER")
		return 123;
	if (name == "CHANGE_GLOBAL_SVC_EVENT_HANDLER")
		return 124;
	if (name == "CHANGE_HOST_EVENT_HANDLER")
		return 125;
	if (name == "CHANGE_SVC_EVENT_HANDLER")
		return 126;
	if (name == "CHANGE_HOST_CHECK_COMMAND")
		return 127;
	if (name == "CHANGE_SVC_CHECK_COMMAND")
		return 128;
	if (name == "CHANGE_NORMAL_HOST_CHECK_INTERVAL")
		return 129;
	if (name == "CHANGE_NORMAL_SVC_CHECK_INTERVAL")
		return 130;
	if (name == "CHANGE_RETRY_SVC_CHECK_INTERVAL")
		return 131;
	if (name == "CHANGE_MAX_HOST_CHECK_ATTEMPTS")
		return 132;
	if (name == "CHANGE_MAX_SVC_CHECK_ATTEMPTS")
		return 133;
	if (name == "SCHEDULE_AND_PROPAGATE_TRIGGERED_HOST_DOWNTIME")
		return 134;
	if (name == "ENABLE_HOST_AND_CHILD_NOTIFICATIONS")
		return 135;
	if (name == "DISABLE_HOST_AND_CHILD_NOTIFICATIONS")
		return 136;
	if (name == "SCHEDULE_AND_PROPAGATE_HOST_DOWNTIME")
		return 137;
	if (name == "ENABLE_SERVICE_FRESHNESS_CHECKS")
		return 138;
	if (name == "DISABLE_SERVICE_FRESHNESS_CHECKS")
		return 139;
	if (name == "ENABLE_HOST_FRESHNESS_CHECKS")
		return 140;
	if (name == "DISABLE_HOST_FRESHNESS_CHECKS")
		return 141;
	if (name == "SET_HOST_NOTIFICATION_NUMBER")
		return 142;
	if (name == "SET_SVC_NOTIFICATION_NUMBER")
		return 143;
	if (name == "CHANGE_HOST_CHECK_TIMEPERIOD")
		return 144;
	if (name == "CHANGE_SVC_CHECK_TIMEPERIOD")
		return 145;
	if (name == "PROCESS_FILE")
		return 146;
	if (name == "CHANGE_CUSTOM_HOST_VAR")
		return 147;
	if (name == "CHANGE_CUSTOM_SVC_VAR")
		return 148;
	if (name == "CHANGE_CUSTOM_CONTACT_VAR")
		return 149;
	if (name == "ENABLE_CONTACT_HOST_NOTIFICATIONS")
		return 150;
	if (name == "DISABLE_CONTACT_HOST_NOTIFICATIONS")
		return 151;
	if (name == "ENABLE_CONTACT_SVC_NOTIFICATIONS")
		return 152;
	if (name == "DISABLE_CONTACT_SVC_NOTIFICATIONS")
		return 153;
	if (name == "ENABLE_CONTACTGROUP_HOST_NOTIFICATIONS")
		return 154;
	if (name == "DISABLE_CONTACTGROUP_HOST_NOTIFICATIONS")
		return 155;
	if (name == "ENABLE_CONTACTGROUP_SVC_NOTIFICATIONS")
		return 156;
	if (name == "DISABLE_CONTACTGROUP_SVC_NOTIFICATIONS")
		return 157;
	if (name == "CHANGE_RETRY_HOST_CHECK_INTERVAL")
		return 158;
	if (name == "SEND_CUSTOM_HOST_NOTIFICATION")
		return 159;
	if (name == "SEND_CUSTOM_SVC_NOTIFICATION")
		return 160;
	if (name == "CHANGE_HOST_NOTIFICATION_TIMEPERIOD")
		return 161;
	if (name == "CHANGE_SVC_NOTIFICATION_TIMEPERIOD")
		return 162;
	if (name == "CHANGE_CONTACT_HOST_NOTIFICATION_TIMEPERIOD")
		return 163;
	if (name == "CHANGE_CONTACT_SVC_NOTIFICATION_TIMEPERIOD")
		return 164;
	if (name == "CHANGE_HOST_MODATTR")
		return 165;
	if (name == "CHANGE_SVC_MODATTR")
		return 166;
	if (name == "CHANGE_CONTACT_MODATTR")
		return 167;
	if (name == "CHANGE_CONTACT_MODHATTR")
		return 168;
	if (name == "CHANGE_CONTACT_MODSATTR")
		return 169;
	if (name == "SYNC_STATE_INFORMATION")
		return 170;
	if (name == "DEL_DOWNTIME_BY_HOST_NAME")
		return 171;
	if (name == "DEL_DOWNTIME_BY_HOSTGROUP_NAME")
		return 172;
	if (name == "DEL_DOWNTIME_BY_START_TIME_COMMENT")
		return 173;
	if (name == "ACKNOWLEDGE_HOST_PROBLEM_EXPIRE")
		return 174;
	if (name == "ACKNOWLEDGE_SVC_PROBLEM_EXPIRE")
		return 175;
	if (name == "DISABLE_NOTIFICATIONS_EXPIRE_TIME")
		return 176;
	if (name == "CUSTOM_COMMAND")
		return 999;

	return 0;
}

