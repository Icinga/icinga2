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

#include "base/convert.h"
#include "icinga/compatutility.h"
#include "icinga/checkcommand.h"
#include "icinga/eventcommand.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/debug.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>

using namespace icinga;

Dictionary::Ptr CompatUtility::GetHostConfigAttributes(const Host::Ptr& host)
{
	Dictionary::Ptr attr = boost::make_shared<Dictionary>();
	Dictionary::Ptr service_attr = boost::make_shared<Dictionary>();

	ASSERT(host->OwnsLock());

	/* host config attributes */
	Dictionary::Ptr custom;
	Dictionary::Ptr macros;
	std::vector<String> notification_options;

	/* dicts */
	macros = host->GetMacros();
	custom = host->GetCustom();

	if (macros) {
		attr->Set("address", macros->Get("address"));
		attr->Set("address6", macros->Get("address6"));
	}

	if (custom) {
		attr->Set("notes", custom->Get("notes"));
		attr->Set("notes_url", custom->Get("notes_url"));
		attr->Set("action_url", custom->Get("action_url"));
		attr->Set("icon_image", custom->Get("icon_image"));
		attr->Set("icon_image_alt", custom->Get("icon_image_alt"));

		attr->Set("statusmap_image", custom->Get("statusmap_image"));
		attr->Set("2d_coords", custom->Get("2d_coords"));

		if (!custom->Get("2d_coords").IsEmpty()) {
			std::vector<String> tokens;
			String coords = custom->Get("2d_coords");
			boost::algorithm::split(tokens, coords, boost::is_any_of(","));

			if (tokens.size() == 2) {
				attr->Set("have_2d_coords", 1);
				attr->Set("x_2d", tokens[0]);
				attr->Set("y_2d", tokens[1]);
			}
			else
				attr->Set("have_2d_coords", 0);
		}
		else
			attr->Set("have_2d_coords", 0);

		/* deprecated in 1.x, but empty */
		attr->Set("vrml_image", Empty);
		attr->Set("3d_coords", Empty);
		attr->Set("have_3d_coords", 0);
		attr->Set("x_3d", Empty);
		attr->Set("y_3d", Empty);
		attr->Set("z_3d", Empty);
	}

	/* alias */
	if (!host->GetDisplayName().IsEmpty())
		attr->Set("alias", host->GetName());
	else
		attr->Set("alias", host->GetDisplayName());

	/* get additonal attributes from check service */
	Service::Ptr service = host->GetCheckService();

	if (service) {
		unsigned long notification_type_filter = 0;
		unsigned long notification_state_filter = 0;

		ObjectLock olock(service);

		BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
			if (notification->GetNotificationTypeFilter())
				notification_type_filter = notification->GetNotificationTypeFilter();

			if (notification->GetNotificationStateFilter())
				notification_state_filter = notification->GetNotificationStateFilter();
		}

		/* notification state filters */
		if (notification_state_filter & (1<<StateWarning) ||
		    notification_state_filter & (1<<StateCritical)) {
			attr->Set("notify_on_down", 1);
			notification_options.push_back("d");
		}

		/* notification type filters */
		if (notification_type_filter & (1<<NotificationRecovery)) {
			attr->Set("notify_on_recovery", 1);
			notification_options.push_back("r");
		}
		if (notification_type_filter & (1<<NotificationFlappingStart) ||
		    notification_type_filter & (1<<NotificationFlappingEnd)) {
			attr->Set("notify_on_flapping", 1);
			notification_options.push_back("f");
		}
		if (notification_type_filter & (1<<NotificationDowntimeStart) ||
		    notification_type_filter & (1<<NotificationDowntimeEnd) ||
		    notification_type_filter & (1<<NotificationDowntimeRemoved)) {
			attr->Set("notify_on_downtime", 1);
			notification_options.push_back("s");
		}

		service_attr = CompatUtility::GetServiceConfigAttributes(service);

		attr->Set("check_period", service_attr->Get("check_period"));
		attr->Set("check_interval", service_attr->Get("check_interval"));
		attr->Set("retry_interval", service_attr->Get("retry_interval"));
		attr->Set("max_check_attempts", service_attr->Get("max_check_attempts"));
		attr->Set("active_checks_enabled", service_attr->Get("active_checks_enabled"));
		attr->Set("passive_checks_enabled", service_attr->Get("passive_checks_enabled"));
		attr->Set("flap_detection_enabled", service_attr->Get("flap_detection_enabled"));
		attr->Set("low_flap_threshold", service_attr->Get("low_flap_threshold"));
		attr->Set("high_flap_threshold", service_attr->Get("high_flap_threshold"));
		attr->Set("notifications_enabled", service_attr->Get("notifications_enabled"));
		attr->Set("eventhandler_enabled", service_attr->Get("eventhandler_enabled"));
		attr->Set("is_volatile", service_attr->Get("is_volatile"));
		attr->Set("notifications_enabled", service_attr->Get("notifications_enabled"));
		attr->Set("notification_options", boost::algorithm::join(notification_options, ","));
		attr->Set("notification_interval", service_attr->Get("notification_interval"));
		attr->Set("process_performance_data", service_attr->Get("process_performance_data"));
		attr->Set("notification_period", service_attr->Get("notification_period"));
		attr->Set("check_freshness", service_attr->Get("check_freshness"));
		attr->Set("check_command", service_attr->Get("check_command"));
		attr->Set("event_handler", service_attr->Get("event_handler"));
	}

	return attr;
}

