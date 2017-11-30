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

#include "livestatus/hoststable.hpp"
#include "livestatus/hostgroupstable.hpp"
#include "livestatus/endpointstable.hpp"
#include "icinga/host.hpp"
#include "icinga/service.hpp"
#include "icinga/hostgroup.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/timeperiod.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/compatutility.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/json.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace icinga;

HostsTable::HostsTable(LivestatusGroupByType type)
    :Table(type)
{
	AddColumns(this);
}

void HostsTable::AddColumns(Table *table, const String& prefix,
    const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "name", Column(&HostsTable::NameAccessor, objectAccessor));
	table->AddColumn(prefix + "host_name", Column(&HostsTable::NameAccessor, objectAccessor)); //ugly compatibility hack
	table->AddColumn(prefix + "display_name", Column(&HostsTable::DisplayNameAccessor, objectAccessor));
	table->AddColumn(prefix + "alias", Column(&HostsTable::DisplayNameAccessor, objectAccessor));
	table->AddColumn(prefix + "address", Column(&HostsTable::AddressAccessor, objectAccessor));
	table->AddColumn(prefix + "address6", Column(&HostsTable::Address6Accessor, objectAccessor));
	table->AddColumn(prefix + "check_command", Column(&HostsTable::CheckCommandAccessor, objectAccessor));
	table->AddColumn(prefix + "check_command_expanded", Column(&HostsTable::CheckCommandExpandedAccessor, objectAccessor));
	table->AddColumn(prefix + "event_handler", Column(&HostsTable::EventHandlerAccessor, objectAccessor));
	table->AddColumn(prefix + "notification_period", Column(&Table::EmptyStringAccessor, objectAccessor));
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
	table->AddColumn(prefix + "statusmap_image", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "long_plugin_output", Column(&HostsTable::LongPluginOutputAccessor, objectAccessor));
	table->AddColumn(prefix + "initial_state", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "max_check_attempts", Column(&HostsTable::MaxCheckAttemptsAccessor, objectAccessor));
	table->AddColumn(prefix + "flap_detection_enabled", Column(&HostsTable::FlapDetectionEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "check_freshness", Column(&Table::OneAccessor, objectAccessor));
	table->AddColumn(prefix + "process_performance_data", Column(&Table::ZeroAccessor, objectAccessor));
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
	table->AddColumn(prefix + "modified_attributes", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes_list", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "check_interval", Column(&HostsTable::CheckIntervalAccessor, objectAccessor));
	table->AddColumn(prefix + "retry_interval", Column(&HostsTable::RetryIntervalAccessor, objectAccessor));
	table->AddColumn(prefix + "notification_interval", Column(&HostsTable::NotificationIntervalAccessor, objectAccessor));
	table->AddColumn(prefix + "first_notification_delay", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "low_flap_threshold", Column(&HostsTable::LowFlapThresholdAccessor, objectAccessor));
	table->AddColumn(prefix + "high_flap_threshold", Column(&HostsTable::HighFlapThresholdAccessor, objectAccessor));
	table->AddColumn(prefix + "x_3d", Column(&EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "y_3d", Column(&EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "z_3d", Column(&EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "x_2d", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "y_2d", Column(&Table::EmptyStringAccessor, objectAccessor));
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
	table->AddColumn(prefix + "check_source", Column(&HostsTable::CheckSourceAccessor, objectAccessor));
	table->AddColumn(prefix + "is_reachable", Column(&HostsTable::IsReachableAccessor, objectAccessor));
	table->AddColumn(prefix + "cv_is_json", Column(&HostsTable::CVIsJsonAccessor, objectAccessor));
	table->AddColumn(prefix + "original_attributes", Column(&HostsTable::OriginalAttributesAccessor, objectAccessor));

	/* add additional group by values received through the object accessor */
	if (table->GetGroupByType() == LivestatusGroupByHostGroup) {
		/* _1 = row, _2 = groupByType, _3 = groupByObject */
		Log(LogDebug, "Livestatus")
		    << "Processing hosts group by hostgroup table.";
		HostGroupsTable::AddColumns(table, "hostgroup_", std::bind(&HostsTable::HostGroupAccessor, _1, _2, _3));
	}
}

String HostsTable::GetName(void) const
{
	return "hosts";
}

String HostsTable::GetPrefix(void) const
{
	return "host";
}

void HostsTable::FetchRows(const AddRowFunction& addRowFn)
{
	if (GetGroupByType() == LivestatusGroupByHostGroup) {
		for (const HostGroup::Ptr& hg : ConfigType::GetObjectsByType<HostGroup>()) {
			for (const Host::Ptr& host : hg->GetMembers()) {
				/* the caller must know which groupby type and value are set for this row */
				if (!addRowFn(host, LivestatusGroupByHostGroup, hg))
					return;
			}
		}
	} else {
		for (const Host::Ptr& host : ConfigType::GetObjectsByType<Host>()) {
			if (!addRowFn(host, LivestatusGroupByNone, Empty))
				return;
		}
	}
}

Object::Ptr HostsTable::HostGroupAccessor(const Value& row, LivestatusGroupByType groupByType, const Object::Ptr& groupByObject)
{
	/* return the current group by value set from within FetchRows()
	 * this is the hostgrouo object used for the table join inside
	 * in AddColumns()
	 */
	if (groupByType == LivestatusGroupByHostGroup)
		return groupByObject;

	return Object::Ptr();
}

Value HostsTable::NameAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetName();
}

Value HostsTable::DisplayNameAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetDisplayName();
}

Value HostsTable::AddressAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetAddress();
}

Value HostsTable::Address6Accessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetAddress6();
}

