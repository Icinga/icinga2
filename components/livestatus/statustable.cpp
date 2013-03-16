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

#include "i2-livestatus.h"
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
	table->AddColumn(prefix + "neb_callbacks", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "neb_callbacks_rate", Column(&Table::ZeroAccessor, objectAccessor));

	table->AddColumn(prefix + "requests", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "requests_rate", Column(&Table::ZeroAccessor, objectAccessor));

	table->AddColumn(prefix + "connections", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "connections_rate", Column(&Table::ZeroAccessor, objectAccessor));

	table->AddColumn(prefix + "service_checks", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "service_checks_rate", Column(&Table::ZeroAccessor, objectAccessor));

	table->AddColumn(prefix + "host_checks", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "host_checks_rate", Column(&Table::ZeroAccessor, objectAccessor));

	table->AddColumn(prefix + "forks", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "forks_rate", Column(&Table::ZeroAccessor, objectAccessor));

	table->AddColumn(prefix + "log_messages", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "log_messages_rate", Column(&Table::ZeroAccessor, objectAccessor));

	table->AddColumn(prefix + "external_commands", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "external_commands_rate", Column(&Table::ZeroAccessor, objectAccessor));

	table->AddColumn(prefix + "livechecks", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "livechecks_rate", Column(&Table::ZeroAccessor, objectAccessor));

	table->AddColumn(prefix + "livecheck_overflows", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "livecheck_overflows_rate", Column(&Table::ZeroAccessor, objectAccessor));

	table->AddColumn(prefix + "nagios_pid", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "enable_notifications", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "execute_service_checks", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "accept_passive_service_checks", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "execute_host_checks", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "accept_passive_host_checks", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "enable_event_handlers", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "obsess_over_services", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "obsess_over_hosts", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "check_service_freshness", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "check_host_freshness", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "enable_flap_detection", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "process_performance_data", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "check_external_commands", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "program_start", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "last_command_check", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "last_log_rotation", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "interval_length", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "num_hosts", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "program_version", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "external_command_buffer_slots", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "external_command_buffer_usage", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "external_command_buffer_max", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "cached_log_messages", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "livestatus_version", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "livestatus_active_connections", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "livestatus_queued_connections", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "livestatus_threads", Column(&Table::ZeroAccessor, objectAccessor));
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