Dictionary::Ptr CompatUtility::GetServiceStatusAttributes(const Service::Ptr& service, CompatObjectType type)
{
	Dictionary::Ptr attr = boost::make_shared<Dictionary>();

	ASSERT(service->OwnsLock());

	String raw_output;
	String output;
	String long_output;
	String perfdata;
	String check_source;
	double schedule_end = -1;

	String check_period_str;
	TimePeriod::Ptr check_period = service->GetCheckPeriod();
	if (check_period)
		check_period_str = check_period->GetName();
	else
		check_period_str = "24x7";

	Dictionary::Ptr cr = service->GetLastCheckResult();

	if (cr) {
		Dictionary::Ptr output_bag = GetCheckResultOutput(cr);
		output = output_bag->Get("output");
		long_output = output_bag->Get("long_output");

		check_source = cr->Get("check_source");

		perfdata = GetCheckResultPerfdata(cr);
		schedule_end = cr->Get("schedule_end");
	}

	int state = service->GetState();

	if (state > StateUnknown)
		state = StateUnknown;

	if (type == CompatTypeHost) {
		if (state == StateOK || state == StateWarning)
			state = 0; /* UP */
		else
			state = 1; /* DOWN */

		Host::Ptr host = service->GetHost();

		ASSERT(host);

		if (!host->IsReachable())
			state = 2; /* UNREACHABLE */

		attr->Set("last_time_up", host->GetLastStateUp());
		attr->Set("last_time_down", host->GetLastStateDown());
		attr->Set("last_time_unreachable", host->GetLastStateUnreachable());
	} else {
		attr->Set("last_time_ok", service->GetLastStateOK());
		attr->Set("last_time_warn", service->GetLastStateWarning());
		attr->Set("last_time_critical", service->GetLastStateCritical());
		attr->Set("last_time_unknown", service->GetLastStateUnknown());
	}

	double last_notification = 0;
	double next_notification = 0;
	int notification_number = 0;
	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
		if (notification->GetLastNotification() > last_notification)
			last_notification = notification->GetLastNotification();

		if (notification->GetNextNotification() < next_notification)
			next_notification = notification->GetNextNotification();

		if (notification->GetNotificationNumber() > notification_number)
			notification_number = notification->GetNotificationNumber();
	}

	CheckCommand::Ptr checkcommand = service->GetCheckCommand();
	if (checkcommand)
		attr->Set("check_command", "check_" + checkcommand->GetName());

	EventCommand::Ptr eventcommand = service->GetEventCommand();
	if (eventcommand)
		attr->Set("event_handler", "event_" + eventcommand->GetName());

	attr->Set("check_period", check_period_str);
	attr->Set("check_interval", service->GetCheckInterval() / 60.0);
	attr->Set("retry_interval", service->GetRetryInterval() / 60.0);
	attr->Set("has_been_checked", (service->GetLastCheckResult() ? 1 : 0));
	attr->Set("should_be_scheduled", 1);
	attr->Set("check_execution_time", Service::CalculateExecutionTime(cr));
	attr->Set("check_latency", Service::CalculateLatency(cr));
	attr->Set("current_state", state);
	attr->Set("state_type", service->GetStateType());
	attr->Set("plugin_output", output);
	attr->Set("long_plugin_output", long_output);
	attr->Set("performance_data", perfdata);
	attr->Set("check_source", check_source);
	attr->Set("check_type", (service->GetEnableActiveChecks() ? 0 : 1));
	attr->Set("last_check", schedule_end);
	attr->Set("next_check", service->GetNextCheck());
	attr->Set("current_attempt", service->GetCurrentCheckAttempt());
	attr->Set("max_attempts", service->GetMaxCheckAttempts());
	attr->Set("last_state_change", service->GetLastStateChange());
	attr->Set("last_hard_state_change", service->GetLastHardStateChange());
	attr->Set("last_update", static_cast<long>(time(NULL)));
	attr->Set("process_performance_data", 1); /* always enabled */
	attr->Set("freshness_checks_enabled", 1); /* always enabled */
	attr->Set("notifications_enabled", (service->GetEnableNotifications() ? 1 : 0));
	attr->Set("event_handler_enabled", 1); /* always enabled */
	attr->Set("active_checks_enabled", (service->GetEnableActiveChecks() ? 1 : 0));
	attr->Set("passive_checks_enabled", (service->GetEnablePassiveChecks() ? 1 : 0));
	attr->Set("flap_detection_enabled", (service->GetEnableFlapping() ? 1 : 0));
	attr->Set("is_flapping", (service->IsFlapping() ? 1 : 0));
	attr->Set("percent_state_change", Convert::ToString(service->GetFlappingCurrent()));
	attr->Set("problem_has_been_acknowledged", (service->GetAcknowledgement() != AcknowledgementNone ? 1 : 0));
	attr->Set("acknowledgement_type", static_cast<int>(service->GetAcknowledgement()));
	attr->Set("acknowledgement_end_time", service->GetAcknowledgementExpiry());
	attr->Set("scheduled_downtime_depth", (service->IsInDowntime() ? 1 : 0));
	attr->Set("last_notification", last_notification);
	attr->Set("next_notification", next_notification);
	attr->Set("current_notification_number", notification_number);
	attr->Set("modified_attributes", service->GetModifiedAttributes());

	return attr;
}

