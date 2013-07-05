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

#include "livestatus/hoststable.h"
#include "icinga/host.h"
#include "icinga/service.h"
#include "icinga/checkcommand.h"
#include "icinga/eventcommand.h"
#include "icinga/timeperiod.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include <boost/foreach.hpp>
#include <boost/smart_ptr/make_shared.hpp>

using namespace icinga;
using namespace livestatus;

HostsTable::HostsTable(void)
{
	AddColumns(this);
}

void HostsTable::AddColumns(Table *table, const String& prefix,
    const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "name", Column(&HostsTable::NameAccessor, objectAccessor));
	table->AddColumn(prefix + "display_name", Column(&HostsTable::DisplayNameAccessor, objectAccessor));
	table->AddColumn(prefix + "alias", Column(&HostsTable::DisplayNameAccessor, objectAccessor));
	table->AddColumn(prefix + "address", Column(&HostsTable::AddressAccessor, objectAccessor));
	table->AddColumn(prefix + "address6", Column(&HostsTable::Address6Accessor, objectAccessor));
	table->AddColumn(prefix + "check_command", Column(&HostsTable::CheckCommandAccessor, objectAccessor));
	table->AddColumn(prefix + "check_command_expanded", Column(&HostsTable::CheckCommandExpandedAccessor, objectAccessor));
	table->AddColumn(prefix + "event_handler", Column(&HostsTable::EventHandlerAccessor, objectAccessor));
	table->AddColumn(prefix + "notification_period", Column(&HostsTable::NotificationPeriodAccessor, objectAccessor));
	table->AddColumn(prefix + "check_period", Column(&HostsTable::CheckPeriodAccessor, objectAccessor));
	table->AddColumn(prefix + "notes", Column(&HostsTable::NotesAccessor, objectAccessor));
	table->AddColumn(prefix + "notes_expanded", Column(&HostsTable::NotesExpandedAccessor, objectAccessor));
	table->AddColumn(prefix + "notes_url", Column(&HostsTable::NotesUrlAccessor, objectAccessor));
	table->AddColumn(prefix + "notes_url_expanded", Column(&HostsTable::NotesUrlExpandedAccessor, objectAccessor));
	table->AddColumn(prefix + "action_url", Column(&HostsTable::ActionUrlAccessor, objectAccessor));
	table->AddColumn(prefix + "action_url_expanded", Column(&HostsTable::ActionUrlExpandedAccessor, objectAccessor));
	table->AddColumn(prefix + "plugin_output", Column(&HostsTable::PluginOutputAccessor, objectAccessor));
	table->AddColumn(prefix + "perf_data", Column(&HostsTable::PerfDataAccessor, objectAccessor));
	table->AddColumn(prefix + "icon_image", Column(&HostsTable::IconImageAccessor, objectAccessor));
	table->AddColumn(prefix + "icon_image_expanded", Column(&HostsTable::IconImageExpandedAccessor, objectAccessor));
	table->AddColumn(prefix + "icon_image_alt", Column(&HostsTable::IconImageAltAccessor, objectAccessor));
	table->AddColumn(prefix + "statusmap_image", Column(&HostsTable::StatusmapImageAccessor, objectAccessor));
	table->AddColumn(prefix + "long_plugin_output", Column(&HostsTable::LongPluginOutputAccessor, objectAccessor));
	table->AddColumn(prefix + "initial_state", Column(&HostsTable::InitialStateAccessor, objectAccessor));
	table->AddColumn(prefix + "max_check_attempts", Column(&HostsTable::MaxCheckAttemptsAccessor, objectAccessor));
	table->AddColumn(prefix + "flap_detection_enabled", Column(&HostsTable::FlapDetectionEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "check_freshness", Column(&HostsTable::CheckFreshnessAccessor, objectAccessor));
	table->AddColumn(prefix + "process_performance_data", Column(&HostsTable::ProcessPerformanceDataAccessor, objectAccessor));
	table->AddColumn(prefix + "accept_passive_checks", Column(&HostsTable::AcceptPassiveChecksAccessor, objectAccessor));
	table->AddColumn(prefix + "event_handler_enabled", Column(&HostsTable::EventHandlerEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "acknowledgement_type", Column(&HostsTable::AcknowledgementTypeAccessor, objectAccessor));
	table->AddColumn(prefix + "check_type", Column(&HostsTable::CheckTypeAccessor, objectAccessor));
	table->AddColumn(prefix + "last_state", Column(&HostsTable::LastStateAccessor, objectAccessor));
	table->AddColumn(prefix + "last_hard_state", Column(&HostsTable::LastHardStateAccessor, objectAccessor));
	table->AddColumn(prefix + "current_attempt", Column(&HostsTable::CurrentAttemptAccessor, objectAccessor));
	table->AddColumn(prefix + "last_notification", Column(&HostsTable::LastNotificationAccessor, objectAccessor));
	table->AddColumn(prefix + "next_notification", Column(&HostsTable::NextNotificationAccessor, objectAccessor));
	table->AddColumn(prefix + "next_check", Column(&HostsTable::NextCheckAccessor, objectAccessor));
	table->AddColumn(prefix + "last_hard_state_change", Column(&HostsTable::LastHardStateChangeAccessor, objectAccessor));
	table->AddColumn(prefix + "has_been_checked", Column(&HostsTable::HasBeenCheckedAccessor, objectAccessor));
	table->AddColumn(prefix + "current_notification_number", Column(&HostsTable::CurrentNotificationNumberAccessor, objectAccessor));
	table->AddColumn(prefix + "pending_flex_downtime", Column(&HostsTable::PendingFlexDowntimeAccessor, objectAccessor));
	table->AddColumn(prefix + "total_services", Column(&HostsTable::TotalServicesAccessor, objectAccessor));
	table->AddColumn(prefix + "checks_enabled", Column(&HostsTable::ChecksEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "notifications_enabled", Column(&HostsTable::NotificationsEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "acknowledged", Column(&HostsTable::AcknowledgedAccessor, objectAccessor));
	table->AddColumn(prefix + "state", Column(&HostsTable::StateAccessor, objectAccessor));
	table->AddColumn(prefix + "state_type", Column(&HostsTable::StateTypeAccessor, objectAccessor));
	table->AddColumn(prefix + "no_more_notifications", Column(&HostsTable::NoMoreNotificationsAccessor, objectAccessor));
	table->AddColumn(prefix + "check_flapping_recovery_notification", Column(&HostsTable::CheckFlappingRecoveryNotificationAccessor, objectAccessor));
	table->AddColumn(prefix + "last_check", Column(&HostsTable::LastCheckAccessor, objectAccessor));
	table->AddColumn(prefix + "last_state_change", Column(&HostsTable::LastStateChangeAccessor, objectAccessor));
	table->AddColumn(prefix + "last_time_up", Column(&HostsTable::LastTimeUpAccessor, objectAccessor));
	table->AddColumn(prefix + "last_time_down", Column(&HostsTable::LastTimeDownAccessor, objectAccessor));
	table->AddColumn(prefix + "last_time_unreachable", Column(&HostsTable::LastTimeUnreachableAccessor, objectAccessor));
	table->AddColumn(prefix + "is_flapping", Column(&HostsTable::IsFlappingAccessor, objectAccessor));
	table->AddColumn(prefix + "scheduled_downtime_depth", Column(&HostsTable::ScheduledDowntimeDepthAccessor, objectAccessor));
	table->AddColumn(prefix + "is_executing", Column(&HostsTable::IsExecutingAccessor, objectAccessor));
	table->AddColumn(prefix + "active_checks_enabled", Column(&HostsTable::ActiveChecksEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "check_options", Column(&HostsTable::CheckOptionsAccessor, objectAccessor));
	table->AddColumn(prefix + "obsess_over_host", Column(&HostsTable::ObsessOverHostAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes", Column(&HostsTable::ModifiedAttributesAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes_list", Column(&HostsTable::ModifiedAttributesListAccessor, objectAccessor));
	table->AddColumn(prefix + "check_interval", Column(&HostsTable::CheckIntervalAccessor, objectAccessor));
	table->AddColumn(prefix + "retry_interval", Column(&HostsTable::RetryIntervalAccessor, objectAccessor));
	table->AddColumn(prefix + "notification_interval", Column(&HostsTable::NotificationIntervalAccessor, objectAccessor));
	table->AddColumn(prefix + "first_notification_delay", Column(&HostsTable::FirstNotificationDelayAccessor, objectAccessor));
	table->AddColumn(prefix + "low_flap_threshold", Column(&HostsTable::LowFlapThresholdAccessor, objectAccessor));
	table->AddColumn(prefix + "high_flap_threshold", Column(&HostsTable::HighFlapThresholdAccessor, objectAccessor));
	table->AddColumn(prefix + "x_3d", Column(&HostsTable::X3dAccessor, objectAccessor));
	table->AddColumn(prefix + "y_3d", Column(&HostsTable::Y3dAccessor, objectAccessor));
	table->AddColumn(prefix + "z_3d", Column(&HostsTable::Z3dAccessor, objectAccessor));
	table->AddColumn(prefix + "x_2d", Column(&HostsTable::X2dAccessor, objectAccessor));
	table->AddColumn(prefix + "y_2d", Column(&HostsTable::Y2dAccessor, objectAccessor));
	table->AddColumn(prefix + "latency", Column(&HostsTable::LatencyAccessor, objectAccessor));
	table->AddColumn(prefix + "execution_time", Column(&HostsTable::ExecutionTimeAccessor, objectAccessor));
	table->AddColumn(prefix + "percent_state_change", Column(&HostsTable::PercentStateChangeAccessor, objectAccessor));
	table->AddColumn(prefix + "in_notification_period", Column(&HostsTable::InNotificationPeriodAccessor, objectAccessor));
	table->AddColumn(prefix + "in_check_period", Column(&HostsTable::InCheckPeriodAccessor, objectAccessor));
	table->AddColumn(prefix + "contacts", Column(&HostsTable::ContactsAccessor, objectAccessor));
	table->AddColumn(prefix + "downtimes", Column(&HostsTable::DowntimesAccessor, objectAccessor));
	table->AddColumn(prefix + "downtimes_with_info", Column(&HostsTable::DowntimesWithInfoAccessor, objectAccessor));
	table->AddColumn(prefix + "comments", Column(&HostsTable::CommentsAccessor, objectAccessor));
	table->AddColumn(prefix + "comments_with_info", Column(&HostsTable::CommentsWithInfoAccessor, objectAccessor));
	table->AddColumn(prefix + "comments_with_extra_info", Column(&HostsTable::CommentsWithExtraInfoAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variable_names", Column(&HostsTable::CustomVariableNamesAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variable_values", Column(&HostsTable::CustomVariableValuesAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variables", Column(&HostsTable::CustomVariablesAccessor, objectAccessor));
	table->AddColumn(prefix + "filename", Column(&HostsTable::FilenameAccessor, objectAccessor));
	table->AddColumn(prefix + "parents", Column(&HostsTable::ParentsAccessor, objectAccessor));
	table->AddColumn(prefix + "childs", Column(&HostsTable::ChildsAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services", Column(&HostsTable::NumServicesAccessor, objectAccessor));
	table->AddColumn(prefix + "worst_service_state", Column(&HostsTable::WorstServiceStateAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_ok", Column(&HostsTable::NumServicesOkAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_warn", Column(&HostsTable::NumServicesWarnAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_crit", Column(&HostsTable::NumServicesCritAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_unknown", Column(&HostsTable::NumServicesUnknownAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_pending", Column(&HostsTable::NumServicesPendingAccessor, objectAccessor));
	table->AddColumn(prefix + "worst_service_hard_state", Column(&HostsTable::WorstServiceHardStateAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_hard_ok", Column(&HostsTable::NumServicesHardOkAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_hard_warn", Column(&HostsTable::NumServicesHardWarnAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_hard_crit", Column(&HostsTable::NumServicesHardCritAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_hard_unknown", Column(&HostsTable::NumServicesHardUnknownAccessor, objectAccessor));
	table->AddColumn(prefix + "hard_state", Column(&HostsTable::HardStateAccessor, objectAccessor));
	table->AddColumn(prefix + "pnpgraph_present", Column(&HostsTable::PnpgraphPresentAccessor, objectAccessor));
	table->AddColumn(prefix + "groups", Column(&HostsTable::GroupsAccessor, objectAccessor));
	table->AddColumn(prefix + "contact_groups", Column(&HostsTable::ContactGroupsAccessor, objectAccessor));
	table->AddColumn(prefix + "services", Column(&HostsTable::ServicesAccessor, objectAccessor));
	table->AddColumn(prefix + "services_with_state", Column(&HostsTable::ServicesWithStateAccessor, objectAccessor));
	table->AddColumn(prefix + "services_with_info", Column(&HostsTable::ServicesWithInfoAccessor, objectAccessor));
}

String HostsTable::GetName(void) const
{
	return "hosts";
}

void HostsTable::FetchRows(const AddRowFunction& addRowFn)
{
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Host")) {
		addRowFn(object);
	}
}

Value HostsTable::NameAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Host>(object)->GetName();
}

Value HostsTable::DisplayNameAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Host>(object)->GetDisplayName();
}

Value HostsTable::AddressAccessor(const Object::Ptr& object)
{
	Dictionary::Ptr macros = static_pointer_cast<Host>(object)->GetMacros();

	if (!macros)
		return Value();

	return macros->Get("address");
}

Value HostsTable::Address6Accessor(const Object::Ptr& object)
{
	Dictionary::Ptr macros = static_pointer_cast<Host>(object)->GetMacros();

	if (!macros)
		return Value();

	return macros->Get("address6");
}

Value HostsTable::CheckCommandAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	CheckCommand::Ptr checkcommand = hc->GetCheckCommand();
	if (checkcommand)
		return checkcommand->GetName(); /* this is the name without '!' args */

	return Value();
}

Value HostsTable::CheckCommandExpandedAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	CheckCommand::Ptr checkcommand = hc->GetCheckCommand();
	if (checkcommand)
		return checkcommand->GetName(); /* this is the name without '!' args */

	return Value();
}

Value HostsTable::EventHandlerAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	EventCommand::Ptr eventcommand = hc->GetEventCommand();
	if (eventcommand)
		return eventcommand->GetName();

	return Value();
}

Value HostsTable::NotificationPeriodAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	/* TODO hc->GetNotifications->(loop)->GetNotificationPeriod() */
	return Value();
}

Value HostsTable::CheckPeriodAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	TimePeriod::Ptr timeperiod = hc->GetCheckPeriod();

	if (!timeperiod)
		return Value();

	return timeperiod->GetName();
}

Value HostsTable::NotesAccessor(const Object::Ptr& object)
{
	Dictionary::Ptr custom = static_pointer_cast<Host>(object)->GetCustom();

	if (!custom)
		return Value();

	return custom->Get("notes");
}

Value HostsTable::NotesExpandedAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::NotesUrlAccessor(const Object::Ptr& object)
{
	Dictionary::Ptr custom = static_pointer_cast<Host>(object)->GetCustom();

	if (!custom)
		return Value();

	return custom->Get("notes_url");
}

Value HostsTable::NotesUrlExpandedAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::ActionUrlAccessor(const Object::Ptr& object)
{
	Dictionary::Ptr custom = static_pointer_cast<Host>(object)->GetCustom();

	if (!custom)
		return Value();

	return custom->Get("action_url");
}

Value HostsTable::ActionUrlExpandedAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::PluginOutputAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();
	String output;

	if(hc)
		output = hc->GetLastCheckOutput();

	return output;
}

Value HostsTable::PerfDataAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();
	String perfdata;

	if (hc)
		perfdata = hc->GetLastCheckPerfData();

	return perfdata;
}

Value HostsTable::IconImageAccessor(const Object::Ptr& object)
{
	Dictionary::Ptr custom = static_pointer_cast<Host>(object)->GetCustom();

	if (!custom)
		return Value();

	return custom->Get("icon_image");
}

Value HostsTable::IconImageExpandedAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::IconImageAltAccessor(const Object::Ptr& object)
{
	Dictionary::Ptr custom = static_pointer_cast<Host>(object)->GetCustom();

	if (!custom)
		return Value();

	return custom->Get("icon_image_alt");
}

Value HostsTable::StatusmapImageAccessor(const Object::Ptr& object)
{
	Dictionary::Ptr custom = static_pointer_cast<Host>(object)->GetCustom();

	if (!custom)
		return Value();

	return custom->Get("statusmap_image");
}

Value HostsTable::LongPluginOutputAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();
	String long_output;

	if (hc)
		long_output = hc->GetLastCheckLongOutput();

	return long_output;
}

Value HostsTable::InitialStateAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::MaxCheckAttemptsAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return hc->GetMaxCheckAttempts();
}

Value HostsTable::FlapDetectionEnabledAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return (hc->GetEnableFlapping() ? 1 : 0);
}

Value HostsTable::CheckFreshnessAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::ProcessPerformanceDataAccessor(const Object::Ptr& object)
{
	/* TODO always enabled */
	return Value(1);
}

Value HostsTable::AcceptPassiveChecksAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return (hc->GetEnablePassiveChecks() ? 1 : 0);
}

Value HostsTable::EventHandlerEnabledAccessor(const Object::Ptr& object)
{
	/* TODO always enabled */
	return Value(1);
}

Value HostsTable::AcknowledgementTypeAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	/* important: lock acknowledgements */
	ObjectLock olock(hc);

	return static_cast<int>(hc->GetAcknowledgement());
}

Value HostsTable::CheckTypeAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return (hc->GetEnableActiveChecks() ? 1 : 0);
}