Value HostsTable::CheckCommandAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	CheckCommand::Ptr checkcommand = host->GetCheckCommand();
	if (checkcommand)
		return CompatUtility::GetCommandName(checkcommand) + "!" + CompatUtility::GetCheckableCommandArgs(host);

	return Empty;
}

Value HostsTable::CheckCommandExpandedAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	CheckCommand::Ptr checkcommand = host->GetCheckCommand();
	if (checkcommand)
		return CompatUtility::GetCommandName(checkcommand) + "!" + CompatUtility::GetCheckableCommandArgs(host);

	return Empty;
}

Value HostsTable::EventHandlerAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	EventCommand::Ptr eventcommand = host->GetEventCommand();
	if (eventcommand)
		return CompatUtility::GetCommandName(eventcommand);

	return Empty;
}

Value HostsTable::CheckPeriodAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableCheckPeriod(host);
}

Value HostsTable::NotesAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetNotes();
}

Value HostsTable::NotesExpandedAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	MacroProcessor::ResolverList resolvers {
	    { "host", host },
	    { "icinga", IcingaApplication::GetInstance() }
	};

	return MacroProcessor::ResolveMacros(host->GetNotes(), resolvers, CheckResult::Ptr());
}

Value HostsTable::NotesUrlAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetNotesUrl();
}

Value HostsTable::NotesUrlExpandedAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	MacroProcessor::ResolverList resolvers {
	    { "host", host },
	    { "icinga", IcingaApplication::GetInstance() }
	};

	return MacroProcessor::ResolveMacros(host->GetNotesUrl(), resolvers);
}

Value HostsTable::ActionUrlAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetActionUrl();
}

Value HostsTable::ActionUrlExpandedAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	MacroProcessor::ResolverList resolvers {
	    { "host", host },
	    { "icinga", IcingaApplication::GetInstance() }
	};

	return MacroProcessor::ResolveMacros(host->GetActionUrl(), resolvers);
}

Value HostsTable::PluginOutputAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	String output;
	CheckResult::Ptr cr = host->GetLastCheckResult();

	if (cr)
		output = CompatUtility::GetCheckResultOutput(cr);

	return output;
}

Value HostsTable::PerfDataAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	String perfdata;
	CheckResult::Ptr cr = host->GetLastCheckResult();

	if (cr)
		perfdata = CompatUtility::GetCheckResultPerfdata(cr);

	return perfdata;
}

Value HostsTable::IconImageAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetIconImage();
}

Value HostsTable::IconImageExpandedAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	MacroProcessor::ResolverList resolvers {
	    { "host", host },
	    { "icinga", IcingaApplication::GetInstance() }
	};

	return MacroProcessor::ResolveMacros(host->GetIconImage(), resolvers);
}

Value HostsTable::IconImageAltAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetIconImageAlt();
}

Value HostsTable::LongPluginOutputAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	String long_output;
	CheckResult::Ptr cr = host->GetLastCheckResult();

	if (cr)
		long_output = CompatUtility::GetCheckResultLongOutput(cr);

	return long_output;
}

Value HostsTable::MaxCheckAttemptsAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetMaxCheckAttempts();
}

Value HostsTable::FlapDetectionEnabledAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableFlapDetectionEnabled(host);
}

Value HostsTable::AcceptPassiveChecksAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckablePassiveChecksEnabled(host);
}

Value HostsTable::EventHandlerEnabledAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableEventHandlerEnabled(host);
}

Value HostsTable::AcknowledgementTypeAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	ObjectLock olock(host);
	return CompatUtility::GetCheckableAcknowledgementType(host);
}

