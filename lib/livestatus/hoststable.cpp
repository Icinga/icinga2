/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
#include "icinga/pluginutility.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/json.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
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
	table->AddColumn(prefix + "check_options", Column(&Table::EmptyStringAccessor, objectAccessor));
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
		HostGroupsTable::AddColumns(table, "hostgroup_", [](const Value& row, LivestatusGroupByType groupByType, const Object::Ptr& groupByObject) -> Value {
			return HostGroupAccessor(row, groupByType, groupByObject);
		});
	}
}

String HostsTable::GetName() const
{
	return "hosts";
}

String HostsTable::GetPrefix() const
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

	return nullptr;
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

	TimePeriod::Ptr checkPeriod = host->GetCheckPeriod();

	if (!checkPeriod)
		return Empty;

	return checkPeriod->GetName();
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

	return MacroProcessor::ResolveMacros(host->GetNotes(), resolvers);
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

	if (!cr)
		return Empty;

	return PluginUtility::FormatPerfdata(cr->GetPerformanceData());
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

	return Convert::ToLong(host->GetEnableFlapping());
}

Value HostsTable::AcceptPassiveChecksAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return Convert::ToLong(host->GetEnablePassiveChecks());
}

Value HostsTable::EventHandlerEnabledAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return Convert::ToLong(host->GetEnableEventHandler());
}

Value HostsTable::AcknowledgementTypeAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	ObjectLock olock(host);
	return host->GetAcknowledgement();
}

Value HostsTable::CheckTypeAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return (host->GetEnableActiveChecks() ? 0 : 1); /* 0 .. active, 1 .. passive */
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

	return Convert::ToLong(host->HasBeenChecked());
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

	return Convert::ToLong(host->GetEnableActiveChecks());
}

Value HostsTable::NotificationsEnabledAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return Convert::ToLong(host->GetEnableNotifications());
}

Value HostsTable::ProcessPerformanceDataAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return Convert::ToLong(host->GetEnablePerfdata());
}

Value HostsTable::AcknowledgedAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	ObjectLock olock(host);
	return host->IsAcknowledged();
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

	return (CompatUtility::GetCheckableNotificationNotificationInterval(host) == 0 && !host->GetVolatile()) ? 1 : 0;
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

	return Convert::ToLong(host->GetEnableActiveChecks());
}

Value HostsTable::CheckIntervalAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetCheckInterval() / LIVESTATUS_INTERVAL_LENGTH;
}

Value HostsTable::RetryIntervalAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetRetryInterval() / LIVESTATUS_INTERVAL_LENGTH;
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

	return host->GetFlappingThresholdLow();
}

Value HostsTable::HighFlapThresholdAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetFlappingThresholdHigh();
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

	return host->GetFlappingCurrent();
}

Value HostsTable::InNotificationPeriodAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	for (const Notification::Ptr& notification : host->GetNotifications()) {
		TimePeriod::Ptr timeperiod = notification->GetPeriod();

		if (!timeperiod || timeperiod->IsInside(Utility::GetTime()))
			return 1;
	}

	return 0;
}

Value HostsTable::InCheckPeriodAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	TimePeriod::Ptr timeperiod = host->GetCheckPeriod();

	/* none set means always checked */
	if (!timeperiod)
		return 1;

	return Convert::ToLong(timeperiod->IsInside(Utility::GetTime()));
}

Value HostsTable::ContactsAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	ArrayData result;

	for (const User::Ptr& user : CompatUtility::GetCheckableNotificationUsers(host)) {
		result.push_back(user->GetName());
	}

	return new Array(std::move(result));
}

Value HostsTable::DowntimesAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	ArrayData result;

	for (const Downtime::Ptr& downtime : host->GetDowntimes()) {
		if (downtime->IsExpired())
			continue;

		result.push_back(downtime->GetLegacyId());
	}

	return new Array(std::move(result));
}

Value HostsTable::DowntimesWithInfoAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	ArrayData result;

	for (const Downtime::Ptr& downtime : host->GetDowntimes()) {
		if (downtime->IsExpired())
			continue;

		result.push_back(new Array({
			downtime->GetLegacyId(),
			downtime->GetAuthor(),
			downtime->GetComment()
		}));
	}

	return new Array(std::move(result));
}

Value HostsTable::CommentsAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	ArrayData result;

	for (const Comment::Ptr& comment : host->GetComments()) {
		if (comment->IsExpired())
			continue;

		result.push_back(comment->GetLegacyId());
	}

	return new Array(std::move(result));
}

