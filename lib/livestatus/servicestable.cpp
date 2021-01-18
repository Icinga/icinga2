/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "livestatus/servicestable.hpp"
#include "livestatus/hoststable.hpp"
#include "livestatus/servicegroupstable.hpp"
#include "livestatus/hostgroupstable.hpp"
#include "livestatus/endpointstable.hpp"
#include "icinga/service.hpp"
#include "icinga/servicegroup.hpp"
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

ServicesTable::ServicesTable(LivestatusGroupByType type)
	: Table(type)
{
	AddColumns(this);
}


void ServicesTable::AddColumns(Table *table, const String& prefix,
	const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "description", Column(&ServicesTable::ShortNameAccessor, objectAccessor));
	table->AddColumn(prefix + "service_description", Column(&ServicesTable::ShortNameAccessor, objectAccessor)); //ugly compatibility hack
	table->AddColumn(prefix + "display_name", Column(&ServicesTable::DisplayNameAccessor, objectAccessor));
	table->AddColumn(prefix + "check_command", Column(&ServicesTable::CheckCommandAccessor, objectAccessor));
	table->AddColumn(prefix + "check_command_expanded", Column(&ServicesTable::CheckCommandExpandedAccessor, objectAccessor));
	table->AddColumn(prefix + "event_handler", Column(&ServicesTable::EventHandlerAccessor, objectAccessor));
	table->AddColumn(prefix + "plugin_output", Column(&ServicesTable::PluginOutputAccessor, objectAccessor));
	table->AddColumn(prefix + "long_plugin_output", Column(&ServicesTable::LongPluginOutputAccessor, objectAccessor));
	table->AddColumn(prefix + "perf_data", Column(&ServicesTable::PerfDataAccessor, objectAccessor));
	table->AddColumn(prefix + "notification_period", Column(&Table::EmptyStringAccessor, objectAccessor));
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
	table->AddColumn(prefix + "process_performance_data", Column(&ServicesTable::ProcessPerformanceDataAccessor, objectAccessor));
	table->AddColumn(prefix + "is_executing", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "active_checks_enabled", Column(&ServicesTable::ActiveChecksEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "check_options", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "flap_detection_enabled", Column(&ServicesTable::FlapDetectionEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "check_freshness", Column(&Table::OneAccessor, objectAccessor));
	table->AddColumn(prefix + "obsess_over_service", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes_list", Column(&Table::ZeroAccessor, objectAccessor));
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
	table->AddColumn(prefix + "check_source", Column(&ServicesTable::CheckSourceAccessor, objectAccessor));
	table->AddColumn(prefix + "is_reachable", Column(&ServicesTable::IsReachableAccessor, objectAccessor));
	table->AddColumn(prefix + "cv_is_json", Column(&ServicesTable::CVIsJsonAccessor, objectAccessor));
	table->AddColumn(prefix + "original_attributes", Column(&ServicesTable::OriginalAttributesAccessor, objectAccessor));

	HostsTable::AddColumns(table, "host_", [objectAccessor](const Value& row, LivestatusGroupByType, const Object::Ptr&) -> Value {
		return HostAccessor(row, objectAccessor);
	});

	/* add additional group by values received through the object accessor */
	if (table->GetGroupByType() == LivestatusGroupByServiceGroup) {
		/* _1 = row, _2 = groupByType, _3 = groupByObject */
		Log(LogDebug, "Livestatus")
			<< "Processing services group by servicegroup table.";
		ServiceGroupsTable::AddColumns(table, "servicegroup_", [](const Value& row, LivestatusGroupByType groupByType, const Object::Ptr& groupByObject) -> Value {
			return ServiceGroupAccessor(row, groupByType, groupByObject);
		});
	} else if (table->GetGroupByType() == LivestatusGroupByHostGroup) {
		/* _1 = row, _2 = groupByType, _3 = groupByObject */
		Log(LogDebug, "Livestatus")
			<< "Processing services group by hostgroup table.";
		HostGroupsTable::AddColumns(table, "hostgroup_", [](const Value& row, LivestatusGroupByType groupByType, const Object::Ptr& groupByObject) -> Value {
			return HostGroupAccessor(row, groupByType, groupByObject);
		});
	}
}

String ServicesTable::GetName() const
{
	return "services";
}

String ServicesTable::GetPrefix() const
{
	return "service";
}

void ServicesTable::FetchRows(const AddRowFunction& addRowFn)
{
	if (GetGroupByType() == LivestatusGroupByServiceGroup) {
		for (const ServiceGroup::Ptr& sg : ConfigType::GetObjectsByType<ServiceGroup>()) {
			for (const Service::Ptr& service : sg->GetMembers()) {
				/* the caller must know which groupby type and value are set for this row */
				if (!addRowFn(service, LivestatusGroupByServiceGroup, sg))
					return;
			}
		}
	} else if (GetGroupByType() == LivestatusGroupByHostGroup) {
		for (const HostGroup::Ptr& hg : ConfigType::GetObjectsByType<HostGroup>()) {
			ObjectLock ylock(hg);
			for (const Host::Ptr& host : hg->GetMembers()) {
				ObjectLock ylock(host);
				for (const Service::Ptr& service : host->GetServices()) {
					/* the caller must know which groupby type and value are set for this row */
					if (!addRowFn(service, LivestatusGroupByHostGroup, hg))
						return;
				}
			}
		}
	} else {
		for (const Service::Ptr& service : ConfigType::GetObjectsByType<Service>()) {
			if (!addRowFn(service, LivestatusGroupByNone, Empty))
				return;
		}
	}
}

Object::Ptr ServicesTable::HostAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor)
{
	Value service;

	if (parentObjectAccessor)
		service = parentObjectAccessor(row, LivestatusGroupByNone, Empty);
	else
		service = row;

	Service::Ptr svc = static_cast<Service::Ptr>(service);

	if (!svc)
		return nullptr;

	return svc->GetHost();
}

Object::Ptr ServicesTable::ServiceGroupAccessor(const Value& row, LivestatusGroupByType groupByType, const Object::Ptr& groupByObject)
{
	/* return the current group by value set from within FetchRows()
	 * this is the servicegroup object used for the table join inside
	 * in AddColumns()
	 */
	if (groupByType == LivestatusGroupByServiceGroup)
		return groupByObject;

	return nullptr;
}

Object::Ptr ServicesTable::HostGroupAccessor(const Value& row, LivestatusGroupByType groupByType, const Object::Ptr& groupByObject)
{
	/* return the current group by value set from within FetchRows()
	 * this is the servicegroup object used for the table join inside
	 * in AddColumns()
	 */
	if (groupByType == LivestatusGroupByHostGroup)
		return groupByObject;

	return nullptr;
}

Value ServicesTable::ShortNameAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->GetShortName();
}

