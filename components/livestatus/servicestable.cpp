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

#include "livestatus/servicestable.h"
#include "livestatus/hoststable.h"
#include "icinga/service.h"
#include "icinga/checkcommand.h"
#include "icinga/eventcommand.h"
#include "icinga/timeperiod.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include <boost/foreach.hpp>

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
	table->AddColumn(prefix + "initial_state", Column(&ServicesTable::InitialStateAccessor, objectAccessor));
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
	table->AddColumn(prefix + "process_performance_data", Column(&ServicesTable::ProcessPerformanceDataAccessor, objectAccessor));
	table->AddColumn(prefix + "is_executing", Column(&ServicesTable::IsExecutingAccessor, objectAccessor));
	table->AddColumn(prefix + "active_checks_enabled", Column(&ServicesTable::ActiveChecksEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "check_options", Column(&ServicesTable::CheckOptionsAccessor, objectAccessor));
	table->AddColumn(prefix + "flap_detection_enabled", Column(&ServicesTable::FlapDetectionEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "check_freshness", Column(&ServicesTable::CheckFreshnessAccessor, objectAccessor));
	table->AddColumn(prefix + "obsess_over_service", Column(&ServicesTable::ObsessOverServiceAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes", Column(&ServicesTable::ModifiedAttributesAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes_list", Column(&ServicesTable::ModifiedAttributesListAccessor, objectAccessor));
	table->AddColumn(prefix + "pnpgraph_present", Column(&ServicesTable::PnpgraphPresentAccessor, objectAccessor));
	table->AddColumn(prefix + "check_interval", Column(&ServicesTable::CheckIntervalAccessor, objectAccessor));
	table->AddColumn(prefix + "retry_interval", Column(&ServicesTable::RetryIntervalAccessor, objectAccessor));
	table->AddColumn(prefix + "notification_interval", Column(&ServicesTable::NotificationIntervalAccessor, objectAccessor));
	table->AddColumn(prefix + "first_notification_delay", Column(&ServicesTable::FirstNotificationDelayAccessor, objectAccessor));
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

	HostsTable::AddColumns(table, "host_", &ServicesTable::HostAccessor);
}

String ServicesTable::GetName(void) const
{
	return "services";
}

void ServicesTable::FetchRows(const AddRowFunction& addRowFn)
{
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		addRowFn(object);
	}
}

Object::Ptr ServicesTable::HostAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Service>(object)->GetHost();
}

Value ServicesTable::ShortNameAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Service>(object)->GetShortName();
}

Value ServicesTable::DisplayNameAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Service>(object)->GetDisplayName();
}

Value ServicesTable::CheckCommandAccessor(const Object::Ptr& object)
{
	CheckCommand::Ptr checkcommand = static_pointer_cast<Service>(object)->GetCheckCommand();

	if (checkcommand)
		return checkcommand->GetName(); /* this is the name without '!' args */

	return Value();
}

Value ServicesTable::CheckCommandExpandedAccessor(const Object::Ptr& object)
{
	CheckCommand::Ptr checkcommand = static_pointer_cast<Service>(object)->GetCheckCommand();

	if (checkcommand)
		return checkcommand->GetName(); /* this is the name without '!' args */

	return Value();
}

Value ServicesTable::EventHandlerAccessor(const Object::Ptr& object)
{
	EventCommand::Ptr eventcommand = static_pointer_cast<Service>(object)->GetEventCommand();

	if (eventcommand)
		return eventcommand->GetName();

	return Value();
}

Value ServicesTable::PluginOutputAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Service>(object)->GetLastCheckOutput();
}

Value ServicesTable::LongPluginOutputAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Service>(object)->GetLastCheckLongOutput();
}

Value ServicesTable::PerfDataAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Service>(object)->GetLastCheckPerfData();
}

Value ServicesTable::NotificationPeriodAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::CheckPeriodAccessor(const Object::Ptr& object)
{
	TimePeriod::Ptr timeperiod = static_pointer_cast<Service>(object)->GetCheckPeriod();

	if (!timeperiod)
		return Value();

	return timeperiod->GetName();
}

Value ServicesTable::NotesAccessor(const Object::Ptr& object)
{
	Dictionary::Ptr custom = static_pointer_cast<Service>(object)->GetCustom();

	if (!custom)
		return Value();

	return custom->Get("notes");
}

