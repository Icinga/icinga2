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

#include "livestatus/servicestable.h"
#include "livestatus/hoststable.h"
#include "icinga/service.h"
#include "icinga/checkcommand.h"
#include "icinga/eventcommand.h"
#include "icinga/timeperiod.h"
#include "icinga/macroprocessor.h"
#include "icinga/icingaapplication.h"
#include "icinga/compatutility.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/convert.h"
#include "base/utility.h"
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace icinga;
using namespace livestatus;

ServicesTable::ServicesTable(void)
{
	AddColumns(this);
}

void ServicesTable::AddColumns(Table *table, const String& prefix,
    const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "description", Column(&ServicesTable::ShortNameAccessor, objectAccessor));
	table->AddColumn(prefix + "display_name", Column(&ServicesTable::DisplayNameAccessor, objectAccessor));
	table->AddColumn(prefix + "check_command", Column(&ServicesTable::CheckCommandAccessor, objectAccessor));
	table->AddColumn(prefix + "check_command_expanded", Column(&ServicesTable::CheckCommandExpandedAccessor, objectAccessor));
	table->AddColumn(prefix + "event_handler", Column(&ServicesTable::EventHandlerAccessor, objectAccessor));
	table->AddColumn(prefix + "plugin_output", Column(&ServicesTable::PluginOutputAccessor, objectAccessor));
	table->AddColumn(prefix + "long_plugin_output", Column(&ServicesTable::LongPluginOutputAccessor, objectAccessor));
	table->AddColumn(prefix + "perf_data", Column(&ServicesTable::PerfDataAccessor, objectAccessor));
	table->AddColumn(prefix + "notification_period", Column(&ServicesTable::NotificationPeriodAccessor, objectAccessor));
	table->AddColumn(prefix + "check_period", Column(&ServicesTable::CheckPeriodAccessor, objectAccessor));
	table->AddColumn(prefix + "notes", Column(&ServicesTable::NotesAccessor, objectAccessor));
	table->AddColumn(prefix + "notes_expanded", Column(&ServicesTable::NotesExpandedAccessor, objectAccessor));
	table->AddColumn(prefix + "notes_url", Column(&ServicesTable::NotesUrlAccessor, objectAccessor));
	table->AddColumn(prefix + "notes_url_expanded", Column(&ServicesTable::NotesUrlExpandedAccessor, objectAccessor));
	table->AddColumn(prefix + "action_url", Column(&ServicesTable::ActionUrlAccessor, objectAccessor));
	table->AddColumn(prefix + "action_url_expanded", Column(&ServicesTable::ActionUrlExpandedAccessor, objectAccessor));
	table->AddColumn(prefix + "icon_image", Column(&ServicesTable::IconImageAccessor, objectAccessor));
	table->AddColumn(prefix + "icon_image_expanded", Column(&ServicesTable::IconImageExpandedAccessor, objectAccessor));
	table->AddColumn(prefix + "icon_image_alt", Column(&ServicesTable::IconImageAltAccessor, objectAccessor));
	table->AddColumn(prefix + "initial_state", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "max_check_attempts", Column(&ServicesTable::MaxCheckAttemptsAccessor, objectAccessor));
	table->AddColumn(prefix + "current_attempt", Column(&ServicesTable::CurrentAttemptAccessor, objectAccessor));
	table->AddColumn(prefix + "state", Column(&ServicesTable::StateAccessor, objectAccessor));
	table->AddColumn(prefix + "has_been_checked", Column(&ServicesTable::HasBeenCheckedAccessor, objectAccessor));
	table->AddColumn(prefix + "last_state", Column(&ServicesTable::LastStateAccessor, objectAccessor));
	table->AddColumn(prefix + "last_hard_state", Column(&ServicesTable::LastHardStateAccessor, objectAccessor));
	table->AddColumn(prefix + "state_type", Column(&ServicesTable::StateTypeAccessor, objectAccessor));
	table->AddColumn(prefix + "check_type", Column(&ServicesTable::CheckTypeAccessor, objectAccessor));
	table->AddColumn(prefix + "acknowledged", Column(&ServicesTable::AcknowledgedAccessor, objectAccessor));
	table->AddColumn(prefix + "acknowledgement_type", Column(&ServicesTable::AcknowledgementTypeAccessor, objectAccessor));
	table->AddColumn(prefix + "no_more_notifications", Column(&ServicesTable::NoMoreNotificationsAccessor, objectAccessor));
	table->AddColumn(prefix + "last_time_ok", Column(&ServicesTable::LastTimeOkAccessor, objectAccessor));
	table->AddColumn(prefix + "last_time_warning", Column(&ServicesTable::LastTimeWarningAccessor, objectAccessor));
	table->AddColumn(prefix + "last_time_critical", Column(&ServicesTable::LastTimeCriticalAccessor, objectAccessor));
	table->AddColumn(prefix + "last_time_unknown", Column(&ServicesTable::LastTimeUnknownAccessor, objectAccessor));
	table->AddColumn(prefix + "last_check", Column(&ServicesTable::LastCheckAccessor, objectAccessor));
	table->AddColumn(prefix + "next_check", Column(&ServicesTable::NextCheckAccessor, objectAccessor));
	table->AddColumn(prefix + "last_notification", Column(&ServicesTable::LastNotificationAccessor, objectAccessor));
	table->AddColumn(prefix + "next_notification", Column(&ServicesTable::NextNotificationAccessor, objectAccessor));
	table->AddColumn(prefix + "current_notification_number", Column(&ServicesTable::CurrentNotificationNumberAccessor, objectAccessor));
	table->AddColumn(prefix + "last_state_change", Column(&ServicesTable::LastStateChangeAccessor, objectAccessor));
	table->AddColumn(prefix + "last_hard_state_change", Column(&ServicesTable::LastHardStateChangeAccessor, objectAccessor));
	table->AddColumn(prefix + "scheduled_downtime_depth", Column(&ServicesTable::ScheduledDowntimeDepthAccessor, objectAccessor));
	table->AddColumn(prefix + "is_flapping", Column(&ServicesTable::IsFlappingAccessor, objectAccessor));
	table->AddColumn(prefix + "checks_enabled", Column(&ServicesTable::ChecksEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "accept_passive_checks", Column(&ServicesTable::AcceptPassiveChecksAccessor, objectAccessor));
	table->AddColumn(prefix + "event_handler_enabled", Column(&ServicesTable::EventHandlerEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "notifications_enabled", Column(&ServicesTable::NotificationsEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "process_performance_data", Column(&Table::OneAccessor, objectAccessor));
	table->AddColumn(prefix + "is_executing", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "active_checks_enabled", Column(&ServicesTable::ActiveChecksEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "check_options", Column(&ServicesTable::CheckOptionsAccessor, objectAccessor));
	table->AddColumn(prefix + "flap_detection_enabled", Column(&ServicesTable::FlapDetectionEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "check_freshness", Column(&ServicesTable::CheckFreshnessAccessor, objectAccessor));
	table->AddColumn(prefix + "obsess_over_service", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes", Column(&ServicesTable::ModifiedAttributesAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes_list", Column(&ServicesTable::ModifiedAttributesListAccessor, objectAccessor));
	table->AddColumn(prefix + "pnpgraph_present", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "staleness", Column(&ServicesTable::StalenessAccessor, objectAccessor));
	table->AddColumn(prefix + "check_interval", Column(&ServicesTable::CheckIntervalAccessor, objectAccessor));
	table->AddColumn(prefix + "retry_interval", Column(&ServicesTable::RetryIntervalAccessor, objectAccessor));
	table->AddColumn(prefix + "notification_interval", Column(&ServicesTable::NotificationIntervalAccessor, objectAccessor));
	table->AddColumn(prefix + "first_notification_delay", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "low_flap_threshold", Column(&ServicesTable::LowFlapThresholdAccessor, objectAccessor));
	table->AddColumn(prefix + "high_flap_threshold", Column(&ServicesTable::HighFlapThresholdAccessor, objectAccessor));
	table->AddColumn(prefix + "latency", Column(&ServicesTable::LatencyAccessor, objectAccessor));
	table->AddColumn(prefix + "execution_time", Column(&ServicesTable::ExecutionTimeAccessor, objectAccessor));
	table->AddColumn(prefix + "percent_state_change", Column(&ServicesTable::PercentStateChangeAccessor, objectAccessor));
	table->AddColumn(prefix + "in_check_period", Column(&ServicesTable::InCheckPeriodAccessor, objectAccessor));
	table->AddColumn(prefix + "in_notification_period", Column(&ServicesTable::InNotificationPeriodAccessor, objectAccessor));
	table->AddColumn(prefix + "contacts", Column(&ServicesTable::ContactsAccessor, objectAccessor));
	table->AddColumn(prefix + "downtimes", Column(&ServicesTable::DowntimesAccessor, objectAccessor));
	table->AddColumn(prefix + "downtimes_with_info", Column(&ServicesTable::DowntimesWithInfoAccessor, objectAccessor));
	table->AddColumn(prefix + "comments", Column(&ServicesTable::CommentsAccessor, objectAccessor));
	table->AddColumn(prefix + "comments_with_info", Column(&ServicesTable::CommentsWithInfoAccessor, objectAccessor));
	table->AddColumn(prefix + "comments_with_extra_info", Column(&ServicesTable::CommentsWithExtraInfoAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variable_names", Column(&ServicesTable::CustomVariableNamesAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variable_values", Column(&ServicesTable::CustomVariableValuesAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variables", Column(&ServicesTable::CustomVariablesAccessor, objectAccessor));
	table->AddColumn(prefix + "groups", Column(&ServicesTable::GroupsAccessor, objectAccessor));
	table->AddColumn(prefix + "contact_groups", Column(&ServicesTable::ContactGroupsAccessor, objectAccessor));

	HostsTable::AddColumns(table, "host_", boost::bind(&ServicesTable::HostAccessor, _1, objectAccessor));
}

String ServicesTable::GetName(void) const
{
	return "services";
}

void ServicesTable::FetchRows(const AddRowFunction& addRowFn)
{
	BOOST_FOREACH(const Service::Ptr& service, DynamicType::GetObjects<Service>()) {
		addRowFn(service);
	}
}

Object::Ptr ServicesTable::HostAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor)
{
	Value service;

	if (parentObjectAccessor)
		service = parentObjectAccessor(row);
	else
		service = row;

	return static_cast<Service::Ptr>(service)->GetHost();
}

Value ServicesTable::ShortNameAccessor(const Value& row)
{
	return static_cast<Service::Ptr>(row)->GetShortName();
}

Value ServicesTable::DisplayNameAccessor(const Value& row)
{
	return static_cast<Service::Ptr>(row)->GetDisplayName();
}

Value ServicesTable::CheckCommandAccessor(const Value& row)
{
	CheckCommand::Ptr checkcommand = static_cast<Service::Ptr>(row)->GetCheckCommand();

	if (checkcommand)
		return checkcommand->GetName(); /* this is the name without '!' args */

	return Empty;
}

Value ServicesTable::CheckCommandExpandedAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

        CheckCommand::Ptr commandObj = service->GetCheckCommand();

	if (!commandObj)
		return Empty;

	Value raw_command = commandObj->GetCommandLine();

        std::vector<MacroResolver::Ptr> resolvers;
        resolvers.push_back(service);
        resolvers.push_back(service->GetHost());
        resolvers.push_back(commandObj);
        resolvers.push_back(IcingaApplication::GetInstance());

        Value commandLine = MacroProcessor::ResolveMacros(raw_command, resolvers, Dictionary::Ptr(), Utility::EscapeShellCmd);

        String buf;
        if (commandLine.IsObjectType<Array>()) {
                Array::Ptr args = commandLine;

                ObjectLock olock(args);
                String arg;
                BOOST_FOREACH(arg, args) {
                        // This is obviously incorrect for non-trivial cases.
                        String argitem = " \"" + arg + "\"";
                        boost::algorithm::replace_all(argitem, "\n", "\\n");
                        buf += argitem;
                }
        } else if (!commandLine.IsEmpty()) {
                String args = Convert::ToString(commandLine);
                boost::algorithm::replace_all(args, "\n", "\\n");
                buf += args;
        } else {
                buf += "<internal>";
        }

	return buf;
}

Value ServicesTable::EventHandlerAccessor(const Value& row)
{
	EventCommand::Ptr eventcommand = static_cast<Service::Ptr>(row)->GetEventCommand();

	if (eventcommand)
		return eventcommand->GetName();

	return Empty;
}

Value ServicesTable::PluginOutputAccessor(const Value& row)
{
	return static_cast<Service::Ptr>(row)->GetLastCheckOutput();
}

Value ServicesTable::LongPluginOutputAccessor(const Value& row)
{
	return static_cast<Service::Ptr>(row)->GetLastCheckLongOutput();
}

Value ServicesTable::PerfDataAccessor(const Value& row)
{
	return static_cast<Service::Ptr>(row)->GetLastCheckPerfData();
}

Value ServicesTable::NotificationPeriodAccessor(const Value& row)
{
	BOOST_FOREACH(const Notification::Ptr& notification, static_cast<Service::Ptr>(row)->GetNotifications()) {
		ObjectLock olock(notification);

		TimePeriod::Ptr timeperiod = notification->GetNotificationPeriod();

		/* XXX first notification wins */
		if (timeperiod)
			return timeperiod->GetName();
	}

	return Empty;
}

Value ServicesTable::CheckPeriodAccessor(const Value& row)
{
	TimePeriod::Ptr timeperiod = static_cast<Service::Ptr>(row)->GetCheckPeriod();

	if (!timeperiod)
		return Empty;

	return timeperiod->GetName();
}

Value ServicesTable::NotesAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<Service::Ptr>(row)->GetCustom();

	if (!custom)
		return Empty;

	return custom->Get("notes");
}

Value ServicesTable::NotesExpandedAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);
	Dictionary::Ptr custom = service->GetCustom();

	if (!custom)
		return Empty;

	std::vector<MacroResolver::Ptr> resolvers;
	resolvers.push_back(service);
	resolvers.push_back(service->GetHost());
	resolvers.push_back(IcingaApplication::GetInstance());

	Value value = custom->Get("notes");

	Dictionary::Ptr cr;
	Value value_expanded = MacroProcessor::ResolveMacros(value, resolvers, cr, Utility::EscapeShellCmd);

	return value_expanded;
}

Value ServicesTable::NotesUrlAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<Service::Ptr>(row)->GetCustom();

	if (!custom)
		return Empty;

	return custom->Get("notes_url");
}

Value ServicesTable::NotesUrlExpandedAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);
	Dictionary::Ptr custom = service->GetCustom();

	if (!custom)
		return Empty;

	std::vector<MacroResolver::Ptr> resolvers;
	resolvers.push_back(service);
	resolvers.push_back(service->GetHost());
	resolvers.push_back(IcingaApplication::GetInstance());

	Value value = custom->Get("notes_url");

	Dictionary::Ptr cr;
	Value value_expanded = MacroProcessor::ResolveMacros(value, resolvers, cr, Utility::EscapeShellCmd);

	return value_expanded;
}

