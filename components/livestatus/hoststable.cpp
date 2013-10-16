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

#include "livestatus/hoststable.h"
#include "icinga/host.h"
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
#include <boost/algorithm/string/classification.hpp>
#include <boost/foreach.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>

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
	table->AddColumn(prefix + "initial_state", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "max_check_attempts", Column(&HostsTable::MaxCheckAttemptsAccessor, objectAccessor));
	table->AddColumn(prefix + "flap_detection_enabled", Column(&HostsTable::FlapDetectionEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "check_freshness", Column(&Table::OneAccessor, objectAccessor));
	table->AddColumn(prefix + "process_performance_data", Column(&Table::OneAccessor, objectAccessor));
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
	table->AddColumn(prefix + "pending_flex_downtime", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "total_services", Column(&HostsTable::TotalServicesAccessor, objectAccessor));
	table->AddColumn(prefix + "checks_enabled", Column(&HostsTable::ChecksEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "notifications_enabled", Column(&HostsTable::NotificationsEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "acknowledged", Column(&HostsTable::AcknowledgedAccessor, objectAccessor));
	table->AddColumn(prefix + "state", Column(&HostsTable::StateAccessor, objectAccessor));
	table->AddColumn(prefix + "state_type", Column(&HostsTable::StateTypeAccessor, objectAccessor));
	table->AddColumn(prefix + "no_more_notifications", Column(&HostsTable::NoMoreNotificationsAccessor, objectAccessor));
	table->AddColumn(prefix + "check_flapping_recovery_notification", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "last_check", Column(&HostsTable::LastCheckAccessor, objectAccessor));
	table->AddColumn(prefix + "last_state_change", Column(&HostsTable::LastStateChangeAccessor, objectAccessor));
	table->AddColumn(prefix + "last_time_up", Column(&HostsTable::LastTimeUpAccessor, objectAccessor));
	table->AddColumn(prefix + "last_time_down", Column(&HostsTable::LastTimeDownAccessor, objectAccessor));
	table->AddColumn(prefix + "last_time_unreachable", Column(&HostsTable::LastTimeUnreachableAccessor, objectAccessor));
	table->AddColumn(prefix + "is_flapping", Column(&HostsTable::IsFlappingAccessor, objectAccessor));
	table->AddColumn(prefix + "scheduled_downtime_depth", Column(&HostsTable::ScheduledDowntimeDepthAccessor, objectAccessor));
	table->AddColumn(prefix + "is_executing", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "active_checks_enabled", Column(&HostsTable::ActiveChecksEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "check_options", Column(&HostsTable::CheckOptionsAccessor, objectAccessor));
	table->AddColumn(prefix + "obsess_over_host", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes", Column(&HostsTable::ModifiedAttributesAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes_list", Column(&HostsTable::ModifiedAttributesListAccessor, objectAccessor));
	table->AddColumn(prefix + "check_interval", Column(&HostsTable::CheckIntervalAccessor, objectAccessor));
	table->AddColumn(prefix + "retry_interval", Column(&HostsTable::RetryIntervalAccessor, objectAccessor));
	table->AddColumn(prefix + "notification_interval", Column(&HostsTable::NotificationIntervalAccessor, objectAccessor));
	table->AddColumn(prefix + "first_notification_delay", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "low_flap_threshold", Column(&HostsTable::LowFlapThresholdAccessor, objectAccessor));
	table->AddColumn(prefix + "high_flap_threshold", Column(&HostsTable::HighFlapThresholdAccessor, objectAccessor));
	table->AddColumn(prefix + "x_3d", Column(&EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "y_3d", Column(&EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "z_3d", Column(&EmptyStringAccessor, objectAccessor));
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
	table->AddColumn(prefix + "filename", Column(&Table::EmptyStringAccessor, objectAccessor));
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
	table->AddColumn(prefix + "pnpgraph_present", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "staleness", Column(&HostsTable::StalenessAccessor, objectAccessor));
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
	BOOST_FOREACH(const Host::Ptr& host, DynamicType::GetObjects<Host>()) {
		addRowFn(host);
	}
}

Value HostsTable::NameAccessor(const Value& row)
{
	return static_cast<Host::Ptr>(row)->GetName();
}

Value HostsTable::DisplayNameAccessor(const Value& row)
{
	return static_cast<Host::Ptr>(row)->GetDisplayName();
}

Value HostsTable::AddressAccessor(const Value& row)
{
	Dictionary::Ptr macros = static_cast<Host::Ptr>(row)->GetMacros();

	if (!macros)
		return Empty;

	return macros->Get("address");
}

Value HostsTable::Address6Accessor(const Value& row)
{
	Dictionary::Ptr macros = static_cast<Host::Ptr>(row)->GetMacros();

	if (!macros)
		return Empty;

	return macros->Get("address6");
}

Value HostsTable::CheckCommandAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	CheckCommand::Ptr checkcommand = hc->GetCheckCommand();
	if (checkcommand)
		return checkcommand->GetName(); /* this is the name without '!' args */

	return Empty;
}

Value HostsTable::CheckCommandExpandedAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);
	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	CheckCommand::Ptr commandObj = hc->GetCheckCommand();

	if (!commandObj)
		return Empty;

	Value raw_command = commandObj->GetCommandLine();

	std::vector<MacroResolver::Ptr> resolvers;
	resolvers.push_back(hc);
	resolvers.push_back(host);
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

Value HostsTable::EventHandlerAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	EventCommand::Ptr eventcommand = hc->GetEventCommand();
	if (eventcommand)
		return eventcommand->GetName();

	return Empty;
}

Value HostsTable::NotificationPeriodAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	BOOST_FOREACH(const Notification::Ptr& notification, hc->GetNotifications()) {
		ObjectLock olock(notification);

		TimePeriod::Ptr timeperiod = notification->GetNotificationPeriod();

		/* XXX first notification wins */
		if (timeperiod)
			return timeperiod->GetName();
	}

	return Empty;
}

Value HostsTable::CheckPeriodAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	TimePeriod::Ptr timeperiod = hc->GetCheckPeriod();

	if (!timeperiod)
		return Empty;

	return timeperiod->GetName();
}