Value ServicesTable::DisplayNameAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->GetDisplayName();
}

Value ServicesTable::CheckCommandAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	CheckCommand::Ptr checkcommand = service->GetCheckCommand();

	if (checkcommand)
		return CompatUtility::GetCommandName(checkcommand) + "!" + CompatUtility::GetCheckableCommandArgs(service);

	return Empty;
}

Value ServicesTable::CheckCommandExpandedAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	CheckCommand::Ptr checkcommand = service->GetCheckCommand();

	if (checkcommand)
		return CompatUtility::GetCommandName(checkcommand) + "!" + CompatUtility::GetCheckableCommandArgs(service);

	return Empty;
}

Value ServicesTable::EventHandlerAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	EventCommand::Ptr eventcommand = service->GetEventCommand();

	if (eventcommand)
		return CompatUtility::GetCommandName(eventcommand);

	return Empty;
}

Value ServicesTable::PluginOutputAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	String output;
	CheckResult::Ptr cr = service->GetLastCheckResult();

	if (cr)
		output = CompatUtility::GetCheckResultOutput(cr);

	return output;
}

Value ServicesTable::LongPluginOutputAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	String long_output;
	CheckResult::Ptr cr = service->GetLastCheckResult();

	if (cr)
		long_output = CompatUtility::GetCheckResultLongOutput(cr);

	return long_output;
}

Value ServicesTable::PerfDataAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	String perfdata;
	CheckResult::Ptr cr = service->GetLastCheckResult();

	if (!cr)
		return Empty;

	return PluginUtility::FormatPerfdata(cr->GetPerformanceData());
}

Value ServicesTable::CheckPeriodAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	TimePeriod::Ptr checkPeriod = service->GetCheckPeriod();

	if (!checkPeriod)
		return Empty;

	return checkPeriod->GetName();
}

Value ServicesTable::NotesAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->GetNotes();
}

Value ServicesTable::NotesExpandedAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	MacroProcessor::ResolverList resolvers {
		{ "service", service },
		{ "host", service->GetHost() },
		{ "icinga", IcingaApplication::GetInstance() }
	};

	return MacroProcessor::ResolveMacros(service->GetNotes(), resolvers);
}

Value ServicesTable::NotesUrlAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->GetNotesUrl();
}