Value ServicesTable::ActionUrlAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<Service::Ptr>(row)->GetCustom();

	if (!custom)
		return Empty;

	return custom->Get("action_url");
}

Value ServicesTable::ActionUrlExpandedAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);
	Dictionary::Ptr custom = service->GetCustom();

	if (!custom)
		return Empty;

	std::vector<MacroResolver::Ptr> resolvers;
	resolvers.push_back(service);
	resolvers.push_back(service->GetHost());
	resolvers.push_back(IcingaApplication::GetInstance());

	Value value = custom->Get("action_url");

	Dictionary::Ptr cr;
	Value value_expanded = MacroProcessor::ResolveMacros(value, resolvers, cr, Utility::EscapeShellCmd);

	return value_expanded;
}

Value ServicesTable::IconImageAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<Service::Ptr>(row)->GetCustom();

	if (!custom)
		return Empty;

	return custom->Get("icon_image");
}

Value ServicesTable::IconImageExpandedAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);
	Dictionary::Ptr custom = service->GetCustom();

	if (!custom)
		return Empty;

	std::vector<MacroResolver::Ptr> resolvers;
	resolvers.push_back(service);
	resolvers.push_back(service->GetHost());
	resolvers.push_back(IcingaApplication::GetInstance());

	Value value = custom->Get("icon_image");

	Dictionary::Ptr cr;
	Value value_expanded = MacroProcessor::ResolveMacros(value, resolvers, cr, Utility::EscapeShellCmd);

	return value_expanded;
}