Value HostsTable::NotesAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<Host::Ptr>(row)->GetCustom();

	if (!custom)
		return Empty;

	return custom->Get("notes");
}

Value HostsTable::NotesExpandedAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);
	Service::Ptr service = host->GetCheckService();
	Dictionary::Ptr custom = host->GetCustom();

	if (!custom)
		return Empty;

	std::vector<MacroResolver::Ptr> resolvers;

	if (service)
		resolvers.push_back(service);

	resolvers.push_back(host);
	resolvers.push_back(IcingaApplication::GetInstance());

	Value value = custom->Get("notes");

	Dictionary::Ptr cr;
	Value value_expanded = MacroProcessor::ResolveMacros(value, resolvers, cr, Utility::EscapeShellCmd);

	return value_expanded;
}

Value HostsTable::NotesUrlAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<Host::Ptr>(row)->GetCustom();

	if (!custom)
		return Empty;

	return custom->Get("notes_url");
}

Value HostsTable::NotesUrlExpandedAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);
	Service::Ptr service = host->GetCheckService();
	Dictionary::Ptr custom = host->GetCustom();

	if (!custom)
		return Empty;

	std::vector<MacroResolver::Ptr> resolvers;

	if (service)
		resolvers.push_back(service);

	resolvers.push_back(host);
	resolvers.push_back(IcingaApplication::GetInstance());

	Value value = custom->Get("notes_url");

	Dictionary::Ptr cr;
	Value value_expanded = MacroProcessor::ResolveMacros(value, resolvers, cr, Utility::EscapeShellCmd);

	return value_expanded;
}

Value HostsTable::ActionUrlAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<Host::Ptr>(row)->GetCustom();

	if (!custom)
		return Empty;

	return custom->Get("action_url");
}

Value HostsTable::ActionUrlExpandedAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);
	Service::Ptr service = host->GetCheckService();
	Dictionary::Ptr custom = host->GetCustom();

	if (!custom)
		return Empty;

	std::vector<MacroResolver::Ptr> resolvers;

	if (service)
		resolvers.push_back(service);

	resolvers.push_back(host);
	resolvers.push_back(IcingaApplication::GetInstance());

	Value value = custom->Get("action_url");

	Dictionary::Ptr cr;
	Value value_expanded = MacroProcessor::ResolveMacros(value, resolvers, cr, Utility::EscapeShellCmd);

	return value_expanded;
}