Value ServicesTable::NotesExpandedAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::NotesUrlAccessor(const Object::Ptr& object)
{
	Dictionary::Ptr custom = static_pointer_cast<Service>(object)->GetCustom();

	if (!custom)
		return Value();

	return custom->Get("notes_url");
}

Value ServicesTable::NotesUrlExpandedAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::ActionUrlAccessor(const Object::Ptr& object)
{
	Dictionary::Ptr custom = static_pointer_cast<Service>(object)->GetCustom();

	if (!custom)
		return Value();

	return custom->Get("action_url");
}

Value ServicesTable::ActionUrlExpandedAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::IconImageAccessor(const Object::Ptr& object)
{
	Dictionary::Ptr custom = static_pointer_cast<Service>(object)->GetCustom();

	if (!custom)
		return Value();

	return custom->Get("icon_image");
}

Value ServicesTable::IconImageExpandedAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::IconImageAltAccessor(const Object::Ptr& object)
{
	Dictionary::Ptr custom = static_pointer_cast<Service>(object)->GetCustom();

	if (!custom)
		return Value();

	return custom->Get("icon_image_alt");
}

Value ServicesTable::InitialStateAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value ServicesTable::MaxCheckAttemptsAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Service>(object)->GetMaxCheckAttempts();
}

Value ServicesTable::CurrentAttemptAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Service>(object)->GetCurrentCheckAttempt();
}

Value ServicesTable::StateAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Service>(object)->GetState();
}

Value ServicesTable::HasBeenCheckedAccessor(const Object::Ptr& object)
{
	return (static_pointer_cast<Service>(object)->HasBeenChecked() ? 1 : 0);
}

Value ServicesTable::LastStateAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Service>(object)->GetLastState();
}

Value ServicesTable::LastHardStateAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Service>(object)->GetLastHardState();
}

Value ServicesTable::StateTypeAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Service>(object)->GetStateType();
}

Value ServicesTable::CheckTypeAccessor(const Object::Ptr& object)
{
	return (static_pointer_cast<Service>(object)->GetEnableActiveChecks() ? 1 : 0);
}

Value ServicesTable::AcknowledgedAccessor(const Object::Ptr& object)
{
	Service::Ptr service = static_pointer_cast<Service>(object);

	/* important: lock acknowledgements */
	ObjectLock olock(service);

	return (service->IsAcknowledged() ? 1 : 0);
}

Value ServicesTable::AcknowledgementTypeAccessor(const Object::Ptr& object)
{
	Service::Ptr service = static_pointer_cast<Service>(object);

	/* important: lock acknowledgements */
	ObjectLock olock(service);

	return static_cast<int>(service->GetAcknowledgement());
}

Value ServicesTable::NoMoreNotificationsAccessor(const Object::Ptr& object)
{
	/* TODO: notification_interval == 0, volatile == false */
	return Value();
}

Value ServicesTable::LastTimeOkAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::LastTimeWarningAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::LastTimeCriticalAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::LastTimeUnknownAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::LastCheckAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Service>(object)->GetLastCheck();
}

Value ServicesTable::NextCheckAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Service>(object)->GetNextCheck();
}

Value ServicesTable::LastNotificationAccessor(const Object::Ptr& object)
{
	/* TODO Host->Service->GetNotifications->(loop)->GetLastNotification() */
	return Value();
}

Value ServicesTable::NextNotificationAccessor(const Object::Ptr& object)
{
	/* TODO Host->Service->GetNotifications->(loop)->GetLastNotification() */
	return Value();
}

Value ServicesTable::CurrentNotificationNumberAccessor(const Object::Ptr& object)
{
	/* TODO Host->Service->GetNotifications->(loop) new attribute */
	return Value();
}

Value ServicesTable::LastStateChangeAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Service>(object)->GetLastStateChange();
}

Value ServicesTable::LastHardStateChangeAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Service>(object)->GetLastHardStateChange();
}

Value ServicesTable::ScheduledDowntimeDepthAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Service>(object)->GetDowntimeDepth();
}

Value ServicesTable::IsFlappingAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Service>(object)->IsFlapping();
}

Value ServicesTable::ChecksEnabledAccessor(const Object::Ptr& object)
{
	return (static_pointer_cast<Service>(object)->GetEnableActiveChecks() ? 1 : 0);
}

Value ServicesTable::AcceptPassiveChecksAccessor(const Object::Ptr& object)
{
	return (static_pointer_cast<Service>(object)->GetEnablePassiveChecks() ? 1 : 0);
}

