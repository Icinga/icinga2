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

#include "livestatus/statustable.h"
#include "icinga/icingaapplication.h"
#include "icinga/cib.h"
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

Value StatusTable::NebCallbacksAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value StatusTable::NebCallbacksRateAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value StatusTable::RequestsAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::RequestsRateAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::ConnectionsAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::ConnectionsRateAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::ServiceChecksAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::ServiceChecksRateAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::HostChecksAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::HostChecksRateAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::ForksAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::ForksRateAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::LogMessagesAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::LogMessagesRateAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::ExternalCommandsAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::ExternalCommandsRateAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::LivechecksAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::LivechecksRateAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::LivecheckOverflowsAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::LivecheckOverflowsRateAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::NagiosPidAccessor(const Object::Ptr& object)
{
	return Utility::GetPid();
}

Value StatusTable::EnableNotificationsAccessor(const Object::Ptr& object)
{
	/* TODO - enabled by default*/
	return 1;
}

Value StatusTable::ExecuteServiceChecksAccessor(const Object::Ptr& object)
{
	/* TODO - enabled by default*/
	return 1;
}

Value StatusTable::AcceptPassiveServiceChecksAccessor(const Object::Ptr& object)
{
	/* TODO - enabled by default*/
	return 1;
}

Value StatusTable::ExecuteHostChecksAccessor(const Object::Ptr& object)
{
	/* TODO - enabled by default*/
	return 1;
}

Value StatusTable::AcceptPassiveHostChecksAccessor(const Object::Ptr& object)
{
	/* TODO - enabled by default*/
	return 1;
}

Value StatusTable::EnableEventHandlersAccessor(const Object::Ptr& object)
{
	/* TODO - enabled by default*/
	return 1;
}

Value StatusTable::ObsessOverServicesAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value StatusTable::ObsessOverHostsAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value StatusTable::CheckServiceFreshnessAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::CheckHostFreshnessAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::EnableFlapDetectionAccessor(const Object::Ptr& object)
{
	/* TODO - enabled by default*/
	return 1;
}

Value StatusTable::ProcessPerformanceDataAccessor(const Object::Ptr& object)
{
	/* TODO - enabled by default*/
	return 1;
}

Value StatusTable::CheckExternalCommandsAccessor(const Object::Ptr& object)
{
	/* TODO - enabled by default*/
	return 1;
}

Value StatusTable::ProgramStartAccessor(const Object::Ptr& object)
{
	return IcingaApplication::GetInstance()->GetStartTime();
}

Value StatusTable::LastCommandCheckAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::LastLogRotationAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::IntervalLengthAccessor(const Object::Ptr& object)
{
	/* hardcoded 60s */
	return 60;
}

Value StatusTable::NumHostsAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::NumServicesAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::ProgramVersionAccessor(const Object::Ptr& object)
{
	return "2.0";
}

Value StatusTable::ExternalCommandBufferSlotsAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::ExternalCommandBufferUsageAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::ExternalCommandBufferMaxAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::CachedLogMessagesAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::LivestatusVersionAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::LivestatusActiveConnectionsAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::LivestatusQueuedConnectionsAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value StatusTable::LivestatusThreadsAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}
