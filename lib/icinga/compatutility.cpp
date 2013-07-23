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

#include "base/convert.h"
#include "icinga/compatutility.h"
#include "icinga/checkcommand.h"
#include "icinga/eventcommand.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

Dictionary::Ptr CompatUtility::GetServiceStatusAttributes(const Service::Ptr& service, CompatObjectType type)
{
	Dictionary::Ptr attr = boost::make_shared<Dictionary>();

	ASSERT(service->OwnsLock());

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

		if (line_end > 0 && line_end != String::NPos) {
			long_output = raw_output.SubStr(line_end+1, raw_output.GetLength());
			long_output = EscapeString(long_output);
		}

		boost::algorithm::replace_all(output, "\n", "\\n");

		schedule_end = cr->Get("schedule_end");

		perfdata = cr->Get("performance_data_raw");
		boost::algorithm::replace_all(perfdata, "\n", "\\n");
	}

	int state = service->GetState();

	if (state > StateUnknown)
		state = StateUnknown;

	if (type == CompatTypeHost) {
		if (state == StateOK || state == StateWarning)
			state = 0; /* UP */
		else
			state = 1; /* DOWN */

		Host::Ptr host = service->GetHost();

		ASSERT(host);

		if (!host->IsReachable())
			state = 2; /* UNREACHABLE */
	}

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

	CheckCommand::Ptr checkcommand = service->GetCheckCommand();
	if (checkcommand)
		attr->Set("check_command", "check_" + checkcommand->GetName());

	EventCommand::Ptr eventcommand = service->GetEventCommand();
	if (eventcommand)
		attr->Set("event_handler", "event_" + eventcommand->GetName());

	attr->Set("check_period", check_period_str);
	attr->Set("check_interval", service->GetCheckInterval() / 60.0);
	attr->Set("retry_interval", service->GetRetryInterval() / 60.0);
	attr->Set("has_been_checked", (service->GetLastCheckResult() ? 1 : 0));
	attr->Set("should_be_scheduled", 1);
	attr->Set("check_execution_time", Service::CalculateExecutionTime(cr));
	attr->Set("check_latency", Service::CalculateLatency(cr));
	attr->Set("current_state", state);
	attr->Set("state_type", service->GetStateType());
	attr->Set("plugin_output", output);
	attr->Set("long_plugin_output", long_output);
	attr->Set("performance_data", perfdata);
	attr->Set("last_check", schedule_end);
	attr->Set("next_check", service->GetNextCheck());
	attr->Set("current_attempt", service->GetCurrentCheckAttempt());
	attr->Set("max_attempts", service->GetMaxCheckAttempts());
	attr->Set("last_state_change", service->GetLastStateChange());
	attr->Set("last_hard_state_change", service->GetLastHardStateChange());
	attr->Set("last_time_ok", service->GetLastStateOK());
	attr->Set("last_time_warn", service->GetLastStateWarning());
	attr->Set("last_time_critical", service->GetLastStateCritical());
	attr->Set("last_time_unknown", service->GetLastStateUnknown());
	attr->Set("last_update", time(NULL));
	attr->Set("notifications_enabled", (service->GetEnableNotifications() ? 1 : 0));
	attr->Set("active_checks_enabled", (service->GetEnableActiveChecks() ? 1 : 0));
	attr->Set("passive_checks_enabled", (service->GetEnablePassiveChecks() ? 1 : 0));
	attr->Set("flap_detection_enabled", (service->GetEnableFlapping() ? 1 : 0));
	attr->Set("is_flapping", (service->IsFlapping() ? 1 : 0));
	attr->Set("percent_state_change", Convert::ToString(service->GetFlappingCurrent()));
	attr->Set("problem_has_been_acknowledged", (service->GetAcknowledgement() != AcknowledgementNone ? 1 : 0));
	attr->Set("acknowledgement_type", static_cast<int>(service->GetAcknowledgement()));
	attr->Set("acknowledgement_end_time", service->GetAcknowledgementExpiry());
	attr->Set("scheduled_downtime_depth", (service->IsInDowntime() ? 1 : 0));
	attr->Set("last_notification", last_notification);
	attr->Set("next_notification", next_notification);
	attr->Set("current_notification_number", notification_number);;

	return attr;
}

String CompatUtility::EscapeString(const String& str)
{
	String result = str;
	boost::algorithm::replace_all(result, "\n", "\\n");
	return result;
}