Value HostsTable::LastStateAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Host>(object)->GetLastState();
}

Value HostsTable::LastHardStateAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Host>(object)->GetLastHardState();
}

Value HostsTable::CurrentAttemptAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return hc->GetCurrentCheckAttempt();
}

Value HostsTable::LastNotificationAccessor(const Object::Ptr& object)
{
	/* TODO Host->Service->GetNotifications->(loop)->GetLastNotification() */
	return Value();
}

Value HostsTable::NextNotificationAccessor(const Object::Ptr& object)
{
	/* TODO Host->Service->GetNotifications->(loop)->GetNextNotification() */
	return Value();
}

Value HostsTable::NextCheckAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return hc->GetNextCheck();
}

Value HostsTable::LastHardStateChangeAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return hc->GetLastHardStateChange();
}

Value HostsTable::HasBeenCheckedAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return (hc->HasBeenChecked() ? 1 : 0);
}

Value HostsTable::CurrentNotificationNumberAccessor(const Object::Ptr& object)
{
	/* TODO Host->Service->GetNotifications->(loop) new attribute */
	return Value();
}

Value HostsTable::PendingFlexDowntimeAccessor(const Object::Ptr& object)
{
	/* TODO Host->Service->GetDowntimes->(loop) type flexible? */
	return Value();
}