Value ServicesTable::IconImageAltAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<Service::Ptr>(row)->GetCustom();

	if (!custom)
		return Empty;

	return custom->Get("icon_image_alt");
}

Value ServicesTable::MaxCheckAttemptsAccessor(const Value& row)
{
	return static_cast<Service::Ptr>(row)->GetMaxCheckAttempts();
}

Value ServicesTable::CurrentAttemptAccessor(const Value& row)
{
	return static_cast<Service::Ptr>(row)->GetCurrentCheckAttempt();
}

Value ServicesTable::StateAccessor(const Value& row)
{
	return static_cast<Service::Ptr>(row)->GetState();
}

Value ServicesTable::HasBeenCheckedAccessor(const Value& row)
{
	return (static_cast<Service::Ptr>(row)->HasBeenChecked() ? 1 : 0);
}

Value ServicesTable::LastStateAccessor(const Value& row)
{
	return static_cast<Service::Ptr>(row)->GetLastState();
}

Value ServicesTable::LastHardStateAccessor(const Value& row)
{
	return static_cast<Service::Ptr>(row)->GetLastHardState();
}

Value ServicesTable::StateTypeAccessor(const Value& row)
{
	return static_cast<Service::Ptr>(row)->GetStateType();
}

Value ServicesTable::CheckTypeAccessor(const Value& row)
{
	return (static_cast<Service::Ptr>(row)->GetEnableActiveChecks() ? 0 : 1);
}

