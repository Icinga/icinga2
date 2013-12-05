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
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace icinga;

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


	return CompatUtility::GetHostAddress(host);
}

Value HostsTable::Address6Accessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetHostAddress6(host);
}

Value HostsTable::CheckCommandAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	/* use hostcheck service */
	Service::Ptr hc = host->GetCheckService();

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

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	CheckCommand::Ptr checkcommand = hc->GetCheckCommand();
	if (checkcommand)
		return checkcommand->GetName(); /* this is the name without '!' args */

	return Empty;
}

Value HostsTable::EventHandlerAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

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
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceNotificationNotificationPeriod(hc);
}

Value HostsTable::CheckPeriodAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceCheckPeriod(hc);
}

Value HostsTable::NotesAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCustomAttributeConfig(host, "notes");
}

Value HostsTable::NotesExpandedAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr service = host->GetCheckService();

	if (!service)
		return Empty;

	std::vector<MacroResolver::Ptr> resolvers;

	if (service)
		resolvers.push_back(service);

	resolvers.push_back(host);
	resolvers.push_back(IcingaApplication::GetInstance());

	Value value = CompatUtility::GetCustomAttributeConfig(host, "notes");

	return MacroProcessor::ResolveMacros(value, resolvers, CheckResult::Ptr(), Utility::EscapeShellCmd);
}

Value HostsTable::NotesUrlAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Dictionary::Ptr custom = host->GetCustom();

	if (!custom)
		return Empty;

	return CompatUtility::GetCustomAttributeConfig(host, "notes_url");
}

Value HostsTable::NotesUrlExpandedAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr service = host->GetCheckService();

	if (!service)
		return Empty;

	std::vector<MacroResolver::Ptr> resolvers;

	if (service)
		resolvers.push_back(service);

	resolvers.push_back(host);
	resolvers.push_back(IcingaApplication::GetInstance());

	Value value = CompatUtility::GetCustomAttributeConfig(host, "notes_url");

	return MacroProcessor::ResolveMacros(value, resolvers, CheckResult::Ptr(), Utility::EscapeShellCmd);
}

Value HostsTable::ActionUrlAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCustomAttributeConfig(host, "action_url");
}

Value HostsTable::ActionUrlExpandedAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr service = host->GetCheckService();

	if (!service)
		return Empty;

	std::vector<MacroResolver::Ptr> resolvers;

	if (service)
		resolvers.push_back(service);

	resolvers.push_back(host);
	resolvers.push_back(IcingaApplication::GetInstance());

	Value value = CompatUtility::GetCustomAttributeConfig(host, "action_url");

	return MacroProcessor::ResolveMacros(value, resolvers, CheckResult::Ptr(), Utility::EscapeShellCmd);
}

Value HostsTable::PluginOutputAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();
	String output;

	if(hc) {
		CheckResult::Ptr cr = hc->GetLastCheckResult();

		if (cr)
			output = CompatUtility::GetCheckResultOutput(cr);
	}

	return output;
}

Value HostsTable::PerfDataAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();
	String perfdata;

	if (hc) {
		CheckResult::Ptr cr = hc->GetLastCheckResult();

		if (cr)
			perfdata = CompatUtility::GetCheckResultPerfdata(cr);
	}

	return perfdata;
}

Value HostsTable::IconImageAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCustomAttributeConfig(host, "icon_image");
}

Value HostsTable::IconImageExpandedAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr service = host->GetCheckService();

	if (!service)
		return Empty;

	std::vector<MacroResolver::Ptr> resolvers;

	if (service)
		resolvers.push_back(service);

	resolvers.push_back(host);
	resolvers.push_back(IcingaApplication::GetInstance());

	Value value = CompatUtility::GetCustomAttributeConfig(host, "icon_image");

	return MacroProcessor::ResolveMacros(value, resolvers, CheckResult::Ptr(), Utility::EscapeShellCmd);
}

Value HostsTable::IconImageAltAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCustomAttributeConfig(host, "icon_image_alt");
}

Value HostsTable::StatusmapImageAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return CompatUtility::GetCustomAttributeConfig(host, "statusmap_image");
}

Value HostsTable::LongPluginOutputAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();
	String long_output;

	if (hc) {
		CheckResult::Ptr cr = hc->GetLastCheckResult();

		if (cr)
			long_output = CompatUtility::GetCheckResultLongOutput(cr);
	}

	return long_output;
}

Value HostsTable::MaxCheckAttemptsAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return hc->GetMaxCheckAttempts();
}

Value HostsTable::FlapDetectionEnabledAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceFlapDetectionEnabled(hc);
}

Value HostsTable::AcceptPassiveChecksAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServicePassiveChecksEnabled(hc);
}

Value HostsTable::EventHandlerEnabledAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceEventHandlerEnabled(hc);
}

Value HostsTable::AcknowledgementTypeAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceAcknowledgementType(hc);
}

Value HostsTable::CheckTypeAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceCheckType(hc);
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
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return hc->GetCheckAttempt();
}