Dictionary::Ptr CompatUtility::GetServiceConfigAttributes(const Service::Ptr& service)
{
	Dictionary::Ptr attr = boost::make_shared<Dictionary>();

	ASSERT(service->OwnsLock());

	Host::Ptr host = service->GetHost();

	if (!host)
		return Dictionary::Ptr();

	String check_period_str;
	TimePeriod::Ptr check_period = service->GetCheckPeriod();
	if (check_period)
		check_period_str = check_period->GetName();
	else
		check_period_str = "24x7";

	double notification_interval = -1;
	String notification_period;
	unsigned long notification_type_filter = 0;
	unsigned long notification_state_filter = 0;

	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
		if (notification_interval == -1 || notification->GetNotificationInterval() < notification_interval)
			notification_interval = notification->GetNotificationInterval();

		if (notification->GetNotificationPeriod())
			notification_period = notification->GetNotificationPeriod()->GetName();

		if (notification->GetNotificationTypeFilter())
			notification_type_filter = notification->GetNotificationTypeFilter();

		if (notification->GetNotificationStateFilter())
			notification_state_filter = notification->GetNotificationStateFilter();

		Log(LogDebug, "icinga", "notification_type_filter: " + Convert::ToString(notification_type_filter) + " notification_state_filter: " + Convert::ToString(notification_state_filter));
	}


	if (notification_interval == -1)
		notification_interval = 60;

	String check_command_str;
	String event_command_str;
	CheckCommand::Ptr checkcommand = service->GetCheckCommand();
	EventCommand::Ptr eventcommand = service->GetEventCommand();

	if (checkcommand)
		check_command_str = checkcommand->GetName();

	if (eventcommand)
		event_command_str = eventcommand->GetName();

	Dictionary::Ptr custom;
	Dictionary::Ptr macros;
	std::vector<String> notification_options;

	/* dicts */
	custom = service->GetCustom();
	macros = service->GetMacros();

	/* notification state filters */
	if (notification_state_filter & (1<<StateWarning)) {
		attr->Set("notify_on_warning", 1);
		notification_options.push_back("w");
	}
	if (notification_state_filter & (1<<StateUnknown)) {
		attr->Set("notify_on_unknown", 1);
		notification_options.push_back("u");
	}
	if (notification_state_filter & (1<<StateCritical)) {
		attr->Set("notify_on_critical", 1);
		notification_options.push_back("c");
	}

	/* notification type filters */
	if (notification_type_filter & (1<<NotificationRecovery)) {
		attr->Set("notify_on_recovery", 1);
		notification_options.push_back("r");
	}
	if (notification_type_filter & (1<<NotificationFlappingStart) ||
			notification_type_filter & (1<<NotificationFlappingEnd)) {
		attr->Set("notify_on_flapping", 1);
		notification_options.push_back("f");
	}
	if (notification_type_filter & (1<<NotificationDowntimeStart) ||
			notification_type_filter & (1<<NotificationDowntimeEnd) ||
			notification_type_filter & (1<<NotificationDowntimeRemoved)) {
		attr->Set("notify_on_downtime", 1);
		notification_options.push_back("s");
	}

	attr->Set("check_period", check_period_str);
	attr->Set("check_interval", service->GetCheckInterval() / 60.0);
	attr->Set("retry_interval", service->GetRetryInterval() / 60.0);
	attr->Set("max_check_attempts", service->GetMaxCheckAttempts());
	attr->Set("active_checks_enabled", (service->GetEnableActiveChecks() ? 1 : 0));
	attr->Set("passive_checks_enabled", (service->GetEnablePassiveChecks() ? 1 : 0));
	attr->Set("flap_detection_enabled", (service->GetEnableFlapping() ? 1 : 0));
	attr->Set("low_flap_threshold", service->GetFlappingThreshold());
	attr->Set("high_flap_threshold", service->GetFlappingThreshold());
	attr->Set("notifications_enabled", (service->GetEnableNotifications() ? 1 : 0));
	attr->Set("eventhandler_enabled", 1); /* always 1 */
	attr->Set("is_volatile", (service->IsVolatile() ? 1 : 0));
	attr->Set("notifications_enabled", (service->GetEnableNotifications() ? 1 : 0));
	attr->Set("notification_options", boost::algorithm::join(notification_options, ","));
	attr->Set("notification_interval", notification_interval / 60.0);
	attr->Set("process_performance_data", 1); /* always 1 */
	attr->Set("notification_period", notification_period);
	attr->Set("check_freshness", 1); /* always 1 */
	attr->Set("check_command", check_command_str);
	attr->Set("event_handler", event_command_str);

	/* custom attr */
	if (custom) {
		attr->Set("notes", custom->Get("notes"));
		attr->Set("notes_url", custom->Get("notes_url"));
		attr->Set("action_url", custom->Get("action_url"));
		attr->Set("icon_image", custom->Get("icon_image"));
		attr->Set("icon_image_alt", custom->Get("icon_image_alt"));
	}

	return attr;

}