Value ServicesTable::AcknowledgedAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	/* important: lock acknowledgements */
	ObjectLock olock(service);

	return (service->IsAcknowledged() ? 1 : 0);
}

Value ServicesTable::AcknowledgementTypeAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	/* important: lock acknowledgements */
	ObjectLock olock(service);

	return static_cast<int>(service->GetAcknowledgement());
}

Value ServicesTable::NoMoreNotificationsAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	/* XXX take the smallest notification_interval */
        double notification_interval = -1;
        BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
                if (notification_interval == -1 || notification->GetNotificationInterval() < notification_interval)
                        notification_interval = notification->GetNotificationInterval();
        }

        if (notification_interval == 0 && !service->IsVolatile())
                return 1;

	return 0;
}

Value ServicesTable::LastTimeOkAccessor(const Value& row)
{
	return static_cast<int>(static_cast<Service::Ptr>(row)->GetLastStateOK());
}

Value ServicesTable::LastTimeWarningAccessor(const Value& row)
{
	return static_cast<int>(static_cast<Service::Ptr>(row)->GetLastStateWarning());
}

Value ServicesTable::LastTimeCriticalAccessor(const Value& row)
{
	return static_cast<int>(static_cast<Service::Ptr>(row)->GetLastStateCritical());
}

Value ServicesTable::LastTimeUnknownAccessor(const Value& row)
{
	return static_cast<int>(static_cast<Service::Ptr>(row)->GetLastStateUnknown());
}