Value HostsTable::LastNotificationAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceNotificationLastNotification(hc);
}

Value HostsTable::NextNotificationAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceNotificationNextNotification(hc);
}

Value HostsTable::NextCheckAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return static_cast<int>(hc->GetNextCheck());
}

Value HostsTable::LastHardStateChangeAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return static_cast<int>(hc->GetLastHardStateChange());
}

Value HostsTable::HasBeenCheckedAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceHasBeenChecked(hc);
}

Value HostsTable::CurrentNotificationNumberAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

        return CompatUtility::GetServiceNotificationNotificationNumber(hc);
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
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceActiveChecksEnabled(hc);
}

Value HostsTable::NotificationsEnabledAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceNotificationsEnabled(hc);
}

Value HostsTable::AcknowledgedAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceIsAcknowledged(hc);
}

Value HostsTable::StateAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetState();
}

Value HostsTable::StateTypeAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return host->GetStateType();
}

Value HostsTable::NoMoreNotificationsAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceNoMoreNotifications(hc);
}

Value HostsTable::LastCheckAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return static_cast<int>(hc->GetLastCheck());
}

Value HostsTable::LastStateChangeAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return static_cast<int>(host->GetLastStateChange());
}

Value HostsTable::LastTimeUpAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return static_cast<int>(host->GetLastStateUp());
}

Value HostsTable::LastTimeDownAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return static_cast<int>(host->GetLastStateDown());
}

Value HostsTable::LastTimeUnreachableAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	return static_cast<int>(host->GetLastStateUnreachable());
}

Value HostsTable::IsFlappingAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return hc->IsFlapping();
}

Value HostsTable::ScheduledDowntimeDepthAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return hc->GetDowntimeDepth();
}

Value HostsTable::ActiveChecksEnabledAccessor(const Value& row)
{
	/* duplicate of ChecksEnableAccessor */
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceActiveChecksEnabled(hc);
}

Value HostsTable::CheckOptionsAccessor(const Value& row)
{
	/* TODO - forcexec, freshness, orphan, none */
	return Empty;
}

Value HostsTable::ModifiedAttributesAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

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
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceCheckInterval(hc);
}

Value HostsTable::RetryIntervalAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceRetryInterval(hc);
}

Value HostsTable::NotificationIntervalAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceNotificationNotificationInterval(hc);
}

Value HostsTable::LowFlapThresholdAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceLowFlapThreshold(hc);
}

Value HostsTable::HighFlapThresholdAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceHighFlapThreshold(hc);
}

Value HostsTable::X2dAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Dictionary::Ptr custom = host->GetCustom();

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
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Dictionary::Ptr custom = host->GetCustom();

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
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return (Service::CalculateLatency(hc->GetLastCheckResult()));
}

Value HostsTable::ExecutionTimeAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return (Service::CalculateExecutionTime(hc->GetLastCheckResult()));
}

Value HostsTable::PercentStateChangeAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServicePercentStateChange(hc);
}

Value HostsTable::InNotificationPeriodAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceInNotificationPeriod(hc);
}

Value HostsTable::InCheckPeriodAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceInCheckPeriod(hc);
}

Value HostsTable::ContactsAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	Array::Ptr contact_names = make_shared<Array>();

	BOOST_FOREACH(const User::Ptr& user, CompatUtility::GetServiceNotificationUsers(hc)) {
		contact_names->Add(user->GetName());
	}

	return contact_names;
}

Value HostsTable::DowntimesAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	Dictionary::Ptr downtimes = hc->GetDowntimes();

	Array::Ptr ids = make_shared<Array>();

	ObjectLock olock(downtimes);

	String id;
	Downtime::Ptr downtime;
	BOOST_FOREACH(boost::tie(id, downtime), downtimes) {

		if (!downtime)
			continue;

		if (downtime->IsExpired())
			continue;

		ids->Add(downtime->GetLegacyId());
	}

	return ids;
}

Value HostsTable::DowntimesWithInfoAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	Dictionary::Ptr downtimes = hc->GetDowntimes();

	Array::Ptr ids = make_shared<Array>();

	ObjectLock olock(downtimes);

	String id;
	Downtime::Ptr downtime;
	BOOST_FOREACH(boost::tie(id, downtime), downtimes) {

		if (!downtime)
			continue;

		if (downtime->IsExpired())
			continue;

		Array::Ptr downtime_info = make_shared<Array>();
		downtime_info->Add(downtime->GetLegacyId());
		downtime_info->Add(downtime->GetAuthor());
		downtime_info->Add(downtime->GetComment());
		ids->Add(downtime_info);
	}

	return ids;
}

Value HostsTable::CommentsAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	Dictionary::Ptr comments = hc->GetComments();

	Array::Ptr ids = make_shared<Array>();

	ObjectLock olock(comments);

	String id;
	Comment::Ptr comment;
	BOOST_FOREACH(boost::tie(id, comment), comments) {

		if (!comment)
			continue;

		if (comment->IsExpired())
			continue;

		ids->Add(comment->GetLegacyId());
	}

	return ids;
}