Value ServicesTable::EventHandlerEnabledAccessor(const Object::Ptr& object)
{
	/* TODO always enabled*/
	return Value(1);
}

Value ServicesTable::NotificationsEnabledAccessor(const Object::Ptr& object)
{
	return (static_pointer_cast<Service>(object)->GetEnableNotifications() ? 1 : 0);
}

Value ServicesTable::ProcessPerformanceDataAccessor(const Object::Ptr& object)
{
	/* TODO always enabled */
	return Value(1);
}

Value ServicesTable::IsExecutingAccessor(const Object::Ptr& object)
{
	/* TODO does that make sense with Icinga2? */
	return Value();
}

Value ServicesTable::ActiveChecksEnabledAccessor(const Object::Ptr& object)
{
	return (static_pointer_cast<Service>(object)->GetEnableActiveChecks() ? 1 : 0);
}

Value ServicesTable::CheckOptionsAccessor(const Object::Ptr& object)
{
	/* TODO - forcexec, freshness, orphan, none */
	return Value();
}

Value ServicesTable::FlapDetectionEnabledAccessor(const Object::Ptr& object)
{
	return (static_pointer_cast<Service>(object)->GetEnableFlapping() ? 1 : 0);
}

Value ServicesTable::CheckFreshnessAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::ObsessOverServiceAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value ServicesTable::ModifiedAttributesAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value ServicesTable::ModifiedAttributesListAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value ServicesTable::PnpgraphPresentAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value ServicesTable::CheckIntervalAccessor(const Object::Ptr& object)
{
	return (static_pointer_cast<Service>(object)->GetCheckInterval() / 60.0);
}

Value ServicesTable::RetryIntervalAccessor(const Object::Ptr& object)
{
	return (static_pointer_cast<Service>(object)->GetRetryInterval() / 60.0);
}

Value ServicesTable::NotificationIntervalAccessor(const Object::Ptr& object)
{
	/* TODO Host->Services->GetNotifications->(loop)->GetNotificationInterval() */
	return Value();
}

Value ServicesTable::FirstNotificationDelayAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value ServicesTable::LowFlapThresholdAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::HighFlapThresholdAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::LatencyAccessor(const Object::Ptr& object)
{
	return (Service::CalculateLatency(static_pointer_cast<Service>(object)->GetLastCheckResult()));
}

Value ServicesTable::ExecutionTimeAccessor(const Object::Ptr& object)
{
	return (Service::CalculateExecutionTime(static_pointer_cast<Service>(object)->GetLastCheckResult()));
}

Value ServicesTable::PercentStateChangeAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Service>(object)->GetFlappingCurrent();
}

Value ServicesTable::InCheckPeriodAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::InNotificationPeriodAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::ContactsAccessor(const Object::Ptr& object)
{
	/* TODO - host->service->notifications->users */
/*
	std::set<User::Ptr> allUsers;
	std::set<User::Ptr> users;

	BOOST_FOREACH(const Notification::Ptr& notification, static_pointer_cast<Service>(object)->GetNotifications()) {
		ObjectLock olock(notification);

		users = notification->GetUsers();

		std::copy(users.begin(), users.end(), std::inserter(allUsers, allUsers.begin()));

		BOOST_FOREACH(const UserGroup::Ptr& ug, notification->GetGroups()) {
			std::set<User::Ptr> members = ug->GetMembers();
			std::copy(members.begin(), members.end(), std::inserter(allUsers, allUsers.begin()));
                }
        }
*/
	return Value();
}

Value ServicesTable::DowntimesAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::DowntimesWithInfoAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::CommentsAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::CommentsWithInfoAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::CommentsWithExtraInfoAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::CustomVariableNamesAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::CustomVariableValuesAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::CustomVariablesAccessor(const Object::Ptr& object)
{
	/*
	Service::Ptr svc = static_pointer_cast<Service>(object);

	Dictionary::Ptr custom = svc->Get("custom");

	if (!custom)
		return Value();

	Dictionary::Ptr customvars = custom->ShallowClone();
	customvars->Remove("notes");
	customvars->Remove("action_url");
	customvars->Remove("notes_url");
	customvars->Remove("icon_image");
	customvars->Remove("icon_image_alt");
	customvars->Remove("statusmap_image");
	customvars->Remove("2d_coords");

	return customvars;
	*/
	/* TODO */
	return Value();
}

Value ServicesTable::GroupsAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value ServicesTable::ContactGroupsAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}