Value ServicesTable::LastCheckAccessor(const Value& row)
{
	return static_cast<int>(static_cast<Service::Ptr>(row)->GetLastCheck());
}

Value ServicesTable::NextCheckAccessor(const Value& row)
{
	return static_cast<int>(static_cast<Service::Ptr>(row)->GetNextCheck());
}

Value ServicesTable::LastNotificationAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	/* XXX Service -> Notifications, latest wins */
	double last_notification = 0;
	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
		if (notification->GetLastNotification() > last_notification)
			last_notification = notification->GetLastNotification();
	}

	return static_cast<int>(last_notification);
}

Value ServicesTable::NextNotificationAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	/* XXX Service -> Notifications, latest wins */
	double next_notification = 0;
	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
		if (notification->GetNextNotification() < next_notification)
			next_notification = notification->GetNextNotification();
	}

	return static_cast<int>(next_notification);
}

Value ServicesTable::CurrentNotificationNumberAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	/* XXX Service -> Notifications, biggest wins */
	int notification_number = 0;
	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
		if (notification->GetNotificationNumber() > notification_number)
			notification_number = notification->GetNotificationNumber();
	}

	return notification_number;
}

Value ServicesTable::LastStateChangeAccessor(const Value& row)
{
	return static_cast<int>(static_cast<Service::Ptr>(row)->GetLastStateChange());
}