Value HostsTable::TotalServicesAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Host>(object)->GetTotalServices();
}

Value HostsTable::ChecksEnabledAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return (hc->GetEnableActiveChecks() ? 1 : 0);
}

Value HostsTable::NotificationsEnabledAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return (hc->GetEnableNotifications() ? 1 : 0);
}

Value HostsTable::AcknowledgedAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	/* important: lock acknowledgements */
	ObjectLock olock(hc);

	return (hc->IsAcknowledged() ? 1 : 0);
}

Value HostsTable::StateAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Host>(object)->GetState();
}

Value HostsTable::StateTypeAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Host>(object)->GetStateType();
}

Value HostsTable::NoMoreNotificationsAccessor(const Object::Ptr& object)
{
	/* TODO: notification_interval == 0, volatile == false */
	return Value(0);
}

Value HostsTable::CheckFlappingRecoveryNotificationAccessor(const Object::Ptr& object)
{
	/* TODO: if we're flapping, state != OK && notified once, set to true */
	return Value(0);
}

Value HostsTable::LastCheckAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return hc->GetLastCheck();
}

Value HostsTable::LastStateChangeAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Host>(object)->GetLastStateChange();
}

Value HostsTable::LastTimeUpAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::LastTimeDownAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::LastTimeUnreachableAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::IsFlappingAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return hc->IsFlapping();
}

Value HostsTable::ScheduledDowntimeDepthAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return hc->GetDowntimeDepth();
}

Value HostsTable::IsExecutingAccessor(const Object::Ptr& object)
{
	/* TODO does that make sense with Icinga2? */
	return Value();
}

Value HostsTable::ActiveChecksEnabledAccessor(const Object::Ptr& object)
{
	/* duplicate of ChecksEnableAccessor */
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return (hc->GetEnableActiveChecks() ? 1 : 0);
}

Value HostsTable::CheckOptionsAccessor(const Object::Ptr& object)
{
	/* TODO - forcexec, freshness, orphan, none */
	return Value();
}

Value HostsTable::ObsessOverHostAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value HostsTable::ModifiedAttributesAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value HostsTable::ModifiedAttributesListAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value HostsTable::CheckIntervalAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return (hc->GetCheckInterval() / 60.0);
}

Value HostsTable::RetryIntervalAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return (hc->GetRetryInterval() / 60.0);
}

Value HostsTable::NotificationIntervalAccessor(const Object::Ptr& object)
{
	/* TODO Host->Services->GetNotifications->(loop)->GetNotificationInterval() */
	return Value();
}

Value HostsTable::FirstNotificationDelayAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value HostsTable::LowFlapThresholdAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::HighFlapThresholdAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::X3dAccessor(const Object::Ptr& object)
{
	/* not supported - removed in Icinga 1.x */
	return Value();
}

