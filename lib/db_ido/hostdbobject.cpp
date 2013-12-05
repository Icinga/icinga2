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

#include "db_ido/hostdbobject.h"
#include "db_ido/dbtype.h"
#include "db_ido/dbvalue.h"
#include "icinga/host.h"
#include "icinga/service.h"
#include "icinga/notification.h"
#include "icinga/checkcommand.h"
#include "icinga/eventcommand.h"
#include "icinga/compatutility.h"
#include "base/convert.h"
#include "base/objectlock.h"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_DBTYPE(Host, "host", DbObjectTypeHost, "host_object_id", HostDbObject);

HostDbObject::HostDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr HostDbObject::GetConfigFields(void) const
{
	Dictionary::Ptr fields = make_shared<Dictionary>();
	Host::Ptr host = static_pointer_cast<Host>(GetObject());

	Service::Ptr service = host->GetCheckService();

	fields->Set("alias", CompatUtility::GetHostAlias(host));
	fields->Set("display_name", host->GetDisplayName());
	fields->Set("address", CompatUtility::GetHostAddress(host));
	fields->Set("address6", CompatUtility::GetHostAddress6(host));

	if (service) {
		fields->Set("check_command_object_id", service->GetCheckCommand());
		fields->Set("check_command_args", Empty);
		fields->Set("eventhandler_command_object_id", service->GetEventCommand());
		fields->Set("eventhandler_command_args", Empty);
		fields->Set("notification_timeperiod_object_id", Notification::GetByName(CompatUtility::GetServiceNotificationNotificationPeriod(service)));
		fields->Set("check_timeperiod_object_id", service->GetCheckPeriod());
		fields->Set("failure_prediction_options", Empty);
		fields->Set("check_interval", CompatUtility::GetServiceCheckInterval(service));
		fields->Set("retry_interval", CompatUtility::GetServiceRetryInterval(service));
		fields->Set("max_check_attempts", service->GetMaxCheckAttempts());

		fields->Set("first_notification_delay", Empty);

		fields->Set("notification_interval", CompatUtility::GetServiceNotificationNotificationInterval(service));
		/* requires host check service */
		fields->Set("notify_on_down", CompatUtility::GetHostNotifyOnDown(host));
		fields->Set("notify_on_unreachable", CompatUtility::GetHostNotifyOnDown(host));

		fields->Set("notify_on_recovery", CompatUtility::GetServiceNotifyOnRecovery(service));
		fields->Set("notify_on_flapping", CompatUtility::GetServiceNotifyOnFlapping(service));
		fields->Set("notify_on_downtime", CompatUtility::GetServiceNotifyOnDowntime(service));

		fields->Set("stalk_on_up", Empty);
		fields->Set("stalk_on_down", Empty);
		fields->Set("stalk_on_unreachable", Empty);

		fields->Set("flap_detection_enabled", CompatUtility::GetServiceFlapDetectionEnabled(service));
		fields->Set("flap_detection_on_up", Empty);
		fields->Set("flap_detection_on_down", Empty);
		fields->Set("flap_detection_on_unreachable", Empty);
		fields->Set("low_flap_threshold", CompatUtility::GetServiceLowFlapThreshold(service));
		fields->Set("high_flap_threshold", CompatUtility::GetServiceHighFlapThreshold(service));
	}

	fields->Set("process_performance_data", 0);

	if (service) {
		fields->Set("freshness_checks_enabled", CompatUtility::GetServiceFreshnessChecksEnabled(service));
		fields->Set("freshness_threshold", CompatUtility::GetServiceFreshnessThreshold(service));
		fields->Set("passive_checks_enabled", CompatUtility::GetServicePassiveChecksEnabled(service));
		fields->Set("event_handler_enabled", CompatUtility::GetServiceEventHandlerEnabled(service));
		fields->Set("active_checks_enabled", CompatUtility::GetServiceActiveChecksEnabled(service));
	}

	fields->Set("retain_status_information", 1);
	fields->Set("retain_nonstatus_information", 1);

	if (service)
		fields->Set("notifications_enabled", CompatUtility::GetServiceNotificationsEnabled(service));

	fields->Set("obsess_over_host", 0);
	fields->Set("failure_prediction_enabled", 0);

	fields->Set("notes", CompatUtility::GetCustomAttributeConfig(host, "notes"));
	fields->Set("notes_url", CompatUtility::GetCustomAttributeConfig(host, "notes_url"));
	fields->Set("action_url", CompatUtility::GetCustomAttributeConfig(host, "action_url"));
	fields->Set("icon_image", CompatUtility::GetCustomAttributeConfig(host, "icon_image"));
	fields->Set("icon_image_alt", CompatUtility::GetCustomAttributeConfig(host, "icon_image_alt"));
	fields->Set("statusmap_image", CompatUtility::GetCustomAttributeConfig(host, "statusmap_image"));

	Host2dCoords coords = CompatUtility::GetHost2dCoords(host);

	fields->Set("have_2d_coords", coords.have_2d_coords);

	if (coords.have_2d_coords) {
		fields->Set("x_2d", coords.x_2d);
		fields->Set("y_2d", coords.y_2d);
	}

	/* deprecated in 1.x */
	fields->Set("have_3d_coords", 0);

	return fields;
}