Dictionary::Ptr CompatUtility::GetCommandConfigAttributes(const Command::Ptr& command)
{
	Dictionary::Ptr attr = boost::make_shared<Dictionary>();

	Value commandLine = command->GetCommandLine();

	String commandline;
	if (commandLine.IsObjectType<Array>()) {
		Array::Ptr args = commandLine;

		ObjectLock olock(args);
		String arg;
		BOOST_FOREACH(arg, args) {
			// This is obviously incorrect for non-trivial cases.
			commandline = " \"" + EscapeString(arg) + "\"";
		}
	} else if (!commandLine.IsEmpty()) {
		commandline = EscapeString(Convert::ToString(commandLine));
	} else {
		commandline = "<internal>";
	}

	attr->Set("command_line", commandline);

	return attr;
}


Dictionary::Ptr CompatUtility::GetCustomVariableConfig(const DynamicObject::Ptr& object)
{
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

	Dictionary::Ptr customvars = boost::make_shared<Dictionary>();

	if (!custom)
		return Dictionary::Ptr();

        ObjectLock olock(custom);
        String key;
        Value value;
        BOOST_FOREACH(boost::tie(key, value), custom) {

                if (key == "notes" ||
		    key == "action_url" ||
		    key == "notes_url" ||
		    key == "icon_image" ||
		    key == "icon_image_alt" ||
		    key == "statusmap_image" ||
		    key == "2d_coords")
                        continue;

		customvars->Set(key, value);
        }

	return customvars;
}

std::set<User::Ptr> CompatUtility::GetServiceNotificationUsers(const Service::Ptr& service)
{
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

Dictionary::Ptr CompatUtility::GetCheckResultOutput(const Dictionary::Ptr& cr)
{
	if (!cr)
		return Empty;

	String long_output;
	String output;
	Dictionary::Ptr bag = boost::make_shared<Dictionary>();

	String raw_output = cr->Get("output");
	size_t line_end = raw_output.Find("\n");

	output = raw_output.SubStr(0, line_end);

	if (line_end > 0 && line_end != String::NPos) {
		long_output = raw_output.SubStr(line_end+1, raw_output.GetLength());
		long_output = EscapeString(long_output);
	}

	output = EscapeString(output);

	bag->Set("output", output);
	bag->Set("long_output", long_output);
	return bag;
}

String CompatUtility::GetCheckResultPerfdata(const Dictionary::Ptr& cr)
{
	if (!cr)
		return Empty;

	String perfdata = EscapeString(cr->Get("performance_data_raw"));
	perfdata.Trim();

	return perfdata;
}

String CompatUtility::EscapeString(const String& str)
{
	String result = str;
	boost::algorithm::replace_all(result, "\n", "\\n");
	return result;
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