Value HostsTable::CheckTypeAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableCheckType(host);
}

Value HostsTable::LastStateAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetLastState();
}

Value HostsTable::LastHardStateAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetLastHardState();
}

Value HostsTable::CurrentAttemptAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetCheckAttempt();
}

Value HostsTable::LastNotificationAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableNotificationLastNotification(host);
}

Value HostsTable::NextNotificationAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableNotificationNextNotification(host);
}

Value HostsTable::NextCheckAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return static_cast<int>(host->GetNextCheck());
}

Value HostsTable::LastHardStateChangeAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return static_cast<int>(host->GetLastHardStateChange());
}

Value HostsTable::HasBeenCheckedAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableHasBeenChecked(host);
}

Value HostsTable::CurrentNotificationNumberAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableNotificationNotificationNumber(host);
}

Value HostsTable::TotalServicesAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetTotalServices();
}

Value HostsTable::ChecksEnabledAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableActiveChecksEnabled(host);
}

Value HostsTable::NotificationsEnabledAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableNotificationsEnabled(host);
}

Value HostsTable::AcknowledgedAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	ObjectLock olock(host);
	return CompatUtility::GetCheckableIsAcknowledged(host);
}

Value HostsTable::StateAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->IsReachable() ? host->GetState() : 2;
}

Value HostsTable::StateTypeAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetStateType();
}

Value HostsTable::NoMoreNotificationsAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableNoMoreNotifications(host);
}

Value HostsTable::LastCheckAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return static_cast<int>(host->GetLastCheck());
}

Value HostsTable::LastStateChangeAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return static_cast<int>(host->GetLastStateChange());
}

Value HostsTable::LastTimeUpAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return static_cast<int>(host->GetLastStateUp());
}

Value HostsTable::LastTimeDownAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return static_cast<int>(host->GetLastStateDown());
}

Value HostsTable::LastTimeUnreachableAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return static_cast<int>(host->GetLastStateUnreachable());
}

Value HostsTable::IsFlappingAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->IsFlapping();
}

Value HostsTable::ScheduledDowntimeDepthAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetDowntimeDepth();
}

Value HostsTable::ActiveChecksEnabledAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableActiveChecksEnabled(host);
}

Value HostsTable::CheckOptionsAccessor(const Value&)
{
	/* TODO - forcexec, freshness, orphan, none */
	return Empty;
}

Value HostsTable::CheckIntervalAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableCheckInterval(host);
}

Value HostsTable::RetryIntervalAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableRetryInterval(host);
}

Value HostsTable::NotificationIntervalAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableNotificationNotificationInterval(host);
}

Value HostsTable::LowFlapThresholdAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableLowFlapThreshold(host);
}

Value HostsTable::HighFlapThresholdAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableHighFlapThreshold(host);
}

Value HostsTable::LatencyAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	CheckResult::Ptr cr = host->GetLastCheckResult();

	if (!cr)
		return Empty;

	return cr->CalculateLatency();
}

Value HostsTable::ExecutionTimeAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	CheckResult::Ptr cr = host->GetLastCheckResult();

	if (!cr)
		return Empty;

	return cr->CalculateExecutionTime();
}

Value HostsTable::PercentStateChangeAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckablePercentStateChange(host);
}

Value HostsTable::InNotificationPeriodAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableInNotificationPeriod(host);
}

Value HostsTable::InCheckPeriodAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableInCheckPeriod(host);
}

Value HostsTable::ContactsAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Array::Ptr contact_names = new Array();

	for (const User::Ptr& user : CompatUtility::GetCheckableNotificationUsers(host)) {
		contact_names->Add(user->GetName());
	}

	return contact_names;
}

Value HostsTable::DowntimesAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Array::Ptr results = new Array();

	for (const Downtime::Ptr& downtime : host->GetDowntimes()) {
		if (downtime->IsExpired())
			continue;

		results->Add(downtime->GetLegacyId());
	}

	return results;
}

Value HostsTable::DowntimesWithInfoAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Array::Ptr results = new Array();

	for (const Downtime::Ptr& downtime : host->GetDowntimes()) {
		if (downtime->IsExpired())
			continue;

		Array::Ptr downtime_info = new Array();
		downtime_info->Add(downtime->GetLegacyId());
		downtime_info->Add(downtime->GetAuthor());
		downtime_info->Add(downtime->GetComment());
		results->Add(downtime_info);
	}

	return results;
}