Value HostsTable::PluginOutputAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();
	String output;

	if(hc)
		output = hc->GetLastCheckOutput();

	return output;
}

Value HostsTable::PerfDataAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();
	String perfdata;

	if (hc)
		perfdata = hc->GetLastCheckPerfData();

	return perfdata;
}

Value HostsTable::IconImageAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<Host::Ptr>(row)->GetCustom();

	if (!custom)
		return Empty;

	return custom->Get("icon_image");
}

Value HostsTable::IconImageExpandedAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);
	Service::Ptr service = host->GetCheckService();
	Dictionary::Ptr custom = host->GetCustom();

	if (!custom)
		return Empty;

	std::vector<MacroResolver::Ptr> resolvers;

	if (service)
		resolvers.push_back(service);

	resolvers.push_back(host);
	resolvers.push_back(IcingaApplication::GetInstance());

	Value value = custom->Get("icon_image");

	Dictionary::Ptr cr;
	Value value_expanded = MacroProcessor::ResolveMacros(value, resolvers, cr, Utility::EscapeShellCmd);

	return value_expanded;
}

Value HostsTable::IconImageAltAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<Host::Ptr>(row)->GetCustom();

	if (!custom)
		return Empty;

	return custom->Get("icon_image_alt");
}

Value HostsTable::StatusmapImageAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<Host::Ptr>(row)->GetCustom();

	if (!custom)
		return Empty;

	return custom->Get("statusmap_image");
}

Value HostsTable::LongPluginOutputAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();
	String long_output;

	if (hc)
		long_output = hc->GetLastCheckLongOutput();

	return long_output;
}

Value HostsTable::MaxCheckAttemptsAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return hc->GetMaxCheckAttempts();
}

Value HostsTable::FlapDetectionEnabledAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return (hc->GetEnableFlapping() ? 1 : 0);
}

Value HostsTable::AcceptPassiveChecksAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return (hc->GetEnablePassiveChecks() ? 1 : 0);
}

Value HostsTable::EventHandlerEnabledAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	EventCommand::Ptr eventcommand = hc->GetEventCommand();
	if (eventcommand)
		return 1;

	return 0;
}

Value HostsTable::AcknowledgementTypeAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	/* important: lock acknowledgements */
	ObjectLock olock(hc);

	return static_cast<int>(hc->GetAcknowledgement());
}

Value HostsTable::CheckTypeAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return (hc->GetEnableActiveChecks() ? 0 : 1);
}

Value HostsTable::LastStateAccessor(const Value& row)
{
	return static_cast<Host::Ptr>(row)->GetLastState();
}

Value HostsTable::LastHardStateAccessor(const Value& row)
{
	return static_cast<Host::Ptr>(row)->GetLastHardState();
}

Value HostsTable::CurrentAttemptAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return hc->GetCurrentCheckAttempt();
}

Value HostsTable::LastNotificationAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	/* XXX Service -> Notifications, latest wins */
	double last_notification = 0;
	BOOST_FOREACH(const Notification::Ptr& notification, hc->GetNotifications()) {
		if (notification->GetLastNotification() > last_notification)
			last_notification = notification->GetLastNotification();
	}

	return static_cast<int>(last_notification);
}

Value HostsTable::NextNotificationAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	/* XXX Service -> Notifications, latest wins */
	double next_notification = 0;
	BOOST_FOREACH(const Notification::Ptr& notification, hc->GetNotifications()) {
		if (notification->GetNextNotification() < next_notification)
			next_notification = notification->GetNextNotification();
	}

	return static_cast<int>(next_notification);
}

Value HostsTable::NextCheckAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return static_cast<int>(hc->GetNextCheck());
}

Value HostsTable::LastHardStateChangeAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return static_cast<int>(hc->GetLastHardStateChange());
}

Value HostsTable::HasBeenCheckedAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return (hc->HasBeenChecked() ? 1 : 0);
}

Value HostsTable::CurrentNotificationNumberAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

        /* XXX Service -> Notifications, biggest wins */
        int notification_number = 0;
        BOOST_FOREACH(const Notification::Ptr& notification, hc->GetNotifications()) {
                if (notification->GetNotificationNumber() > notification_number)
                        notification_number = notification->GetNotificationNumber();
        }

        return notification_number;

}