Value ServicesTable::LastHardStateChangeAccessor(const Value& row)
{
	return static_cast<int>(static_cast<Service::Ptr>(row)->GetLastHardStateChange());
}

Value ServicesTable::ScheduledDowntimeDepthAccessor(const Value& row)
{
	return static_cast<Service::Ptr>(row)->GetDowntimeDepth();
}

Value ServicesTable::IsFlappingAccessor(const Value& row)
{
	return static_cast<Service::Ptr>(row)->IsFlapping();
}

Value ServicesTable::ChecksEnabledAccessor(const Value& row)
{
	return (static_cast<Service::Ptr>(row)->GetEnableActiveChecks() ? 1 : 0);
}

Value ServicesTable::AcceptPassiveChecksAccessor(const Value& row)
{
	return (static_cast<Service::Ptr>(row)->GetEnablePassiveChecks() ? 1 : 0);
}

Value ServicesTable::EventHandlerEnabledAccessor(const Value& row)
{
	EventCommand::Ptr eventcommand = static_cast<Service::Ptr>(row)->GetEventCommand();

	if (eventcommand)
		return 1;

	return 0;
}

Value ServicesTable::NotificationsEnabledAccessor(const Value& row)
{
	return (static_cast<Service::Ptr>(row)->GetEnableNotifications() ? 1 : 0);
}

Value ServicesTable::ActiveChecksEnabledAccessor(const Value& row)
{
	return (static_cast<Service::Ptr>(row)->GetEnableActiveChecks() ? 1 : 0);
}

Value ServicesTable::CheckOptionsAccessor(const Value& row)
{
	/* TODO - forcexec, freshness, orphan, none */
	return Empty;
}

Value ServicesTable::FlapDetectionEnabledAccessor(const Value& row)
{
	return (static_cast<Service::Ptr>(row)->GetEnableFlapping() ? 1 : 0);
}

Value ServicesTable::CheckFreshnessAccessor(const Value& row)
{
	/* always enabled */
	return 1;
}

Value ServicesTable::ModifiedAttributesAccessor(const Value& row)
{
	return static_cast<Service::Ptr>(row)->GetModifiedAttributes();
}

Value ServicesTable::ModifiedAttributesListAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value ServicesTable::StalenessAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (service->HasBeenChecked() && service->GetLastCheck() > 0)
		return (Utility::GetTime() - service->GetLastCheck()) / (service->GetCheckInterval() * 3600);

	return Empty;
}

Value ServicesTable::CheckIntervalAccessor(const Value& row)
{
	return (static_cast<Service::Ptr>(row)->GetCheckInterval() / 60.0);
}