Dictionary::Ptr HostDbObject::GetStatusFields(void) const
{
	Dictionary::Ptr fields = make_shared<Dictionary>();
	Host::Ptr host = static_pointer_cast<Host>(GetObject());
	Service::Ptr service = host->GetCheckService();

	/* fetch service status, or dump a pending hoststatus */
	if (service) {
		CheckResult::Ptr cr = service->GetLastCheckResult();

		if (cr) {
			fields->Set("output", CompatUtility::GetCheckResultOutput(cr));
			fields->Set("long_output", CompatUtility::GetCheckResultLongOutput(cr));
			fields->Set("perfdata", CompatUtility::GetCheckResultPerfdata(cr));
			fields->Set("check_source", cr->GetCheckSource());
		}

		fields->Set("current_state", host->GetState());
		fields->Set("has_been_checked", CompatUtility::GetServiceHasBeenChecked(service));
		fields->Set("should_be_scheduled", CompatUtility::GetServiceShouldBeScheduled(service));
		fields->Set("current_check_attempt", service->GetCheckAttempt());
		fields->Set("max_check_attempts", service->GetMaxCheckAttempts());

		if (cr)
			fields->Set("last_check", DbValue::FromTimestamp(cr->GetScheduleEnd()));

		fields->Set("next_check", DbValue::FromTimestamp(service->GetNextCheck()));
		fields->Set("check_type", CompatUtility::GetServiceCheckType(service));
		fields->Set("last_state_change", DbValue::FromTimestamp(service->GetLastStateChange()));
		fields->Set("last_hard_state_change", DbValue::FromTimestamp(service->GetLastHardStateChange()));
		fields->Set("last_time_up", DbValue::FromTimestamp(static_cast<int>(host->GetLastStateUp())));
		fields->Set("last_time_down", DbValue::FromTimestamp(static_cast<int>(host->GetLastStateDown())));
		fields->Set("last_time_unreachable", DbValue::FromTimestamp(static_cast<int>(host->GetLastStateUnreachable())));
		fields->Set("state_type", service->GetStateType());
		fields->Set("last_notification", DbValue::FromTimestamp(CompatUtility::GetServiceNotificationLastNotification(service)));
		fields->Set("next_notification", DbValue::FromTimestamp(CompatUtility::GetServiceNotificationNextNotification(service)));
		fields->Set("no_more_notifications", Empty);
		fields->Set("notifications_enabled", CompatUtility::GetServiceNotificationsEnabled(service));
		fields->Set("problem_has_been_acknowledged", CompatUtility::GetServiceProblemHasBeenAcknowledged(service));
		fields->Set("acknowledgement_type", CompatUtility::GetServiceAcknowledgementType(service));
		fields->Set("current_notification_number", CompatUtility::GetServiceNotificationNotificationNumber(service));
		fields->Set("passive_checks_enabled", CompatUtility::GetServicePassiveChecksEnabled(service));
		fields->Set("active_checks_enabled", CompatUtility::GetServiceActiveChecksEnabled(service));
		fields->Set("event_handler_enabled", CompatUtility::GetServiceEventHandlerEnabled(service));
		fields->Set("flap_detection_enabled", CompatUtility::GetServiceFlapDetectionEnabled(service));
		fields->Set("is_flapping", CompatUtility::GetServiceIsFlapping(service));
		fields->Set("percent_state_change", CompatUtility::GetServicePercentStateChange(service));

		if (cr) {
			fields->Set("latency", Service::CalculateLatency(cr));
			fields->Set("execution_time", Service::CalculateExecutionTime(cr));
		}
		fields->Set("scheduled_downtime_depth", service->GetDowntimeDepth());
		fields->Set("failure_prediction_enabled", Empty);
		fields->Set("process_performance_data", 0); /* this is a host which does not process any perf data */
		fields->Set("obsess_over_host", Empty);
		fields->Set("modified_host_attributes", service->GetModifiedAttributes());
		fields->Set("event_handler", CompatUtility::GetServiceEventHandler(service));
		fields->Set("check_command", CompatUtility::GetServiceCheckCommand(service));
		fields->Set("normal_check_interval", CompatUtility::GetServiceCheckInterval(service));
		fields->Set("retry_check_interval", CompatUtility::GetServiceRetryInterval(service));
		fields->Set("check_timeperiod_object_id", service->GetCheckPeriod());
	}
	else {
		fields->Set("has_been_checked", 0);
		fields->Set("last_check", DbValue::FromTimestamp(0));
		fields->Set("next_check", DbValue::FromTimestamp(0));
		fields->Set("active_checks_enabled", 0);
	}

	return fields;
}