Value HostsTable::Y3dAccessor(const Object::Ptr& object)
{
	/* not supported - removed in Icinga 1.x */
	return Value();
}

Value HostsTable::Z3dAccessor(const Object::Ptr& object)
{
	/* not supported - removed in Icinga 1.x */
	return Value();
}

Value HostsTable::X2dAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::Y2dAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::LatencyAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return (Service::CalculateLatency(hc->GetLastCheckResult()));
}

Value HostsTable::ExecutionTimeAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return (Service::CalculateExecutionTime(hc->GetLastCheckResult()));
}

Value HostsTable::PercentStateChangeAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return hc->GetFlappingCurrent();
}

Value HostsTable::InNotificationPeriodAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::InCheckPeriodAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::ContactsAccessor(const Object::Ptr& object)
{
	/* TODO - host->service->notifications->users */
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	std::set<User::Ptr> allUsers;
	std::set<User::Ptr> users;

	BOOST_FOREACH(const Notification::Ptr& notification, hc->GetNotifications()) {
		ObjectLock olock(notification);

		users = notification->GetUsers();

		std::copy(users.begin(), users.end(), std::inserter(allUsers, allUsers.begin()));

		BOOST_FOREACH(const UserGroup::Ptr& ug, notification->GetGroups()) {
			std::set<User::Ptr> members = ug->GetMembers();
			std::copy(members.begin(), members.end(), std::inserter(allUsers, allUsers.begin()));
		}
	}


	return Value();
}

