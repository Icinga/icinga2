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

using namespace icinga;
using namespace livestatus;

StatusTable::StatusTable(void)
{
	AddColumn("neb_callbacks", &Table::ZeroAccessor);
	AddColumn("neb_callbacks_rate", &Table::ZeroAccessor);

	AddColumn("requests", &Table::ZeroAccessor);
	AddColumn("requests_rate", &Table::ZeroAccessor);

	AddColumn("connections", &Table::ZeroAccessor);
	AddColumn("connections_rate", &Table::ZeroAccessor);

	AddColumn("service_checks", &Table::ZeroAccessor);
	AddColumn("service_checks_rate", &Table::ZeroAccessor);

	AddColumn("host_checks", &Table::ZeroAccessor);
	AddColumn("host_checks_rate", &Table::ZeroAccessor);

	AddColumn("forks", &Table::ZeroAccessor);
	AddColumn("forks_rate", &Table::ZeroAccessor);

	AddColumn("log_messages", &Table::ZeroAccessor);
	AddColumn("log_messages_rate", &Table::ZeroAccessor);

	AddColumn("external_commands", &Table::ZeroAccessor);
	AddColumn("external_commands_rate", &Table::ZeroAccessor);

	AddColumn("livechecks", &Table::ZeroAccessor);
	AddColumn("livechecks_rate", &Table::ZeroAccessor);

	AddColumn("livecheck_overflows", &Table::ZeroAccessor);
	AddColumn("livecheck_overflows_rate", &Table::ZeroAccessor);

	AddColumn("nagios_pid", &Table::ZeroAccessor);
	AddColumn("enable_notifications", &Table::ZeroAccessor);
	AddColumn("execute_service_checks", &Table::ZeroAccessor);
	AddColumn("accept_passive_service_checks", &Table::ZeroAccessor);
	AddColumn("execute_host_checks", &Table::ZeroAccessor);
	AddColumn("accept_passive_host_checks", &Table::ZeroAccessor);
	AddColumn("enable_event_handlers", &Table::ZeroAccessor);
	AddColumn("obsess_over_services", &Table::ZeroAccessor);
	AddColumn("obsess_over_hosts", &Table::ZeroAccessor);
	AddColumn("check_service_freshness", &Table::ZeroAccessor);
	AddColumn("check_host_freshness", &Table::ZeroAccessor);
	AddColumn("enable_flap_detection", &Table::ZeroAccessor);
	AddColumn("process_performance_data", &Table::ZeroAccessor);
	AddColumn("check_external_commands", &Table::ZeroAccessor);
	AddColumn("program_start", &Table::ZeroAccessor);
	AddColumn("last_command_check", &Table::ZeroAccessor);
	AddColumn("last_log_rotation", &Table::ZeroAccessor);
	AddColumn("interval_length", &Table::ZeroAccessor);
	AddColumn("num_hosts", &Table::ZeroAccessor);
	AddColumn("num_services", &Table::ZeroAccessor);
	AddColumn("program_version", &Table::ZeroAccessor);
	AddColumn("external_command_buffer_slots", &Table::ZeroAccessor);
	AddColumn("external_command_buffer_usage", &Table::ZeroAccessor);
	AddColumn("external_command_buffer_max", &Table::ZeroAccessor);
	AddColumn("cached_log_messages", &Table::ZeroAccessor);
	AddColumn("livestatus_version", &Table::ZeroAccessor);
	AddColumn("livestatus_active_connections", &Table::ZeroAccessor);
	AddColumn("livestatus_queued_connections", &Table::ZeroAccessor);
	AddColumn("livestatus_threads", &Table::ZeroAccessor);
}

String StatusTable::GetName(void) const
{
	return "status";
}

void StatusTable::FetchRows(const function<void (const Object::Ptr&)>& addRowFn)
{
	Object::Ptr obj = boost::make_shared<Object>();

	/* Return a fake row. */
	addRowFn(obj);
}