void HostDbObject::OnConfigUpdate(void)
{
	Host::Ptr host = static_pointer_cast<Host>(GetObject());

	/* parents, host dependencies */
	BOOST_FOREACH(const Host::Ptr& parent, host->GetParentHosts()) {
		Log(LogDebug, "db_ido", "host parents: " + parent->GetName());

		/* parents: host_id, parent_host_object_id */
		Dictionary::Ptr fields1 = make_shared<Dictionary>();
		fields1->Set(GetType()->GetTable() + "_id", DbValue::FromObjectInsertID(GetObject()));
		fields1->Set("parent_host_object_id", parent);
		fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

		DbQuery query1;
		query1.Table = GetType()->GetTable() + "_parenthosts";
		query1.Type = DbQueryInsert;
		query1.Category = DbCatConfig;
		query1.Fields = fields1;
		OnQuery(query1);

		/* host dependencies */
		Dictionary::Ptr fields2 = make_shared<Dictionary>();
		fields2->Set("host_object_id", parent);
		fields2->Set("dependent_host_object_id", host);
		fields2->Set("instance_id", 0); /* DbConnection class fills in real ID */

		DbQuery query2;
		query2.Table = GetType()->GetTable() + "dependencies";
		query2.Type = DbQueryInsert;
		query2.Category = DbCatConfig;
		query2.Fields = fields2;
		OnQuery(query2);
	}

	/* host contacts, contactgroups */
	Service::Ptr service = host->GetCheckService();

	if (service) {
		Log(LogDebug, "db_ido", "host contacts: " + host->GetName());

		BOOST_FOREACH(const User::Ptr& user, CompatUtility::GetServiceNotificationUsers(service)) {
			Log(LogDebug, "db_ido", "host contacts: " + user->GetName());

			Dictionary::Ptr fields_contact = make_shared<Dictionary>();
			fields_contact->Set("host_id", DbValue::FromObjectInsertID(host));
			fields_contact->Set("contact_object_id", user);
			fields_contact->Set("instance_id", 0); /* DbConnection class fills in real ID */

			DbQuery query_contact;
			query_contact.Table = GetType()->GetTable() + "_contacts";
			query_contact.Type = DbQueryInsert;
			query_contact.Category = DbCatConfig;
			query_contact.Fields = fields_contact;
			OnQuery(query_contact);
		}

		Log(LogDebug, "db_ido", "host contactgroups: " + host->GetName());

		BOOST_FOREACH(const UserGroup::Ptr& usergroup, CompatUtility::GetServiceNotificationUserGroups(service)) {
			Log(LogDebug, "db_ido", "host contactgroups: " + usergroup->GetName());

			Dictionary::Ptr fields_contact = make_shared<Dictionary>();
			fields_contact->Set("host_id", DbValue::FromObjectInsertID(host));
			fields_contact->Set("contactgroup_object_id", usergroup);
			fields_contact->Set("instance_id", 0); /* DbConnection class fills in real ID */

			DbQuery query_contact;
			query_contact.Table = GetType()->GetTable() + "_contactgroups";
			query_contact.Type = DbQueryInsert;
			query_contact.Category = DbCatConfig;
			query_contact.Fields = fields_contact;
			OnQuery(query_contact);
		}
	}

	/* custom variables */
	Log(LogDebug, "ido", "host customvars for '" + host->GetName() + "'");

	Dictionary::Ptr customvars;
	{
		ObjectLock olock(host);
		customvars = CompatUtility::GetCustomVariableConfig(host);
	}

	if (customvars) {
		ObjectLock olock (customvars);

		BOOST_FOREACH(const Dictionary::Pair& kv, customvars) {
			Log(LogDebug, "db_ido", "host customvar key: '" + kv.first + "' value: '" + Convert::ToString(kv.second) + "'");

			Dictionary::Ptr fields3 = make_shared<Dictionary>();
			fields3->Set("varname", Convert::ToString(kv.first));
			fields3->Set("varvalue", Convert::ToString(kv.second));
			fields3->Set("config_type", 1);
			fields3->Set("has_been_modified", 0);
			fields3->Set("object_id", host);
			fields3->Set("instance_id", 0); /* DbConnection class fills in real ID */

			DbQuery query3;
			query3.Table = "customvariables";
			query3.Type = DbQueryInsert;
			query3.Category = DbCatConfig;
			query3.Fields = fields3;
			OnQuery(query3);
		}
	}
}

void HostDbObject::OnStatusUpdate(void)
{
}