Value HostsTable::TotalServicesAccessor(const Value& row)
{
	return static_cast<Host::Ptr>(row)->GetTotalServices();
}

Value HostsTable::ChecksEnabledAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return (hc->GetEnableActiveChecks() ? 1 : 0);
}

Value HostsTable::NotificationsEnabledAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return (hc->GetEnableNotifications() ? 1 : 0);
}

Value HostsTable::AcknowledgedAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	/* important: lock acknowledgements */
	ObjectLock olock(hc);

	return (hc->IsAcknowledged() ? 1 : 0);
}

Value HostsTable::StateAccessor(const Value& row)
{
	return static_cast<Host::Ptr>(row)->GetState();
}

Value HostsTable::StateTypeAccessor(const Value& row)
{
	return static_cast<Host::Ptr>(row)->GetStateType();
}

Value HostsTable::NoMoreNotificationsAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	/* XXX take the smallest notification_interval */
	double notification_interval = -1;
	BOOST_FOREACH(const Notification::Ptr& notification, hc->GetNotifications()) {
		if (notification_interval == -1 || notification->GetNotificationInterval() < notification_interval)
			notification_interval = notification->GetNotificationInterval();
	}

	if (notification_interval == 0 && !hc->IsVolatile())
		return 1;

	return 0;
}

Value HostsTable::LastCheckAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return static_cast<int>(hc->GetLastCheck());
}

Value HostsTable::LastStateChangeAccessor(const Value& row)
{
	return static_cast<int>(static_cast<Host::Ptr>(row)->GetLastStateChange());
}

Value HostsTable::LastTimeUpAccessor(const Value& row)
{
	return static_cast<int>(static_cast<Host::Ptr>(row)->GetLastStateUp());
}

Value HostsTable::LastTimeDownAccessor(const Value& row)
{
	return static_cast<int>(static_cast<Host::Ptr>(row)->GetLastStateDown());
}

Value HostsTable::LastTimeUnreachableAccessor(const Value& row)
{
	return static_cast<int>(static_cast<Host::Ptr>(row)->GetLastStateUnreachable());
}

Value HostsTable::IsFlappingAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return hc->IsFlapping();
}

Value HostsTable::ScheduledDowntimeDepthAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return hc->GetDowntimeDepth();
}

Value HostsTable::ActiveChecksEnabledAccessor(const Value& row)
{
	/* duplicate of ChecksEnableAccessor */
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return (hc->GetEnableActiveChecks() ? 1 : 0);
}

Value HostsTable::CheckOptionsAccessor(const Value& row)
{
	/* TODO - forcexec, freshness, orphan, none */
	return Empty;
}

Value HostsTable::ModifiedAttributesAccessor(const Value& row)
{
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return hc->GetModifiedAttributes();
}

Value HostsTable::ModifiedAttributesListAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value HostsTable::CheckIntervalAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return (hc->GetCheckInterval() / 60.0);
}

Value HostsTable::RetryIntervalAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return (hc->GetRetryInterval() / 60.0);
}

Value HostsTable::NotificationIntervalAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	/* XXX take the smallest notification_interval */
	double notification_interval = -1;
	BOOST_FOREACH(const Notification::Ptr& notification, hc->GetNotifications()) {
		if (notification_interval == -1 || notification->GetNotificationInterval() < notification_interval)
			notification_interval = notification->GetNotificationInterval();
	}

	if (notification_interval == -1)
		notification_interval = 60;

	return (notification_interval / 60.0);
}

Value HostsTable::LowFlapThresholdAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return hc->GetFlappingThreshold();
}

Value HostsTable::HighFlapThresholdAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return hc->GetFlappingThreshold();
}

Value HostsTable::X2dAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<Host::Ptr>(row)->GetCustom();

	if (!custom)
		return Empty;

	String coords = custom->Get("2d_coords");

	std::vector<String> tokens;
	boost::algorithm::split(tokens, coords, boost::is_any_of(","));

	if (tokens.size() != 2)
		return Empty;

	return tokens[0];
}

Value HostsTable::Y2dAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<Host::Ptr>(row)->GetCustom();

	if (!custom)
		return Empty;

	String coords = custom->Get("2d_coords");

	std::vector<String> tokens;
	boost::algorithm::split(tokens, coords, boost::is_any_of(","));

	if (tokens.size() != 2)
		return Empty;

	return tokens[1];
}

