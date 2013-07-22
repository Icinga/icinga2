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

#include "ido/servicedbobject.h"
#include "ido/dbtype.h"
#include "icinga/service.h"
#include "icinga/checkcommand.h"

using namespace icinga;

REGISTER_DBTYPE("Service", "service", 2, ServiceDbObject);

ServiceDbObject::ServiceDbObject(const String& name1, const String& name2)
	: DbObject(DbType::GetByName("Service"), name1, name2)
{ }

Dictionary::Ptr ServiceDbObject::GetConfigFields(void) const
{
	Dictionary::Ptr fields = boost::make_shared<Dictionary>();
	Service::Ptr service = static_pointer_cast<Service>(GetObject());

	fields->Set("display_name", service->GetDisplayName());
	fields->Set("check_command_object_id", service->GetCheckCommand());
	fields->Set("check_command_args", Empty);
	fields->Set("eventhandler_command_object_id", Empty);
	fields->Set("eventhandler_command_args", Empty);
	fields->Set("notification_timeperiod_object_id", Empty);
	fields->Set("check_timeperiod_object_id", Empty);
	fields->Set("failure_prediction_options", Empty);
	fields->Set("check_interval", service->GetCheckInterval() * 60);
	fields->Set("retry_interval", service->GetRetryInterval() * 60);
	fields->Set("max_check_attempts", service->GetMaxCheckAttempts());
	fields->Set("first_notification_delay", Empty);
	fields->Set("notification_interval", Empty);
	fields->Set("notify_on_warning", Empty);
	fields->Set("notify_on_unknown", Empty);
	fields->Set("notify_on_critical", Empty);
	fields->Set("notify_on_recovery", Empty);
	fields->Set("notify_on_flapping", Empty);
	fields->Set("notify_on_downtime", Empty);
	fields->Set("stalk_on_ok", 0);
	fields->Set("stalk_on_warning", 0);
	fields->Set("stalk_on_unknown", 0);
	fields->Set("stalk_on_critical", 0);
	fields->Set("is_volatile", Empty);
	fields->Set("flap_detection_enabled", Empty);
	fields->Set("flap_detection_on_ok", Empty);
	fields->Set("flap_detection_on_warning", Empty);
	fields->Set("flap_detection_on_unknown", Empty);
	fields->Set("flap_detection_on_critical", Empty);
	fields->Set("low_flap_threshold", Empty);
	fields->Set("high_flap_threshold", Empty);
	fields->Set("process_performance_data", Empty);
	fields->Set("freshness_checks_enabled", Empty);
	fields->Set("freshness_threshold", Empty);
	fields->Set("passive_checks_enabled", Empty);
	fields->Set("event_handler_enabled", Empty);
	fields->Set("active_checks_enabled", Empty);
	fields->Set("retain_status_information", Empty);
	fields->Set("retain_nonstatus_information", Empty);
	fields->Set("notifications_enabled", Empty);
	fields->Set("obsess_over_service", Empty);
	fields->Set("failure_prediction_enabled", Empty);
	fields->Set("notes", Empty);
	fields->Set("notes_url", Empty);
	fields->Set("action_url", Empty);
	fields->Set("icon_image", Empty);
	fields->Set("icon_image_alt", Empty);

	return fields;
}

Dictionary::Ptr ServiceDbObject::GetStatusFields(void) const
{
	Dictionary::Ptr fields = boost::make_shared<Dictionary>();
	Service::Ptr service = static_pointer_cast<Service>(GetObject());

	fields->Set("output", Empty);
	fields->Set("long_output", Empty);
	fields->Set("perfdata", Empty);
	fields->Set("current_state", Empty);
	fields->Set("has_been_checked", Empty);
	fields->Set("should_be_scheduled", Empty);
	fields->Set("current_check_attempt", Empty);
	fields->Set("max_check_attempts", Empty);
	fields->Set("last_check", Empty);
	fields->Set("next_check", Empty);
	fields->Set("check_type", Empty);
	fields->Set("last_state_change", Empty);
	fields->Set("last_hard_state_change", Empty);
	fields->Set("last_hard_state", Empty);
	fields->Set("last_time_ok", Empty);
	fields->Set("last_time_warning", Empty);
	fields->Set("last_time_unknown", Empty);
	fields->Set("last_time_critical", Empty);
	fields->Set("state_type", Empty);
	fields->Set("last_notification", Empty);
	fields->Set("next_notification", Empty);
	fields->Set("no_more_notifications", Empty);
	fields->Set("notifications_enabled", Empty);
	fields->Set("problem_has_been_acknowledged", Empty);
	fields->Set("acknowledgement_type", Empty);
	fields->Set("current_notification_number", Empty);
	fields->Set("passive_checks_enabled", Empty);
	fields->Set("active_checks_enabled", Empty);
	fields->Set("event_handler_enabled", Empty);
	fields->Set("flap_detection_enabled", Empty);
	fields->Set("is_flapping", Empty);
	fields->Set("percent_state_change", Empty);
	fields->Set("latency", Empty);
	fields->Set("execution_time", Empty);
	fields->Set("scheduled_downtime_depth", Empty);
	fields->Set("failure_prediction_enabled", Empty);
	fields->Set("process_performance_data", Empty);
	fields->Set("obsess_over_service", Empty);
	fields->Set("modified_service_attributes", Empty);
	fields->Set("event_handler", Empty);
	fields->Set("check_command", Empty);
	fields->Set("normal_check_interval", Empty);
	fields->Set("retry_check_interval", Empty);
	fields->Set("check_timeperiod_object_id", Empty);

	return fields;
}