Value ServicesTable::RetryIntervalAccessor(const Value& row)
{
	return (static_cast<Service::Ptr>(row)->GetRetryInterval() / 60.0);
}

Value ServicesTable::NotificationIntervalAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	/* XXX take the smallest notification_interval */
        double notification_interval = -1;
        BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
                if (notification_interval == -1 || notification->GetNotificationInterval() < notification_interval)
                        notification_interval = notification->GetNotificationInterval();
        }

        if (notification_interval == -1)
                notification_interval = 60;

	return (notification_interval / 60.0);
}

Value ServicesTable::LowFlapThresholdAccessor(const Value& row)
{
	return static_cast<Service::Ptr>(row)->GetFlappingThreshold();
}

Value ServicesTable::HighFlapThresholdAccessor(const Value& row)
{
	return static_cast<Service::Ptr>(row)->GetFlappingThreshold();
}

Value ServicesTable::LatencyAccessor(const Value& row)
{
	return (Service::CalculateLatency(static_cast<Service::Ptr>(row)->GetLastCheckResult()));
}

Value ServicesTable::ExecutionTimeAccessor(const Value& row)
{
	return (Service::CalculateExecutionTime(static_cast<Service::Ptr>(row)->GetLastCheckResult()));
}

Value ServicesTable::PercentStateChangeAccessor(const Value& row)
{
	return static_cast<Service::Ptr>(row)->GetFlappingCurrent();
}

Value ServicesTable::InCheckPeriodAccessor(const Value& row)
{
	TimePeriod::Ptr timeperiod = static_cast<Service::Ptr>(row)->GetCheckPeriod();

	/* none set means always checked */
	if (!timeperiod)
		return 1;

	return (timeperiod->IsInside(Utility::GetTime()) ? 1 : 0);
}

Value ServicesTable::InNotificationPeriodAccessor(const Value& row)
{
	BOOST_FOREACH(const Notification::Ptr& notification, static_cast<Service::Ptr>(row)->GetNotifications()) {
		ObjectLock olock(notification);

		TimePeriod::Ptr timeperiod = notification->GetNotificationPeriod();

		/* XXX first notification wins */
		if (timeperiod)
			return (timeperiod->IsInside(Utility::GetTime()) ? 1 : 0);
	}

	/* none set means always notified */
	return 1;
}

Value ServicesTable::ContactsAccessor(const Value& row)
{
	Array::Ptr contact_names = boost::make_shared<Array>();

	BOOST_FOREACH(const User::Ptr& user, CompatUtility::GetServiceNotificationUsers(static_cast<Service::Ptr>(row))) {
		contact_names->Add(user->GetName());
	}

	return contact_names;
}

Value ServicesTable::DowntimesAccessor(const Value& row)
{
	Dictionary::Ptr downtimes = static_cast<Service::Ptr>(row)->GetDowntimes();

	if (!downtimes)
		return Empty;

	Array::Ptr ids = boost::make_shared<Array>();

	ObjectLock olock(downtimes);

	String id;
	Dictionary::Ptr downtime;
	BOOST_FOREACH(boost::tie(id, downtime), downtimes) {

		if (!downtime)
			continue;

		if (Service::IsDowntimeExpired(downtime))
			continue;

		ids->Add(downtime->Get("legacy_id"));
	}

	return ids;
}

Value ServicesTable::DowntimesWithInfoAccessor(const Value& row)
{
	Dictionary::Ptr downtimes = static_cast<Service::Ptr>(row)->GetDowntimes();

	if (!downtimes)
		return Empty;

	Array::Ptr ids = boost::make_shared<Array>();

	ObjectLock olock(downtimes);

	String id;
	Dictionary::Ptr downtime;
	BOOST_FOREACH(boost::tie(id, downtime), downtimes) {

		if (!downtime)
			continue;

		if (Service::IsDowntimeExpired(downtime))
			continue;

		Array::Ptr downtime_info = boost::make_shared<Array>();
		downtime_info->Add(downtime->Get("legacy_id"));
		downtime_info->Add(downtime->Get("author"));
		downtime_info->Add(downtime->Get("comment"));
		ids->Add(downtime_info);
	}

	return ids;
}