Value HostsTable::CommentsAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Array::Ptr results = new Array();
	for (const Comment::Ptr& comment : host->GetComments()) {
		if (comment->IsExpired())
			continue;

		results->Add(comment->GetLegacyId());
	}

	return results;
}

Value HostsTable::CommentsWithInfoAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Array::Ptr results = new Array();

	for (const Comment::Ptr& comment : host->GetComments()) {
		if (comment->IsExpired())
			continue;

		Array::Ptr comment_info = new Array();
		comment_info->Add(comment->GetLegacyId());
		comment_info->Add(comment->GetAuthor());
		comment_info->Add(comment->GetText());
		results->Add(comment_info);
	}

	return results;
}

Value HostsTable::CommentsWithExtraInfoAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Array::Ptr results = new Array();

	for (const Comment::Ptr& comment : host->GetComments()) {
		if (comment->IsExpired())
			continue;

		Array::Ptr comment_info = new Array();
		comment_info->Add(comment->GetLegacyId());
		comment_info->Add(comment->GetAuthor());
		comment_info->Add(comment->GetText());
		comment_info->Add(comment->GetEntryType());
		comment_info->Add(static_cast<int>(comment->GetEntryTime()));
		results->Add(comment_info);
	}

	return results;
}

Value HostsTable::CustomVariableNamesAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Dictionary::Ptr vars;

	{
		ObjectLock olock(host);
		vars = CompatUtility::GetCustomAttributeConfig(host);
	}

	Array::Ptr cv = new Array();

	if (!vars)
		return cv;

	ObjectLock olock(vars);
	for (const Dictionary::Pair& kv : vars) {
		cv->Add(kv.first);
	}

	return cv;
}

Value HostsTable::CustomVariableValuesAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Dictionary::Ptr vars;

	{
		ObjectLock olock(host);
		vars = CompatUtility::GetCustomAttributeConfig(host);
	}

	Array::Ptr cv = new Array();

	if (!vars)
		return cv;

	ObjectLock olock(vars);
	for (const Dictionary::Pair& kv : vars) {
		if (kv.second.IsObjectType<Array>() || kv.second.IsObjectType<Dictionary>())
			cv->Add(JsonEncode(kv.second));
		else
			cv->Add(kv.second);
	}

	return cv;
}

Value HostsTable::CustomVariablesAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Dictionary::Ptr vars;

	{
		ObjectLock olock(host);
		vars = CompatUtility::GetCustomAttributeConfig(host);
	}

	Array::Ptr cv = new Array();

	if (!vars)
		return cv;

	ObjectLock olock(vars);
	for (const Dictionary::Pair& kv : vars) {
		Array::Ptr key_val = new Array();
		key_val->Add(kv.first);

		if (kv.second.IsObjectType<Array>() || kv.second.IsObjectType<Dictionary>())
			key_val->Add(JsonEncode(kv.second));
		else
			key_val->Add(kv.second);

		cv->Add(key_val);
	}

	return cv;
}

Value HostsTable::CVIsJsonAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Dictionary::Ptr vars;

	{
		ObjectLock olock(host);
		vars = CompatUtility::GetCustomAttributeConfig(host);
	}

	if (!vars)
		return Empty;

	bool cv_is_json = false;

	ObjectLock olock(vars);
	for (const Dictionary::Pair& kv : vars) {
		if (kv.second.IsObjectType<Array>() || kv.second.IsObjectType<Dictionary>())
			cv_is_json = true;
	}

	return cv_is_json;
}

Value HostsTable::ParentsAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Array::Ptr parents = new Array();

	for (const Checkable::Ptr& parent : host->GetParents()) {
		Host::Ptr parent_host = dynamic_pointer_cast<Host>(parent);

		if (!parent_host)
			continue;

		parents->Add(parent_host->GetName());
	}

	return parents;
}

Value HostsTable::ChildsAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Array::Ptr childs = new Array();

	for (const Checkable::Ptr& child : host->GetChildren()) {
		Host::Ptr child_host = dynamic_pointer_cast<Host>(child);

		if (!child_host)
			continue;

		childs->Add(child_host->GetName());
	}

	return childs;
}

Value HostsTable::NumServicesAccessor(const Value& row)
{
	/* duplicate of TotalServices */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetTotalServices();
}

Value HostsTable::WorstServiceStateAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Value worst_service = ServiceOK;

	for (const Service::Ptr& service : host->GetServices()) {
		if (service->GetState() > worst_service)
			worst_service = service->GetState();
	}

	return worst_service;
}

Value HostsTable::NumServicesOkAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	int num_services = 0;

	for (const Service::Ptr& service : host->GetServices()) {
		if (service->GetState() == ServiceOK)
			num_services++;
	}

	return num_services;
}