Value HostsTable::DowntimesAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::DowntimesWithInfoAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::CommentsAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::CommentsWithInfoAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::CommentsWithExtraInfoAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::CustomVariableNamesAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::CustomVariableValuesAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::CustomVariablesAccessor(const Object::Ptr& object)
{
	/* TODO Dictionary */
	return Value();
}
Value HostsTable::FilenameAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::ParentsAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::ChildsAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::NumServicesAccessor(const Object::Ptr& object)
{
	/* duplicate of TotalServices */
	return static_pointer_cast<Host>(object)->GetTotalServices();
}

Value HostsTable::WorstServiceStateAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::NumServicesOkAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::NumServicesWarnAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::NumServicesCritAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::NumServicesUnknownAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::NumServicesPendingAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::WorstServiceHardStateAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::NumServicesHardOkAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::NumServicesHardWarnAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::NumServicesHardCritAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::NumServicesHardUnknownAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::HardStateAccessor(const Object::Ptr& object)
{
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	if (hc->GetState() == StateOK)
		return StateOK;
	if (hc->GetStateType() == StateTypeHard)
		return hc->GetState();

	return hc->GetLastHardState();
}

Value HostsTable::PnpgraphPresentAccessor(const Object::Ptr& object)
{
	/* wtf. not supported */
	return Value();
}

Value HostsTable::StalenessAccessor(const Object::Ptr& object)
{
	/* TODO time since last check normalized on the check_interval */
	return Value();
}

Value HostsTable::GroupsAccessor(const Object::Ptr& object)
{
	/* TODO create array */
	/* use hostcheck service */
	Service::Ptr hc = static_pointer_cast<Host>(object)->GetHostCheckService();

	if (!hc)
		return Value();

	return hc->GetGroups();
}

Value HostsTable::ContactGroupsAccessor(const Object::Ptr& object)
{
	/* TODO create array */
	return Value();
}

Value HostsTable::ServicesAccessor(const Object::Ptr& object)
{
	/* TODO create array */
	return Value();
}

/* dictionary */
Value HostsTable::ServicesWithStateAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value HostsTable::ServicesWithInfoAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

