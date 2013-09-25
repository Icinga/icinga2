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

#include "livestatus/statustable.h"
#include "livestatus/listener.h"
#include "icinga/icingaapplication.h"
#include "icinga/cib.h"
#include "icinga/host.h"
#include "icinga/service.h"
#include "base/dynamictype.h"
#include "base/utility.h"
#include <boost/smart_ptr/make_shared.hpp>

using namespace icinga;
using namespace livestatus;

StatusTable::StatusTable(void)
{
	AddColumns(this);
}

void StatusTable::AddColumns(Table *table, const String& prefix,
    const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "neb_callbacks", Column(&StatusTable::NebCallbacksAccessor, objectAccessor));
	table->AddColumn(prefix + "neb_callbacks_rate", Column(&StatusTable::NebCallbacksRateAccessor, objectAccessor));

	table->AddColumn(prefix + "requests", Column(&StatusTable::RequestsAccessor, objectAccessor));
	table->AddColumn(prefix + "requests_rate", Column(&StatusTable::RequestsRateAccessor, objectAccessor));

	table->AddColumn(prefix + "connections", Column(&StatusTable::ConnectionsAccessor, objectAccessor));
	table->AddColumn(prefix + "connections_rate", Column(&StatusTable::ConnectionsRateAccessor, objectAccessor));

	table->AddColumn(prefix + "service_checks", Column(&StatusTable::ServiceChecksAccessor, objectAccessor));
	table->AddColumn(prefix + "service_checks_rate", Column(&StatusTable::ServiceChecksRateAccessor, objectAccessor));

	table->AddColumn(prefix + "host_checks", Column(&StatusTable::HostChecksAccessor, objectAccessor));
	table->AddColumn(prefix + "host_checks_rate", Column(&StatusTable::HostChecksRateAccessor, objectAccessor));

	table->AddColumn(prefix + "forks", Column(&StatusTable::ForksAccessor, objectAccessor));
	table->AddColumn(prefix + "forks_rate", Column(&StatusTable::ForksRateAccessor, objectAccessor));

	table->AddColumn(prefix + "log_messages", Column(&StatusTable::LogMessagesAccessor, objectAccessor));
	table->AddColumn(prefix + "log_messages_rate", Column(&StatusTable::LogMessagesRateAccessor, objectAccessor));

	table->AddColumn(prefix + "external_commands", Column(&StatusTable::ExternalCommandsAccessor, objectAccessor));
	table->AddColumn(prefix + "external_commands_rate", Column(&StatusTable::ExternalCommandsRateAccessor, objectAccessor));

	table->AddColumn(prefix + "livechecks", Column(&StatusTable::LivechecksAccessor, objectAccessor));
	table->AddColumn(prefix + "livechecks_rate", Column(&StatusTable::LivechecksRateAccessor, objectAccessor));

	table->AddColumn(prefix + "livecheck_overflows", Column(&StatusTable::LivecheckOverflowsAccessor, objectAccessor));
	table->AddColumn(prefix + "livecheck_overflows_rate", Column(&StatusTable::LivecheckOverflowsRateAccessor, objectAccessor));

	table->AddColumn(prefix + "nagios_pid", Column(&StatusTable::NagiosPidAccessor, objectAccessor));
	table->AddColumn(prefix + "enable_notifications", Column(&StatusTable::EnableNotificationsAccessor, objectAccessor));
	table->AddColumn(prefix + "execute_service_checks", Column(&StatusTable::ExecuteServiceChecksAccessor, objectAccessor));
	table->AddColumn(prefix + "accept_passive_service_checks", Column(&StatusTable::AcceptPassiveServiceChecksAccessor, objectAccessor));
	table->AddColumn(prefix + "execute_host_checks", Column(&StatusTable::ExecuteHostChecksAccessor, objectAccessor));
	table->AddColumn(prefix + "accept_passive_host_checks", Column(&StatusTable::AcceptPassiveHostChecksAccessor, objectAccessor));
	table->AddColumn(prefix + "enable_event_handlers", Column(&StatusTable::EnableEventHandlersAccessor, objectAccessor));
	table->AddColumn(prefix + "obsess_over_services", Column(&StatusTable::ObsessOverHostsAccessor, objectAccessor));
	table->AddColumn(prefix + "obsess_over_hosts", Column(&StatusTable::ObsessOverServicesAccessor, objectAccessor));
	table->AddColumn(prefix + "check_service_freshness", Column(&StatusTable::CheckServiceFreshnessAccessor, objectAccessor));
	table->AddColumn(prefix + "check_host_freshness", Column(&StatusTable::CheckHostFreshnessAccessor, objectAccessor));
	table->AddColumn(prefix + "enable_flap_detection", Column(&StatusTable::EnableFlapDetectionAccessor, objectAccessor));
	table->AddColumn(prefix + "process_performance_data", Column(&StatusTable::ProcessPerformanceDataAccessor, objectAccessor));
	table->AddColumn(prefix + "check_external_commands", Column(&StatusTable::CheckExternalCommandsAccessor, objectAccessor));
	table->AddColumn(prefix + "program_start", Column(&StatusTable::ProgramStartAccessor, objectAccessor));
	table->AddColumn(prefix + "last_command_check", Column(&StatusTable::LastCommandCheckAccessor, objectAccessor));
	table->AddColumn(prefix + "last_log_rotation", Column(&StatusTable::LastLogRotationAccessor, objectAccessor));
	table->AddColumn(prefix + "interval_length", Column(&StatusTable::IntervalLengthAccessor, objectAccessor));
	table->AddColumn(prefix + "num_hosts", Column(&StatusTable::NumHostsAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services", Column(&StatusTable::NumServicesAccessor, objectAccessor));
	table->AddColumn(prefix + "program_version", Column(&StatusTable::ProgramVersionAccessor, objectAccessor));
	table->AddColumn(prefix + "external_command_buffer_slots", Column(&StatusTable::ExternalCommandBufferSlotsAccessor, objectAccessor));
	table->AddColumn(prefix + "external_command_buffer_usage", Column(&StatusTable::ExternalCommandBufferUsageAccessor, objectAccessor));
	table->AddColumn(prefix + "external_command_buffer_max", Column(&StatusTable::ExternalCommandBufferMaxAccessor, objectAccessor));
	table->AddColumn(prefix + "cached_log_messages", Column(&StatusTable::CachedLogMessagesAccessor, objectAccessor));
	table->AddColumn(prefix + "livestatus_version", Column(&StatusTable::LivestatusVersionAccessor, objectAccessor));
	table->AddColumn(prefix + "livestatus_active_connections", Column(&StatusTable::LivestatusActiveConnectionsAccessor, objectAccessor));
	table->AddColumn(prefix + "livestatus_queued_connections", Column(&StatusTable::LivestatusQueuedConnectionsAccessor, objectAccessor));
	table->AddColumn(prefix + "livestatus_threads", Column(&StatusTable::LivestatusThreadsAccessor, objectAccessor));
}

String StatusTable::GetName(void) const
{
	return "status";
}

void StatusTable::FetchRows(const AddRowFunction& addRowFn)
{
	Object::Ptr obj = boost::make_shared<Object>();

	/* Return a fake row. */
	addRowFn(obj);
}

Value StatusTable::NebCallbacksAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::NebCallbacksRateAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::RequestsAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::RequestsRateAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::ConnectionsAccessor(const Value& row)
{
	return LivestatusListener::GetConnections();
}

Value StatusTable::ConnectionsRateAccessor(const Value& row)
{
	return (LivestatusListener::GetConnections() / (Utility::GetTime() - IcingaApplication::GetInstance()->GetStartTime()));
}

Value StatusTable::ServiceChecksAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::ServiceChecksRateAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::HostChecksAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::HostChecksRateAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::ForksAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::ForksRateAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::LogMessagesAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::LogMessagesRateAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::ExternalCommandsAccessor(const Value& row)
{
	return Query::GetExternalCommands();
}

Value StatusTable::ExternalCommandsRateAccessor(const Value& row)
{
	return (Query::GetExternalCommands() / (Utility::GetTime() - IcingaApplication::GetInstance()->GetStartTime()));
}

Value StatusTable::LivechecksAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::LivechecksRateAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::LivecheckOverflowsAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::LivecheckOverflowsRateAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::NagiosPidAccessor(const Value& row)
{
	return Utility::GetPid();
}

Value StatusTable::EnableNotificationsAccessor(const Value& row)
{
	/* enabled by default*/
	return 1;
}

Value StatusTable::ExecuteServiceChecksAccessor(const Value& row)
{
	/* enabled by default*/
	return 1;
}

Value StatusTable::AcceptPassiveServiceChecksAccessor(const Value& row)
{
	/* enabled by default*/
	return 1;
}

Value StatusTable::ExecuteHostChecksAccessor(const Value& row)
{
	/* enabled by default*/
	return 1;
}

Value StatusTable::AcceptPassiveHostChecksAccessor(const Value& row)
{
	/* enabled by default*/
	return 1;
}

Value StatusTable::EnableEventHandlersAccessor(const Value& row)
{
	/* enabled by default*/
	return 1;
}

Value StatusTable::ObsessOverServicesAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::ObsessOverHostsAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::CheckServiceFreshnessAccessor(const Value& row)
{
	/* enable by default */
	return 1;
}

Value StatusTable::CheckHostFreshnessAccessor(const Value& row)
{
	/* TODO */
	return Empty;
}

Value StatusTable::EnableFlapDetectionAccessor(const Value& row)
{
	/* enabled by default*/
	return 1;
}

Value StatusTable::ProcessPerformanceDataAccessor(const Value& row)
{
	/* enabled by default*/
	return 1;
}

Value StatusTable::CheckExternalCommandsAccessor(const Value& row)
{
	/* enabled by default*/
	return 1;
}

Value StatusTable::ProgramStartAccessor(const Value& row)
{
	return static_cast<int>(IcingaApplication::GetInstance()->GetStartTime());
}

Value StatusTable::LastCommandCheckAccessor(const Value& row)
{
	/* always == now */
	return static_cast<int>(Utility::GetTime());
}

Value StatusTable::LastLogRotationAccessor(const Value& row)
{
	/* TODO */
	return Empty;
}

Value StatusTable::IntervalLengthAccessor(const Value& row)
{
	/* hardcoded 60s */
	return 60;
}

Value StatusTable::NumHostsAccessor(const Value& row)
{
	return DynamicType::GetObjects<Host>().size();
}

Value StatusTable::NumServicesAccessor(const Value& row)
{
	return DynamicType::GetObjects<Service>().size();
}

Value StatusTable::ProgramVersionAccessor(const Value& row)
{
	return "2.0";
}

Value StatusTable::ExternalCommandBufferSlotsAccessor(const Value& row)
{
	/* infinite */
	return Empty;
}

Value StatusTable::ExternalCommandBufferUsageAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::ExternalCommandBufferMaxAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::CachedLogMessagesAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::LivestatusVersionAccessor(const Value& row)
{
	return "2.0";
}

Value StatusTable::LivestatusActiveConnectionsAccessor(const Value& row)
{
	return LivestatusListener::GetClientsConnected();
}

Value StatusTable::LivestatusQueuedConnectionsAccessor(const Value& row)
{
	/* not supported */
	return Empty;
}

Value StatusTable::LivestatusThreadsAccessor(const Value& row)
{
	/* TODO */
	return Empty;
}