Value HostsTable::NumServicesWarnAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	int num_services = 0;

	for (const Service::Ptr& service : host->GetServices()) {
		if (service->GetState() == ServiceWarning)
			num_services++;
	}

	return num_services;
}

Value HostsTable::NumServicesCritAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	int num_services = 0;

	for (const Service::Ptr& service : host->GetServices()) {
		if (service->GetState() == ServiceCritical)
			num_services++;
	}

	return num_services;
}

Value HostsTable::NumServicesUnknownAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	int num_services = 0;

	for (const Service::Ptr& service : host->GetServices()) {
		if (service->GetState() == ServiceUnknown)
			num_services++;
	}

	return num_services;
}

Value HostsTable::NumServicesPendingAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	int num_services = 0;

	for (const Service::Ptr& service : host->GetServices()) {
		if (!service->GetLastCheckResult())
			num_services++;
	}

	return num_services;
}

Value HostsTable::WorstServiceHardStateAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Value worst_service = ServiceOK;

	for (const Service::Ptr& service : host->GetServices()) {
		if (service->GetStateType() == StateTypeHard) {
			if (service->GetState() > worst_service)
				worst_service = service->GetState();
		}
	}

	return worst_service;
}

Value HostsTable::NumServicesHardOkAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	int num_services = 0;

	for (const Service::Ptr& service : host->GetServices()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == ServiceOK)
			num_services++;
	}

	return num_services;
}

Value HostsTable::NumServicesHardWarnAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	int num_services = 0;

	for (const Service::Ptr& service : host->GetServices()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == ServiceWarning)
			num_services++;
	}

	return num_services;
}

Value HostsTable::NumServicesHardCritAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	int num_services = 0;

	for (const Service::Ptr& service : host->GetServices()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == ServiceCritical)
			num_services++;
	}

	return num_services;
}

Value HostsTable::NumServicesHardUnknownAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	int num_services = 0;

	for (const Service::Ptr& service : host->GetServices()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == ServiceUnknown)
			num_services++;
	}

	return num_services;
}

Value HostsTable::HardStateAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	if (host->GetState() == HostUp)
		return HostUp;
	else if (host->GetStateType() == StateTypeHard)
		return host->GetState();

	return host->GetLastHardState();
}

Value HostsTable::StalenessAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCheckableStaleness(host);
}

Value HostsTable::GroupsAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Array::Ptr groups = host->GetGroups();

	if (!groups)
		return Empty;

	return groups;
}

Value HostsTable::ContactGroupsAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Array::Ptr contactgroup_names = new Array();

	for (const UserGroup::Ptr& usergroup : CompatUtility::GetCheckableNotificationUserGroups(host)) {
		contactgroup_names->Add(usergroup->GetName());
	}

	return contactgroup_names;
}

Value HostsTable::ServicesAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	std::vector<Service::Ptr> rservices = host->GetServices();

	Array::Ptr services = new Array();
	services->Reserve(rservices.size());

	for (const Service::Ptr& service : rservices) {
		services->Add(service->GetShortName());
	}

	return services;
}

Value HostsTable::ServicesWithStateAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	std::vector<Service::Ptr> rservices = host->GetServices();

	Array::Ptr services = new Array();
	services->Reserve(rservices.size());

	for (const Service::Ptr& service : rservices) {
		Array::Ptr svc_add = new Array();

		svc_add->Add(service->GetShortName());
		svc_add->Add(service->GetState());
		svc_add->Add(service->HasBeenChecked() ? 1 : 0);
		services->Add(svc_add);
	}

	return services;
}

Value HostsTable::ServicesWithInfoAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	std::vector<Service::Ptr> rservices = host->GetServices();

	Array::Ptr services = new Array();
	services->Reserve(rservices.size());

	for (const Service::Ptr& service : rservices) {
		Array::Ptr svc_add = new Array();

		svc_add->Add(service->GetShortName());
		svc_add->Add(service->GetState());
		svc_add->Add(service->HasBeenChecked() ? 1 : 0);

		String output;
		CheckResult::Ptr cr = service->GetLastCheckResult();

		if (cr)
			output = CompatUtility::GetCheckResultOutput(cr);

		svc_add->Add(output);
		services->Add(svc_add);
	}

	return services;
}

Value HostsTable::CheckSourceAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	CheckResult::Ptr cr = host->GetLastCheckResult();

	if (cr)
		return cr->GetCheckSource();

	return Empty;
}

Value HostsTable::IsReachableAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->IsReachable();
}

Value HostsTable::OriginalAttributesAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return JsonEncode(host->GetOriginalAttributes());
}