Value ServicesTable::NotesUrlExpandedAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	MacroProcessor::ResolverList resolvers {
		{ "service", service },
		{ "host", service->GetHost() },
		{ "icinga", IcingaApplication::GetInstance() }
	};

	return MacroProcessor::ResolveMacros(service->GetNotesUrl(), resolvers);
}

Value ServicesTable::ActionUrlAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->GetActionUrl();
}

Value ServicesTable::ActionUrlExpandedAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	MacroProcessor::ResolverList resolvers {
		{ "service", service },
		{ "host", service->GetHost() },
		{ "icinga", IcingaApplication::GetInstance() }
	};

	return MacroProcessor::ResolveMacros(service->GetActionUrl(), resolvers);
}

Value ServicesTable::IconImageAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->GetIconImage();
}

Value ServicesTable::IconImageExpandedAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	MacroProcessor::ResolverList resolvers {
		{ "service", service },
		{ "host", service->GetHost() },
		{ "icinga", IcingaApplication::GetInstance() }
	};

	return MacroProcessor::ResolveMacros(service->GetIconImage(), resolvers);
}

Value ServicesTable::IconImageAltAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->GetIconImageAlt();
}

Value ServicesTable::MaxCheckAttemptsAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->GetMaxCheckAttempts();
}

Value ServicesTable::CurrentAttemptAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->GetCheckAttempt();
}

Value ServicesTable::StateAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->GetState();
}

Value ServicesTable::HasBeenCheckedAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return Convert::ToLong(service->HasBeenChecked());
}

Value ServicesTable::LastStateAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->GetLastState();
}

Value ServicesTable::LastHardStateAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->GetLastHardState();
}

Value ServicesTable::StateTypeAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->GetStateType();
}

Value ServicesTable::CheckTypeAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return (service->GetEnableActiveChecks() ? 0 : 1); /* 0 .. active, 1 .. passive */
}

Value ServicesTable::AcknowledgedAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	ObjectLock olock(service);
	return service->IsAcknowledged();
}

Value ServicesTable::AcknowledgementTypeAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	ObjectLock olock(service);
	return service->GetAcknowledgement();
}

Value ServicesTable::NoMoreNotificationsAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return (CompatUtility::GetCheckableNotificationNotificationInterval(service) == 0 && !service->GetVolatile()) ? 1 : 0;
}

Value ServicesTable::LastTimeOkAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return static_cast<int>(service->GetLastStateOK());
}

Value ServicesTable::LastTimeWarningAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return static_cast<int>(service->GetLastStateWarning());
}

Value ServicesTable::LastTimeCriticalAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return static_cast<int>(service->GetLastStateCritical());
}

Value ServicesTable::LastTimeUnknownAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return static_cast<int>(service->GetLastStateUnknown());
}

Value ServicesTable::LastCheckAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return static_cast<int>(service->GetLastCheck());
}

Value ServicesTable::NextCheckAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return static_cast<int>(service->GetNextCheck());
}

Value ServicesTable::LastNotificationAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return CompatUtility::GetCheckableNotificationLastNotification(service);
}

Value ServicesTable::NextNotificationAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return CompatUtility::GetCheckableNotificationNextNotification(service);
}

Value ServicesTable::CurrentNotificationNumberAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return CompatUtility::GetCheckableNotificationNotificationNumber(service);
}

Value ServicesTable::LastStateChangeAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return static_cast<int>(service->GetLastStateChange());
}

Value ServicesTable::LastHardStateChangeAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return static_cast<int>(service->GetLastHardStateChange());
}

Value ServicesTable::ScheduledDowntimeDepthAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->GetDowntimeDepth();
}

Value ServicesTable::IsFlappingAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->IsFlapping();
}

Value ServicesTable::ChecksEnabledAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return Convert::ToLong(service->GetEnableActiveChecks());
}

Value ServicesTable::AcceptPassiveChecksAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return Convert::ToLong(service->GetEnablePassiveChecks());
}

Value ServicesTable::EventHandlerEnabledAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return Convert::ToLong(service->GetEnableEventHandler());
}

Value ServicesTable::NotificationsEnabledAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return Convert::ToLong(service->GetEnableNotifications());
}

Value ServicesTable::ProcessPerformanceDataAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return Convert::ToLong(service->GetEnablePerfdata());
}

Value ServicesTable::ActiveChecksEnabledAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return Convert::ToLong(service->GetEnableActiveChecks());
}

Value ServicesTable::FlapDetectionEnabledAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return Convert::ToLong(service->GetEnableFlapping());
}

Value ServicesTable::StalenessAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	if (service->HasBeenChecked() && service->GetLastCheck() > 0)
		return (Utility::GetTime() - service->GetLastCheck()) / (service->GetCheckInterval() * 3600);

	return 0.0;
}