Value ServicesTable::CommentsAccessor(const Value& row)
{
	Dictionary::Ptr comments = static_cast<Service::Ptr>(row)->GetComments();

	if (!comments)
		return Empty;

	Array::Ptr ids = boost::make_shared<Array>();

	ObjectLock olock(comments);

	String id;
	Dictionary::Ptr comment;
	BOOST_FOREACH(boost::tie(id, comment), comments) {

		if (!comment)
			continue;

		if (Service::IsCommentExpired(comment))
			continue;

		ids->Add(comment->Get("legacy_id"));
	}

	return ids;
}

Value ServicesTable::CommentsWithInfoAccessor(const Value& row)
{
	Dictionary::Ptr comments = static_cast<Service::Ptr>(row)->GetComments();

	if (!comments)
		return Empty;

	Array::Ptr ids = boost::make_shared<Array>();

	ObjectLock olock(comments);

	String id;
	Dictionary::Ptr comment;
	BOOST_FOREACH(boost::tie(id, comment), comments) {

		if (!comment)
			continue;

		if (Service::IsCommentExpired(comment))
			continue;

		Array::Ptr comment_info = boost::make_shared<Array>();
		comment_info->Add(comment->Get("legacy_id"));
		comment_info->Add(comment->Get("author"));
		comment_info->Add(comment->Get("text"));
		ids->Add(comment_info);
	}

	return ids;
}

Value ServicesTable::CommentsWithExtraInfoAccessor(const Value& row)
{
	Dictionary::Ptr comments = static_cast<Service::Ptr>(row)->GetComments();

	if (!comments)
		return Empty;

	Array::Ptr ids = boost::make_shared<Array>();

	ObjectLock olock(comments);

	String id;
	Dictionary::Ptr comment;
	BOOST_FOREACH(boost::tie(id, comment), comments) {

		if (!comment)
			continue;

		if (Service::IsCommentExpired(comment))
			continue;

		Array::Ptr comment_info = boost::make_shared<Array>();
		comment_info->Add(comment->Get("legacy_id"));
		comment_info->Add(comment->Get("author"));
		comment_info->Add(comment->Get("text"));
		comment_info->Add(comment->Get("entry_type"));
		comment_info->Add(static_cast<int>(comment->Get("entry_time")));
		ids->Add(comment_info);
	}

	return ids;
}

Value ServicesTable::CustomVariableNamesAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<Service::Ptr>(row)->GetCustom();

	if (!custom)
		return Empty;

	Array::Ptr cv = boost::make_shared<Array>();

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

		cv->Add(key);
	}

	return cv;
}

Value ServicesTable::CustomVariableValuesAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<Service::Ptr>(row)->GetCustom();

	if (!custom)
		return Empty;

	Array::Ptr cv = boost::make_shared<Array>();

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

		cv->Add(value);
	}

	return cv;
}

Value ServicesTable::CustomVariablesAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<Service::Ptr>(row)->GetCustom();

	if (!custom)
		return Empty;

	Array::Ptr cv = boost::make_shared<Array>();

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

		Array::Ptr key_val = boost::make_shared<Array>();
		key_val->Add(key);
		key_val->Add(value);
		cv->Add(key_val);
	}

	return cv;
}

Value ServicesTable::GroupsAccessor(const Value& row)
{
	Array::Ptr groups = static_cast<Service::Ptr>(row)->GetGroups();

	if (!groups)
		return Empty;

	return groups;
}

Value ServicesTable::ContactGroupsAccessor(const Value& row)
{
	Array::Ptr contactgroup_names = boost::make_shared<Array>();

	BOOST_FOREACH(const UserGroup::Ptr& usergroup, CompatUtility::GetServiceNotificationUserGroups(static_cast<Service::Ptr>(row))) {
		contactgroup_names->Add(usergroup->GetName());
	}

	return contactgroup_names;
}


