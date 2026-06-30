// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "livestatus/statustable.hpp"
#include "livestatus/livestatuslistener.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/cib.hpp"
#include "icinga/host.hpp"
#include "icinga/service.hpp"
#include "base/configtype.hpp"
#include "base/utility.hpp"
#include "base/application.hpp"

using namespace icinga;

StatusTable::StatusTable()
{
	AddColumns(this);
}

void StatusTable::AddColumns(Table *table, const String& prefix,
	const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "neb_callbacks", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "neb_callbacks_rate", Column(&Table::ZeroAccessor, objectAccessor));

	table->AddColumn(prefix + "requests", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "requests_rate", Column(&Table::ZeroAccessor, objectAccessor));

	table->AddColumn(prefix + "connections", Column(&StatusTable::ConnectionsAccessor, objectAccessor));
	table->AddColumn(prefix + "connections_rate", Column(&StatusTable::ConnectionsRateAccessor, objectAccessor));

	table->AddColumn(prefix + "service_checks", Column(&StatusTable::ServiceChecksAccessor, objectAccessor));
	table->AddColumn(prefix + "service_checks_rate", Column(&StatusTable::ServiceChecksRateAccessor, objectAccessor));

	table->AddColumn(prefix + "host_checks", Column(&StatusTable::HostChecksAccessor, objectAccessor));
	table->AddColumn(prefix + "host_checks_rate", Column(&StatusTable::HostChecksRateAccessor, objectAccessor));

	table->AddColumn(prefix + "forks", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "forks_rate", Column(&Table::ZeroAccessor, objectAccessor));

	table->AddColumn(prefix + "log_messages", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "log_messages_rate", Column(&Table::ZeroAccessor, objectAccessor));

	table->AddColumn(prefix + "external_commands", Column(&StatusTable::ExternalCommandsAccessor, objectAccessor));
	table->AddColumn(prefix + "external_commands_rate", Column(&StatusTable::ExternalCommandsRateAccessor, objectAccessor));

	table->AddColumn(prefix + "livechecks", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "livechecks_rate", Column(&Table::ZeroAccessor, objectAccessor));

	table->AddColumn(prefix + "livecheck_overflows", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "livecheck_overflows_rate", Column(&Table::ZeroAccessor, objectAccessor));

	table->AddColumn(prefix + "nagios_pid", Column(&StatusTable::NagiosPidAccessor, objectAccessor));
	table->AddColumn(prefix + "enable_notifications", Column(&StatusTable::EnableNotificationsAccessor, objectAccessor));
	table->AddColumn(prefix + "execute_service_checks", Column(&StatusTable::ExecuteServiceChecksAccessor, objectAccessor));
	table->AddColumn(prefix + "accept_passive_service_checks", Column(&Table::OneAccessor, objectAccessor));
	table->AddColumn(prefix + "execute_host_checks", Column(&StatusTable::ExecuteHostChecksAccessor, objectAccessor));
	table->AddColumn(prefix + "accept_passive_host_checks", Column(&Table::OneAccessor, objectAccessor));
	table->AddColumn(prefix + "enable_event_handlers", Column(&StatusTable::EnableEventHandlersAccessor, objectAccessor));
	table->AddColumn(prefix + "obsess_over_services", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "obsess_over_hosts", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "check_service_freshness", Column(&Table::OneAccessor, objectAccessor));
	table->AddColumn(prefix + "check_host_freshness", Column(&Table::OneAccessor, objectAccessor));
	table->AddColumn(prefix + "enable_flap_detection", Column(&StatusTable::EnableFlapDetectionAccessor, objectAccessor));
	table->AddColumn(prefix + "process_performance_data", Column(&StatusTable::ProcessPerformanceDataAccessor, objectAccessor));
	table->AddColumn(prefix + "check_external_commands", Column(&Table::OneAccessor, objectAccessor));
	table->AddColumn(prefix + "program_start", Column(&StatusTable::ProgramStartAccessor, objectAccessor));
	table->AddColumn(prefix + "last_command_check", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "last_log_rotation", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "interval_length", Column(&StatusTable::IntervalLengthAccessor, objectAccessor));
	table->AddColumn(prefix + "num_hosts", Column(&StatusTable::NumHostsAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services", Column(&StatusTable::NumServicesAccessor, objectAccessor));
	table->AddColumn(prefix + "program_version", Column(&StatusTable::ProgramVersionAccessor, objectAccessor));
	table->AddColumn(prefix + "external_command_buffer_slots", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "external_command_buffer_usage", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "external_command_buffer_max", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "cached_log_messages", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "livestatus_version", Column(&StatusTable::LivestatusVersionAccessor, objectAccessor));
	table->AddColumn(prefix + "livestatus_active_connections", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "livestatus_queued_connections", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "livestatus_threads", Column(&Table::ZeroAccessor, objectAccessor));

	table->AddColumn(prefix + "custom_variable_names", Column(&StatusTable::CustomVariableNamesAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variable_values", Column(&StatusTable::CustomVariableValuesAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variables", Column(&StatusTable::CustomVariablesAccessor, objectAccessor));
}

String StatusTable::GetName() const
{
	return "status";
}

String StatusTable::GetPrefix() const
{
	return "status";
}

void StatusTable::FetchRows(const AddRowFunction& addRowFn)
{
	Object::Ptr obj = new Object();

	/* Return a fake row. */
	addRowFn(obj, LivestatusGroupByNone, Empty);
}

Value StatusTable::ConnectionsAccessor(const Value&)
{
	return LivestatusListener::GetConnections();
}

Value StatusTable::ConnectionsRateAccessor(const Value&)
{
	return (LivestatusListener::GetConnections() / (Utility::GetTime() - Application::GetStartTime()));
}

Value StatusTable::HostChecksAccessor(const Value&)
{
	auto timespan = static_cast<long>(Utility::GetTime() - Application::GetStartTime());
	return CIB::GetActiveHostChecksStatistics(timespan);
}

Value StatusTable::HostChecksRateAccessor(const Value&)
{
	auto timespan = static_cast<long>(Utility::GetTime() - Application::GetStartTime());
	return (CIB::GetActiveHostChecksStatistics(timespan) / (Utility::GetTime() - Application::GetStartTime()));
}

Value StatusTable::ServiceChecksAccessor(const Value&)
{
	auto timespan = static_cast<long>(Utility::GetTime() - Application::GetStartTime());
	return CIB::GetActiveServiceChecksStatistics(timespan);
}

Value StatusTable::ServiceChecksRateAccessor(const Value&)
{
	auto timespan = static_cast<long>(Utility::GetTime() - Application::GetStartTime());
	return (CIB::GetActiveServiceChecksStatistics(timespan) / (Utility::GetTime() - Application::GetStartTime()));
}

Value StatusTable::ExternalCommandsAccessor(const Value&)
{
	return LivestatusQuery::GetExternalCommands();
}

Value StatusTable::ExternalCommandsRateAccessor(const Value&)
{
	return (LivestatusQuery::GetExternalCommands() / (Utility::GetTime() - Application::GetStartTime()));
}

Value StatusTable::NagiosPidAccessor(const Value&)
{
	return Utility::GetPid();
}

Value StatusTable::EnableNotificationsAccessor(const Value&)
{
	return (IcingaApplication::GetInstance()->GetEnableNotifications() ? 1 : 0);
}

Value StatusTable::ExecuteServiceChecksAccessor(const Value&)
{
	return (IcingaApplication::GetInstance()->GetEnableServiceChecks() ? 1 : 0);
}

Value StatusTable::ExecuteHostChecksAccessor(const Value&)
{
	return (IcingaApplication::GetInstance()->GetEnableHostChecks() ? 1 : 0);
}

Value StatusTable::EnableEventHandlersAccessor(const Value&)
{
	return (IcingaApplication::GetInstance()->GetEnableEventHandlers() ? 1 : 0);
}

Value StatusTable::EnableFlapDetectionAccessor(const Value&)
{
	return (IcingaApplication::GetInstance()->GetEnableFlapping() ? 1 : 0);
}

Value StatusTable::ProcessPerformanceDataAccessor(const Value&)
{
	return (IcingaApplication::GetInstance()->GetEnablePerfdata() ? 1 : 0);
}

Value StatusTable::ProgramStartAccessor(const Value&)
{
	return static_cast<long>(Application::GetStartTime());
}

Value StatusTable::IntervalLengthAccessor(const Value&)
{
	return LIVESTATUS_INTERVAL_LENGTH;
}

Value StatusTable::NumHostsAccessor(const Value&)
{
	return ConfigType::Get<Host>()->GetObjectCount();
}

Value StatusTable::NumServicesAccessor(const Value&)
{
	return ConfigType::Get<Service>()->GetObjectCount();
}

Value StatusTable::ProgramVersionAccessor(const Value&)
{
	return Application::GetAppVersion() + "-icinga2";
}

Value StatusTable::LivestatusVersionAccessor(const Value&)
{
	return Application::GetAppVersion();
}

Value StatusTable::LivestatusActiveConnectionsAccessor(const Value&)
{
	return LivestatusListener::GetClientsConnected();
}

Value StatusTable::CustomVariableNamesAccessor(const Value&)
{
	Dictionary::Ptr vars = IcingaApplication::GetInstance()->GetVars();

	ArrayData result;

	if (vars) {
		ObjectLock olock(vars);
		for (const auto& kv : vars) {
			result.push_back(kv.first);
		}
	}

	return new Array(std::move(result));
}

Value StatusTable::CustomVariableValuesAccessor(const Value&)
{
	Dictionary::Ptr vars = IcingaApplication::GetInstance()->GetVars();

	ArrayData result;

	if (vars) {
		ObjectLock olock(vars);
		for (const auto& kv : vars) {
			result.push_back(kv.second);
		}
	}

	return new Array(std::move(result));
}

Value StatusTable::CustomVariablesAccessor(const Value&)
{
	Dictionary::Ptr vars = IcingaApplication::GetInstance()->GetVars();

	ArrayData result;

	if (vars) {
		ObjectLock olock(vars);
		for (const auto& kv : vars) {
			result.push_back(new Array({
				kv.first,
				kv.second
			}));
		}
	}

	return new Array(std::move(result));
}