Value HostsTable::LatencyAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return (Service::CalculateLatency(hc->GetLastCheckResult()));
}

Value HostsTable::ExecutionTimeAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return (Service::CalculateExecutionTime(hc->GetLastCheckResult()));
}

Value HostsTable::PercentStateChangeAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	return hc->GetFlappingCurrent();
}

Value HostsTable::InNotificationPeriodAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	BOOST_FOREACH(const Notification::Ptr& notification, hc->GetNotifications()) {
		ObjectLock olock(notification);

		TimePeriod::Ptr timeperiod = notification->GetNotificationPeriod();

		/* XXX first notification wins */
		if (timeperiod)
			return (timeperiod->IsInside(Utility::GetTime()) ? 1 : 0);
	}

	/* none set means always notified */
	return 1;
}

Value HostsTable::InCheckPeriodAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	TimePeriod::Ptr timeperiod = hc->GetCheckPeriod();

	/* none set means always checked */
	if (!timeperiod)
		return 1;

	return (timeperiod->IsInside(Utility::GetTime()) ? 1 : 0);
}

Value HostsTable::ContactsAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	Array::Ptr contact_names = boost::make_shared<Array>();

	BOOST_FOREACH(const User::Ptr& user, CompatUtility::GetServiceNotificationUsers(hc)) {
		contact_names->Add(user->GetName());
	}

	return contact_names;
}

Value HostsTable::DowntimesAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	Dictionary::Ptr downtimes = hc->GetDowntimes();

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

Value HostsTable::DowntimesWithInfoAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	Dictionary::Ptr downtimes = hc->GetDowntimes();

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

Value HostsTable::CommentsAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	Dictionary::Ptr comments = hc->GetComments();

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

Value HostsTable::CommentsWithInfoAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	Dictionary::Ptr comments = hc->GetComments();

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

Value HostsTable::CommentsWithExtraInfoAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	Dictionary::Ptr comments = hc->GetComments();

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

Value HostsTable::CustomVariableNamesAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<Host::Ptr>(row)->GetCustom();

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

Value HostsTable::CustomVariableValuesAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<Host::Ptr>(row)->GetCustom();

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

Value HostsTable::CustomVariablesAccessor(const Value& row)
{
	Dictionary::Ptr custom = static_cast<Host::Ptr>(row)->GetCustom();

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

Value HostsTable::ParentsAccessor(const Value& row)
{
	Array::Ptr parents = boost::make_shared<Array>();

	BOOST_FOREACH(const Host::Ptr& parent, static_cast<Host::Ptr>(row)->GetParentHosts()) {
		parents->Add(parent->GetName());
	}

	return parents;
}

Value HostsTable::ChildsAccessor(const Value& row)
{
	Array::Ptr childs = boost::make_shared<Array>();

	BOOST_FOREACH(const Host::Ptr& child, static_cast<Host::Ptr>(row)->GetChildHosts()) {
		childs->Add(child->GetName());
	}

	return childs;
}

Value HostsTable::NumServicesAccessor(const Value& row)
{
	/* duplicate of TotalServices */
	return static_cast<Host::Ptr>(row)->GetTotalServices();
}

Value HostsTable::WorstServiceStateAccessor(const Value& row)
{
	Value worst_service = StateOK;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<Host::Ptr>(row)->GetServices()) {
		if (service->GetState() > worst_service)
			worst_service = service->GetState();
	}

	return worst_service;
}

Value HostsTable::NumServicesOkAccessor(const Value& row)
{
	int num_services = 0;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<Host::Ptr>(row)->GetServices()) {
		if (service->GetState() == StateOK)
			num_services++;
	}

	return num_services;
}

Value HostsTable::NumServicesWarnAccessor(const Value& row)
{
	int num_services = 0;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<Host::Ptr>(row)->GetServices()) {
		if (service->GetState() == StateWarning)
			num_services++;
	}

	return num_services;
}

Value HostsTable::NumServicesCritAccessor(const Value& row)
{
	int num_services = 0;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<Host::Ptr>(row)->GetServices()) {
		if (service->GetState() == StateCritical)
			num_services++;
	}

	return num_services;
}

Value HostsTable::NumServicesUnknownAccessor(const Value& row)
{
	int num_services = 0;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<Host::Ptr>(row)->GetServices()) {
		if (service->GetState() == StateUnknown)
			num_services++;
	}

	return num_services;
}

