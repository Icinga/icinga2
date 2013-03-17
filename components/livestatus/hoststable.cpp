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

#include "livestatus/hoststable.h"
#include "icinga/host.h"
#include "base/dynamictype.h"
#include <boost/foreach.hpp>

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
	table->AddColumn(prefix + "alias", Column(&HostsTable::NameAccessor, objectAccessor));
	table->AddColumn(prefix + "address", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "check_command", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "check_command_expanded", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "event_handler", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "notification_period", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "check_period", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "notes", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "notes_expanded", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "notes_url", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "notes_url_expanded", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "action_url", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "action_url_expanded", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "plugin_output", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "perf_data", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "icon_image", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "icon_image_expanded", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "icon_image_alt", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "statusmap_image", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "long_plugin_output", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "initial_state", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "max_check_attempts", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "flap_detection_enabled", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "check_freshness", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "process_performance_data", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "accept_passive_checks", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "event_handler_enabled", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "acknowledgement_type", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "check_type", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "last_state", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "last_hard_state", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "current_attempt", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "last_notification", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "next_notification", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "next_check", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "last_hard_state_change", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "has_been_checked", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "current_notification_number", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "pending_flex_downtime", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "total_services", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "checks_enabled", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "notifications_enabled", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "acknowledged", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "state", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "state_type", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "no_more_notifications", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "check_flapping_recovery_notification", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "last_check", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "last_state_change", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "last_time_up", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "last_time_down", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "last_time_unreachable", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "is_flapping", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "scheduled_downtime_depth", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "is_executing", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "active_checks_enabled", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "check_options", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "obsess_over_host", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes_list", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "check_interval", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "retry_interval", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "notification_interval", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "first_notification_delay", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "low_flap_threshold", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "high_flap_threshold", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "x_3d", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "y_3d", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "z_3d", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "latency", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "execution_time", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "percent_state_change", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "in_notification_period", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "in_check_period", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "contacts", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "downtimes", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "downtimes_with_info", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "comments", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "comments_with_info", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "comments_with_extra_info", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variable_names", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variable_values", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variables", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "filename", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "parents", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "childs", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "worst_service_state", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_ok", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_warn", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_crit", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_unknown", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_pending", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "worst_service_hard_state", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_hard_ok", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_hard_warn", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_hard_crit", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "num_services_hard_unknown", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "hard_state", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "pnpgraph_present", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "groups", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "contact_groups", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "services", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "services_with_state", Column(&Table::EmptyDictionaryAccessor, objectAccessor));
	table->AddColumn(prefix + "services_with_info", Column(&Table::EmptyDictionaryAccessor, objectAccessor));
	table->AddColumn(prefix + "groups", Column(&Table::EmptyArrayAccessor, objectAccessor));
}

String HostsTable::GetName(void) const
{
	return "hosts";
}

void HostsTable::FetchRows(const AddRowFunction& addRowFn)
{
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Host")) {
		addRowFn(object);
	}
}

Value HostsTable::NameAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Host>(object)->GetName();
}

Value HostsTable::DisplayNameAccessor(const Object::Ptr& object)
{
	return static_pointer_cast<Host>(object)->GetDisplayName();
}