Value ServicesTable::CheckIntervalAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->GetCheckInterval() / LIVESTATUS_INTERVAL_LENGTH;
}

Value ServicesTable::RetryIntervalAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->GetRetryInterval() / LIVESTATUS_INTERVAL_LENGTH;
}

Value ServicesTable::NotificationIntervalAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return CompatUtility::GetCheckableNotificationNotificationInterval(service);
}

Value ServicesTable::LowFlapThresholdAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->GetFlappingThresholdLow();
}

Value ServicesTable::HighFlapThresholdAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->GetFlappingThresholdHigh();
}

Value ServicesTable::LatencyAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	CheckResult::Ptr cr = service->GetLastCheckResult();

	if (!cr)
		return Empty;

	return cr->CalculateLatency();
}

Value ServicesTable::ExecutionTimeAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	CheckResult::Ptr cr = service->GetLastCheckResult();

	if (!cr)
		return Empty;

	return cr->CalculateExecutionTime();
}

Value ServicesTable::PercentStateChangeAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->GetFlappingCurrent();
}

Value ServicesTable::InCheckPeriodAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	TimePeriod::Ptr timeperiod = service->GetCheckPeriod();

	/* none set means always checked */
	if (!timeperiod)
		return 1;

	return Convert::ToLong(timeperiod->IsInside(Utility::GetTime()));
}

Value ServicesTable::InNotificationPeriodAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	for (const Notification::Ptr& notification : service->GetNotifications()) {
		TimePeriod::Ptr timeperiod = notification->GetPeriod();

		if (!timeperiod || timeperiod->IsInside(Utility::GetTime()))
			return 1;
	}

	return 0;
}

Value ServicesTable::ContactsAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	ArrayData result;

	for (const User::Ptr& user : CompatUtility::GetCheckableNotificationUsers(service)) {
		result.push_back(user->GetName());
	}

	return new Array(std::move(result));
}

Value ServicesTable::DowntimesAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	ArrayData result;

	for (const Downtime::Ptr& downtime : service->GetDowntimes()) {
		if (downtime->IsExpired())
			continue;

		result.push_back(downtime->GetLegacyId());
	}

	return new Array(std::move(result));
}

Value ServicesTable::DowntimesWithInfoAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	ArrayData result;

	for (const Downtime::Ptr& downtime : service->GetDowntimes()) {
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

Value ServicesTable::CommentsAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	ArrayData result;

	for (const Comment::Ptr& comment : service->GetComments()) {
		if (comment->IsExpired())
			continue;

		result.push_back(comment->GetLegacyId());
	}

	return new Array(std::move(result));
}

Value ServicesTable::CommentsWithInfoAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	ArrayData result;

	for (const Comment::Ptr& comment : service->GetComments()) {
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

Value ServicesTable::CommentsWithExtraInfoAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	ArrayData result;

	for (const Comment::Ptr& comment : service->GetComments()) {
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

Value ServicesTable::CustomVariableNamesAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	Dictionary::Ptr vars = service->GetVars();

	ArrayData result;

	if (vars) {
		ObjectLock olock(vars);
		for (const Dictionary::Pair& kv : vars) {
			result.push_back(kv.first);
		}
	}

	return new Array(std::move(result));
}

Value ServicesTable::CustomVariableValuesAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	Dictionary::Ptr vars = service->GetVars();

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

Value ServicesTable::CustomVariablesAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	Dictionary::Ptr vars = service->GetVars();

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

Value ServicesTable::CVIsJsonAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	Dictionary::Ptr vars = service->GetVars();

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

Value ServicesTable::GroupsAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	Array::Ptr groups = service->GetGroups();

	if (!groups)
		return Empty;

	return groups;
}

Value ServicesTable::ContactGroupsAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	ArrayData result;

	for (const UserGroup::Ptr& usergroup : CompatUtility::GetCheckableNotificationUserGroups(service)) {
		result.push_back(usergroup->GetName());
	}

	return new Array(std::move(result));
}

Value ServicesTable::CheckSourceAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	CheckResult::Ptr cr = service->GetLastCheckResult();

	if (cr)
		return cr->GetCheckSource();

	return Empty;
}

Value ServicesTable::IsReachableAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return service->IsReachable();
}

Value ServicesTable::OriginalAttributesAccessor(const Value& row)
{
	Service::Ptr service = static_cast<Service::Ptr>(row);

	if (!service)
		return Empty;

	return JsonEncode(service->GetOriginalAttributes());
}