Value HostsTable::CommentsWithInfoAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	ArrayData result;

	for (const Comment::Ptr& comment : host->GetComments()) {
		if (comment->IsExpired())
			continue;

		result.push_back(new Array({
			comment->GetLegacyId(),
			comment->GetAuthor(),
			comment->GetText()
		}));
	}

	return new Array(std::move(result));
}

Value HostsTable::CommentsWithExtraInfoAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	ArrayData result;

	for (const Comment::Ptr& comment : host->GetComments()) {
		if (comment->IsExpired())
			continue;

		result.push_back(new Array({
			comment->GetLegacyId(),
			comment->GetAuthor(),
			comment->GetText(),
			comment->GetEntryType(),
			static_cast<int>(comment->GetEntryTime())
		}));
	}

	return new Array(std::move(result));
}

Value HostsTable::CustomVariableNamesAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Dictionary::Ptr vars = host->GetVars();

	ArrayData result;

	if (vars) {
		ObjectLock olock(vars);
		for (const Dictionary::Pair& kv : vars) {
			result.push_back(kv.first);
		}
	}

	return new Array(std::move(result));
}

Value HostsTable::CustomVariableValuesAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Dictionary::Ptr vars = host->GetVars();

	ArrayData result;

	if (vars) {
		ObjectLock olock(vars);
		for (const Dictionary::Pair& kv : vars) {
			if (kv.second.IsObjectType<Array>() || kv.second.IsObjectType<Dictionary>())
				result.push_back(JsonEncode(kv.second));
			else
				result.push_back(kv.second);
		}
	}

	return new Array(std::move(result));
}

Value HostsTable::CustomVariablesAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Dictionary::Ptr vars = host->GetVars();

	ArrayData result;

	if (vars) {
		ObjectLock olock(vars);
		for (const Dictionary::Pair& kv : vars) {
			Value val;

			if (kv.second.IsObjectType<Array>() || kv.second.IsObjectType<Dictionary>())
				val = JsonEncode(kv.second);
			else
				val = kv.second;

			result.push_back(new Array({
				kv.first,
				val
			}));
		}
	}

	return new Array(std::move(result));
}

Value HostsTable::CVIsJsonAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Dictionary::Ptr vars = host->GetVars();

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

	ArrayData result;

	for (const Checkable::Ptr& parent : host->GetParents()) {
		Host::Ptr parent_host = dynamic_pointer_cast<Host>(parent);

		if (!parent_host)
			continue;

		result.push_back(parent_host->GetName());
	}

	return new Array(std::move(result));
}

Value HostsTable::ChildsAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	ArrayData result;

	for (const Checkable::Ptr& child : host->GetChildren()) {
		Host::Ptr child_host = dynamic_pointer_cast<Host>(child);

		if (!child_host)
			continue;

		result.push_back(child_host->GetName());
	}

	return new Array(std::move(result));
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

	if (host->HasBeenChecked() && host->GetLastCheck() > 0)
		return (Utility::GetTime() - host->GetLastCheck()) / (host->GetCheckInterval() * 3600);

	return 0.0;
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

	ArrayData result;

	for (const UserGroup::Ptr& usergroup : CompatUtility::GetCheckableNotificationUserGroups(host)) {
		result.push_back(usergroup->GetName());
	}

	return new Array(std::move(result));
}

Value HostsTable::ServicesAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	std::vector<Service::Ptr> rservices = host->GetServices();

	ArrayData result;
	result.reserve(rservices.size());

	for (const Service::Ptr& service : rservices) {
		result.push_back(service->GetShortName());
	}

	return new Array(std::move(result));
}

Value HostsTable::ServicesWithStateAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	std::vector<Service::Ptr> rservices = host->GetServices();

	ArrayData result;
	result.reserve(rservices.size());

	for (const Service::Ptr& service : rservices) {
		result.push_back(new Array({
			service->GetShortName(),
			service->GetState(),
			service->HasBeenChecked() ? 1 : 0
		}));
	}

	return new Array(std::move(result));
}

Value HostsTable::ServicesWithInfoAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	std::vector<Service::Ptr> rservices = host->GetServices();

	ArrayData result;
	result.reserve(rservices.size());

	for (const Service::Ptr& service : rservices) {
		String output;
		CheckResult::Ptr cr = service->GetLastCheckResult();

		if (cr)
			output = CompatUtility::GetCheckResultOutput(cr);

		result.push_back(new Array({
			service->GetShortName(),
			service->GetState(),
			service->HasBeenChecked() ? 1 : 0,
			output
		}));
	}

	return new Array(std::move(result));
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
