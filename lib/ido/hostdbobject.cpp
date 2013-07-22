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

#include "ido/hostdbobject.h"
#include "ido/dbtype.h"
#include "icinga/host.h"
#include "icinga/service.h"
#include "icinga/checkcommand.h"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_DBTYPE("Host", "host", 1, HostDbObject);

HostDbObject::HostDbObject(const String& name1, const String& name2)
	: DbObject(DbType::GetByName("Host"), name1, name2)
{ }

Dictionary::Ptr HostDbObject::GetConfigFields(void) const
{
	Dictionary::Ptr fields = boost::make_shared<Dictionary>();
	Host::Ptr host = static_pointer_cast<Host>(GetObject());

	Service::Ptr service = host->GetHostCheckService();

	if (!service)
		return Empty;

	fields->Set("alias", host->GetName());
	fields->Set("display_name", host->GetDisplayName());

	fields->Set("check_command_object_id", service->GetCheckCommand());
	fields->Set("check_command_args", Empty);
	fields->Set("check_interval", service->GetCheckInterval() / 60);
	fields->Set("retry_interval", service->GetRetryInterval() / 60);
	fields->Set("max_check_attempts", service->GetMaxCheckAttempts());

	fields->Set("address", Empty);
	fields->Set("address6", Empty);
	fields->Set("eventhandler_command_object_id", Empty);
	fields->Set("eventhandler_command_args", Empty);
	fields->Set("notification_timeperiod_object_id", Empty);
	fields->Set("check_timeperiod_object_id", Empty);
	fields->Set("failure_prediction_options", Empty);
	fields->Set("first_notification_delay", Empty);
	fields->Set("notification_interval", Empty);
	fields->Set("notify_on_down", Empty);
	fields->Set("notify_on_unreachable", Empty);
	fields->Set("notify_on_recovery", Empty);
	fields->Set("notify_on_flapping", Empty);
	fields->Set("notify_on_downtime", Empty);
	fields->Set("stalk_on_up", Empty);
	fields->Set("stalk_on_down", Empty);
	fields->Set("stalk_on_unreachable", Empty);
	fields->Set("flap_detection_enabled", Empty);
	fields->Set("flap_detection_on_up", Empty);
	fields->Set("flap_detection_on_down", Empty);
	fields->Set("flap_detection_on_unreachable", Empty);
	fields->Set("low_flap_threshold", Empty);
	fields->Set("high_flap_threshold", Empty);
	fields->Set("process_performance_data", Empty);
	fields->Set("freshness_checks_enabled", Empty);
	fields->Set("freshness_threshold", Empty);
	fields->Set("passive_checks_enabled", Empty);
	fields->Set("event_handler_enabled", Empty);
	fields->Set("active_checks_enabled", Empty);
	fields->Set("retain_status_information", 1);
	fields->Set("retain_nonstatus_information", 1);
	fields->Set("notifications_enabled", 1);
	fields->Set("obsess_over_host", 0);
	fields->Set("failure_prediction_enabled", 0);
	fields->Set("notes", Empty);
	fields->Set("notes_url", Empty);
	fields->Set("action_url", Empty);
	fields->Set("icon_image", Empty);
	fields->Set("icon_image_alt", Empty);
	fields->Set("vrml_image", Empty);
	fields->Set("statusmap_image", Empty);
	fields->Set("have_2d_coords", Empty);
	fields->Set("x_2d", Empty);
	fields->Set("y_2d", Empty);
	fields->Set("have_3d_coords", Empty);
	fields->Set("x_3d", Empty);
	fields->Set("y_3d", Empty);
	fields->Set("z_3d", Empty);

	return fields;
}

Dictionary::Ptr HostDbObject::GetStatusFields(void) const
{
	Dictionary::Ptr fields = boost::make_shared<Dictionary>();
	Host::Ptr host = static_pointer_cast<Host>(GetObject());
	Service::Ptr service = host->GetHostCheckService();

	if (!service)
		return Empty;

	String raw_output;
	String output;
	String long_output;
	String perfdata;
	double schedule_end = -1;

	String check_period_str;
	TimePeriod::Ptr check_period = service->GetCheckPeriod();
	if (check_period)
		check_period_str = check_period->GetName();
	else
		check_period_str = "24x7";

	Dictionary::Ptr cr = service->GetLastCheckResult();

	if (cr) {
		raw_output = cr->Get("output");
		size_t line_end = raw_output.Find("\n");

		output = raw_output.SubStr(0, line_end);

		if (line_end > 0 && line_end != String::NPos)
			long_output = raw_output.SubStr(line_end+1, raw_output.GetLength());

		boost::algorithm::replace_all(output, "\n", "\\n");

		schedule_end = cr->Get("schedule_end");

		perfdata = cr->Get("performance_data_raw");
		boost::algorithm::replace_all(perfdata, "\n", "\\n");
	}

	int state = service->GetState();

	if (state > StateUnknown)
		state = StateUnknown;

//	if (type == CompatTypeHost) {
		if (state == StateOK || state == StateWarning)
			state = 0; /* UP */
		else
			state = 1; /* DOWN */

		if (!host->IsReachable())
			state = 2; /* UNREACHABLE */
//	}

	double last_notification = 0;
	double next_notification = 0;
	int notification_number = 0;
	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
		if (notification->GetLastNotification() > last_notification)
			last_notification = notification->GetLastNotification();

		if (notification->GetNextNotification() < next_notification)
			next_notification = notification->GetNextNotification();

		if (notification->GetNotificationNumber() > notification_number)
			notification_number = notification->GetNotificationNumber();
	}
	fields->Set("output", output);
	fields->Set("long_output", long_output);
	fields->Set("perfdata", perfdata);
	fields->Set("current_state", state);
	fields->Set("has_been_checked", (service->GetLastCheckResult() ? 1 : 0));
	fields->Set("should_be_scheduled", 1);
	fields->Set("current_check_attempt", service->GetCurrentCheckAttempt());
	fields->Set("max_check_attempts", service->GetMaxCheckAttempts());
	fields->Set("last_check", schedule_end);
	fields->Set("next_check", service->GetNextCheck());
	fields->Set("check_type", Empty);
	fields->Set("last_state_change", Empty);
	fields->Set("last_hard_state_change", Empty);
	fields->Set("last_hard_state", Empty);
	fields->Set("last_time_up", Empty);
	fields->Set("last_time_down", Empty);
	fields->Set("last_time_unreachable", Empty);
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
	fields->Set("obsess_over_host", Empty);
	fields->Set("modified_host_attributes", Empty);
	fields->Set("event_handler", Empty);
	fields->Set("check_command", Empty);
	fields->Set("normal_check_interval", Empty);
	fields->Set("retry_check_interval", Empty);
	fields->Set("check_timeperiod_object_id", Empty);

	return fields;
}