Value HostsTable::CommentsWithInfoAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	Dictionary::Ptr comments = hc->GetComments();

	Array::Ptr ids = make_shared<Array>();

	ObjectLock olock(comments);

	String id;
	Comment::Ptr comment;
	BOOST_FOREACH(boost::tie(id, comment), comments) {

		if (!comment)
			continue;

		if (comment->IsExpired())
			continue;

		Array::Ptr comment_info = make_shared<Array>();
		comment_info->Add(comment->GetLegacyId());
		comment_info->Add(comment->GetAuthor());
		comment_info->Add(comment->GetText());
		ids->Add(comment_info);
	}

	return ids;
}

Value HostsTable::CommentsWithExtraInfoAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	Dictionary::Ptr comments = hc->GetComments();

	Array::Ptr ids = make_shared<Array>();

	ObjectLock olock(comments);

	String id;
	Comment::Ptr comment;
	BOOST_FOREACH(boost::tie(id, comment), comments) {

		if (!comment)
			continue;

		if (comment->IsExpired())
			continue;

		Array::Ptr comment_info = make_shared<Array>();
		comment_info->Add(comment->GetLegacyId());
		comment_info->Add(comment->GetAuthor());
		comment_info->Add(comment->GetText());
		comment_info->Add(comment->GetEntryType());
		comment_info->Add(static_cast<int>(comment->GetEntryTime()));
		ids->Add(comment_info);
	}

	return ids;
}

Value HostsTable::CustomVariableNamesAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Dictionary::Ptr customvars;

	{
		ObjectLock olock(host);
		customvars = CompatUtility::GetCustomVariableConfig(host);
	}

	if (!customvars)
		return Empty;

	Array::Ptr cv = make_shared<Array>();

	String key;
	Value value;
	BOOST_FOREACH(boost::tie(key, value), customvars) {
		cv->Add(key);
	}

	return cv;
}

Value HostsTable::CustomVariableValuesAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Dictionary::Ptr customvars;

	{
		ObjectLock olock(host);
		customvars = CompatUtility::GetCustomVariableConfig(host);
	}

	if (!customvars)
		return Empty;

	Array::Ptr cv = make_shared<Array>();

	String key;
	Value value;
	BOOST_FOREACH(boost::tie(key, value), customvars) {
		cv->Add(value);
	}

	return cv;
}

Value HostsTable::CustomVariablesAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Dictionary::Ptr customvars;

	{
		ObjectLock olock(host);
		customvars = CompatUtility::GetCustomVariableConfig(host);
	}

	if (!customvars)
		return Empty;

	Array::Ptr cv = make_shared<Array>();

	String key;
	Value value;
	BOOST_FOREACH(boost::tie(key, value), customvars) {
		Array::Ptr key_val = make_shared<Array>();
		key_val->Add(key);
		key_val->Add(value);
		cv->Add(key_val);
	}

	return cv;
}

Value HostsTable::ParentsAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Array::Ptr parents = make_shared<Array>();

	BOOST_FOREACH(const Host::Ptr& parent, host->GetParentHosts()) {
		parents->Add(parent->GetName());
	}

	return parents;
}

Value HostsTable::ChildsAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Array::Ptr childs = make_shared<Array>();

	BOOST_FOREACH(const Host::Ptr& child, host->GetChildHosts()) {
		childs->Add(child->GetName());
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

	Value worst_service = StateOK;

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
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

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
		if (service->GetState() == StateOK)
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

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
		if (service->GetState() == StateWarning)
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

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
		if (service->GetState() == StateCritical)
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

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
		if (service->GetState() == StateUnknown)
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

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
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

	Value worst_service = StateOK;

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
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

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == StateOK)
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

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == StateWarning)
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

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == StateCritical)
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

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
		if (service->GetStateType() == StateTypeHard && service->GetState() == StateUnknown)
			num_services++;
	}

	return num_services;
}

Value HostsTable::HardStateAccessor(const Value& row)
{
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

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
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	return CompatUtility::GetServiceStaleness(hc);
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
	/* use hostcheck service */
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Service::Ptr hc = host->GetCheckService();

	if (!hc)
		return Empty;

	Array::Ptr contactgroup_names = make_shared<Array>();

	BOOST_FOREACH(const UserGroup::Ptr& usergroup, CompatUtility::GetServiceNotificationUserGroups(hc)) {
		contactgroup_names->Add(usergroup->GetName());
	}

	return contactgroup_names;
}

Value HostsTable::ServicesAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Array::Ptr services = make_shared<Array>();

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
		services->Add(service->GetShortName());
	}

	return services;
}

Value HostsTable::ServicesWithStateAccessor(const Value& row)
{
	Host::Ptr host = static_cast<Host::Ptr>(row);

	if (!host)
		return Empty;

	Array::Ptr services = make_shared<Array>();

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
		Array::Ptr svc_add = make_shared<Array>();

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

	Array::Ptr services = make_shared<Array>();

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
		Array::Ptr svc_add = make_shared<Array>();

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