Value HostsTable::NumServicesPendingAccessor(const Value& row)
{
	int num_services = 0;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<Host::Ptr>(row)->GetServices()) {
		if (!service->GetLastCheckResult())
			num_services++;
	}

	return num_services;
}

Value HostsTable::WorstServiceHardStateAccessor(const Value& row)
{
	Value worst_service = StateOK;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<Host::Ptr>(row)->GetServices()) {
		if (service->GetStateType() == StateTypeHard) {
			if (service->GetState() > worst_service)
				worst_service = service->GetState();
		}
	}

	return worst_service;
}

Value HostsTable::NumServicesHardOkAccessor(const Value& row)
{
	int num_services = 0;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<Host::Ptr>(row)->GetServices()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == StateOK)
			num_services++;
	}

	return num_services;
}

Value HostsTable::NumServicesHardWarnAccessor(const Value& row)
{
	int num_services = 0;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<Host::Ptr>(row)->GetServices()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == StateWarning)
			num_services++;
	}

	return num_services;
}

Value HostsTable::NumServicesHardCritAccessor(const Value& row)
{
	int num_services = 0;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<Host::Ptr>(row)->GetServices()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == StateCritical)
			num_services++;
	}

	return num_services;
}

Value HostsTable::NumServicesHardUnknownAccessor(const Value& row)
{
	int num_services = 0;

	BOOST_FOREACH(const Service::Ptr& service, static_cast<Host::Ptr>(row)->GetServices()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == StateUnknown)
			num_services++;
	}

	return num_services;
}

Value HostsTable::HardStateAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	if (hc->GetState() == StateOK)
		return StateOK;
	if (hc->GetStateType() == StateTypeHard)
		return hc->GetState();

	return hc->GetLastHardState();
}

Value HostsTable::StalenessAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	if (hc->HasBeenChecked() && hc->GetLastCheck() > 0)
		return (Utility::GetTime() - hc->GetLastCheck()) / (hc->GetCheckInterval() * 3600);

	return Empty;
}

Value HostsTable::GroupsAccessor(const Value& row)
{
	Array::Ptr groups = static_cast<Host::Ptr>(row)->GetGroups();

	if (!groups)
		return Empty;

	return groups;
}

Value HostsTable::ContactGroupsAccessor(const Value& row)
{
	/* use hostcheck service */
	Service::Ptr hc = static_cast<Host::Ptr>(row)->GetCheckService();

	if (!hc)
		return Empty;

	Array::Ptr contactgroup_names = boost::make_shared<Array>();

	BOOST_FOREACH(const UserGroup::Ptr& usergroup, CompatUtility::GetServiceNotificationUserGroups(hc)) {
		contactgroup_names->Add(usergroup->GetName());
	}

	return contactgroup_names;
}

Value HostsTable::ServicesAccessor(const Value& row)
{
	Array::Ptr services = boost::make_shared<Array>();

	BOOST_FOREACH(const Service::Ptr& service, static_cast<Host::Ptr>(row)->GetServices()) {
		services->Add(service->GetShortName());
	}

	return services;
}

Value HostsTable::ServicesWithStateAccessor(const Value& row)
{
	Array::Ptr services = boost::make_shared<Array>();

	BOOST_FOREACH(const Service::Ptr& service, static_cast<Host::Ptr>(row)->GetServices()) {
		Array::Ptr svc_add = boost::make_shared<Array>();

		svc_add->Add(service->GetShortName());
		svc_add->Add(service->GetState());
		svc_add->Add(service->HasBeenChecked() ? 1 : 0);
		services->Add(svc_add);
	}

	return services;
}

Value HostsTable::ServicesWithInfoAccessor(const Value& row)
{
	Array::Ptr services = boost::make_shared<Array>();

	BOOST_FOREACH(const Service::Ptr& service, static_cast<Host::Ptr>(row)->GetServices()) {
		Array::Ptr svc_add = boost::make_shared<Array>();

		svc_add->Add(service->GetShortName());
		svc_add->Add(service->GetState());
		svc_add->Add(service->HasBeenChecked() ? 1 : 0);
		svc_add->Add(service->GetLastCheckOutput());
		services->Add(svc_add);
	}

	return services;
}

