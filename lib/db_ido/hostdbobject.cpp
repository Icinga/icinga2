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
#include <boost/tuple/tuple.hpp>

using namespace icinga;

REGISTER_DBTYPE(Host, "host", DbObjectTypeHost, "host_object_id", HostDbObject);

HostDbObject::HostDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr HostDbObject::GetConfigFields(void) const
{
	Dictionary::Ptr fields = boost::make_shared<Dictionary>();
	Host::Ptr host = static_pointer_cast<Host>(GetObject());

	Service::Ptr service = host->GetCheckService();

	Dictionary::Ptr attrs;

	{
		ObjectLock olock(host);
		attrs = CompatUtility::GetHostConfigAttributes(host);
	}

	fields->Set("alias", attrs->Get("alias"));
	fields->Set("display_name", attrs->Get("display_name"));

	fields->Set("address", attrs->Get("address"));
	fields->Set("address6", attrs->Get("address6"));

	if (service) {
		fields->Set("check_command_object_id", service->GetCheckCommand());
		fields->Set("check_command_args", Empty);
		fields->Set("eventhandler_command_object_id", service->GetEventCommand());
		fields->Set("eventhandler_command_args", Empty);
	}

	fields->Set("notification_timeperiod_object_id", Notification::GetByName(attrs->Get("notification_period")));

	if (service)
		fields->Set("check_timeperiod_object_id", service->GetCheckPeriod());

	fields->Set("failure_prediction_options", Empty);

	fields->Set("check_interval", attrs->Get("check_interval"));
	fields->Set("retry_interval", attrs->Get("retry_interval"));
	fields->Set("max_check_attempts", attrs->Get("max_check_attempts"));

	fields->Set("first_notification_delay", Empty);
	fields->Set("notification_interval", attrs->Get("notification_interval"));
	fields->Set("notify_on_down", attrs->Get("notify_on_down"));
	fields->Set("notify_on_unreachable", attrs->Get("notify_on_unreachable"));
	fields->Set("notify_on_recovery", attrs->Get("notify_on_recovery"));
	fields->Set("notify_on_flapping", attrs->Get("notify_on_flapping"));
	fields->Set("notify_on_downtime", attrs->Get("notify_on_downtime"));

	fields->Set("stalk_on_up", Empty);
	fields->Set("stalk_on_down", Empty);
	fields->Set("stalk_on_unreachable", Empty);

	fields->Set("flap_detection_enabled", attrs->Get("flap_detection_enabled"));
	fields->Set("flap_detection_on_up", Empty);
	fields->Set("flap_detection_on_down", Empty);
	fields->Set("flap_detection_on_unreachable", Empty);
	fields->Set("low_flap_threshold", attrs->Get("low_flap_threshold"));
	fields->Set("high_flap_threshold", attrs->Get("high_flap_threshold"));
	fields->Set("process_performance_data", attrs->Get("process_performance_data"));
	fields->Set("freshness_checks_enabled", attrs->Get("check_freshness"));
	fields->Set("freshness_threshold", Empty);
	fields->Set("passive_checks_enabled", attrs->Get("passive_checks_enabled"));
	fields->Set("event_handler_enabled", attrs->Get("event_handler_enabled"));
	fields->Set("active_checks_enabled", attrs->Get("active_checks_enabled"));
	fields->Set("retain_status_information", 1);
	fields->Set("retain_nonstatus_information", 1);
	fields->Set("notifications_enabled", attrs->Get("notifications_enabled"));
	fields->Set("obsess_over_host", 0);
	fields->Set("failure_prediction_enabled", 0);

	fields->Set("notes", attrs->Get("notes"));
	fields->Set("notes_url", attrs->Get("notes_url"));
	fields->Set("action_url", attrs->Get("action_url"));
	fields->Set("icon_image", attrs->Get("icon_image"));
	fields->Set("icon_image_alt", attrs->Get("icon_image_alt"));
	fields->Set("statusmap_image", attrs->Get("statusmap_image"));
	fields->Set("have_2d_coords", attrs->Get("have_2d_coords"));
	fields->Set("x_2d", attrs->Get("x_2d"));
	fields->Set("y_2d", attrs->Get("y_2d"));
	/* deprecated in 1.x */
	fields->Set("have_3d_coords", 0);

	return fields;
}

Dictionary::Ptr HostDbObject::GetStatusFields(void) const
{
	Dictionary::Ptr fields = boost::make_shared<Dictionary>();
	Host::Ptr host = static_pointer_cast<Host>(GetObject());
	Service::Ptr service = host->GetCheckService();

	Dictionary::Ptr attrs;

	/* fetch service status, or dump a pending hoststatus */
	if (service) {
		ObjectLock olock(service);
		attrs = CompatUtility::GetServiceStatusAttributes(service, CompatTypeHost);

		fields->Set("output", attrs->Get("plugin_output"));
		fields->Set("long_output", attrs->Get("long_plugin_output"));
		fields->Set("perfdata", attrs->Get("performance_data"));
		fields->Set("check_source", attrs->Get("check_source"));
		fields->Set("current_state", attrs->Get("current_state"));
		fields->Set("has_been_checked", attrs->Get("has_been_checked"));
		fields->Set("should_be_scheduled", attrs->Get("should_be_scheduled"));
		fields->Set("current_check_attempt", attrs->Get("current_attempt"));
		fields->Set("max_check_attempts", attrs->Get("max_attempts"));
		fields->Set("last_check", DbValue::FromTimestamp(attrs->Get("last_check")));
		fields->Set("next_check", DbValue::FromTimestamp(attrs->Get("next_check")));
		fields->Set("check_type", attrs->Get("check_type"));
		fields->Set("last_state_change", DbValue::FromTimestamp(attrs->Get("last_state_change")));
		fields->Set("last_hard_state_change", DbValue::FromTimestamp(attrs->Get("last_hard_state_change")));
		fields->Set("last_time_up", DbValue::FromTimestamp(attrs->Get("last_time_up")));
		fields->Set("last_time_down", DbValue::FromTimestamp(attrs->Get("last_time_down")));
		fields->Set("last_time_unreachable", DbValue::FromTimestamp(attrs->Get("last_time_unreachable")));
		fields->Set("state_type", attrs->Get("state_type"));
		fields->Set("last_notification", DbValue::FromTimestamp(attrs->Get("last_notification")));
		fields->Set("next_notification", DbValue::FromTimestamp(attrs->Get("next_notification")));
		fields->Set("no_more_notifications", Empty);
		fields->Set("notifications_enabled", attrs->Get("notifications_enabled"));
		fields->Set("problem_has_been_acknowledged", attrs->Get("problem_has_been_acknowledged"));
		fields->Set("acknowledgement_type", attrs->Get("acknowledgement_type"));
		fields->Set("current_notification_number", attrs->Get("current_notification_number"));
		fields->Set("passive_checks_enabled", attrs->Get("passive_checks_enabled"));
		fields->Set("active_checks_enabled", attrs->Get("active_checks_enabled"));
		fields->Set("eventhandler_enabled", attrs->Get("eventhandler_enabled"));
		fields->Set("flap_detection_enabled", attrs->Get("flap_detection_enabled"));
		fields->Set("is_flapping", attrs->Get("is_flapping"));
		fields->Set("percent_state_change", attrs->Get("percent_state_change"));
		fields->Set("latency", attrs->Get("check_latency"));
		fields->Set("execution_time", attrs->Get("check_execution_time"));
		fields->Set("scheduled_downtime_depth", attrs->Get("scheduled_downtime_depth"));
		fields->Set("failure_prediction_enabled", Empty);
		fields->Set("process_performance_data", attrs->Get("process_performance_data"));
		fields->Set("obsess_over_host", Empty);
		fields->Set("modified_host_attributes", attrs->Get("modified_attributes"));
		fields->Set("event_handler", attrs->Get("event_handler"));
		fields->Set("check_command", attrs->Get("check_command"));
		fields->Set("normal_check_interval", attrs->Get("check_interval"));
		fields->Set("retry_check_interval", attrs->Get("retry_interval"));
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
	DbQuery query_del1;
	query_del1.Table = GetType()->GetTable() + "_parenthosts";
	query_del1.Type = DbQueryDelete;
	query_del1.WhereCriteria = boost::make_shared<Dictionary>();
	query_del1.WhereCriteria->Set(GetType()->GetTable() + "_id", DbValue::FromObjectInsertID(GetObject()));
	OnQuery(query_del1);

	DbQuery query_del2;
	query_del2.Table = GetType()->GetTable() + "dependencies";
	query_del2.Type = DbQueryDelete;
	query_del2.WhereCriteria = boost::make_shared<Dictionary>();
	query_del2.WhereCriteria->Set("dependent_host_object_id", host);
	OnQuery(query_del2);

	BOOST_FOREACH(const Host::Ptr& parent, host->GetParentHosts()) {
		Log(LogDebug, "db_ido", "host parents: " + parent->GetName());

		/* parents: host_id, parent_host_object_id */
		Dictionary::Ptr fields1 = boost::make_shared<Dictionary>();
		fields1->Set(GetType()->GetTable() + "_id", DbValue::FromObjectInsertID(GetObject()));
		fields1->Set("parent_host_object_id", parent);
		fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

		DbQuery query1;
		query1.Table = GetType()->GetTable() + "_parenthosts";
		query1.Type = DbQueryInsert;
		query1.Fields = fields1;
		OnQuery(query1);

		/* host dependencies */
		Dictionary::Ptr fields2 = boost::make_shared<Dictionary>();
		fields2->Set("host_object_id", parent);
		fields2->Set("dependent_host_object_id", host);
		fields2->Set("instance_id", 0); /* DbConnection class fills in real ID */

		DbQuery query2;
		query2.Table = GetType()->GetTable() + "dependencies";
		query2.Type = DbQueryInsert;
		query2.Fields = fields2;
		OnQuery(query2);
	}

	/* host contacts, contactgroups */
	Service::Ptr service = host->GetCheckService();

	if (service) {
		Log(LogDebug, "db_ido", "host contacts: " + host->GetName());

		BOOST_FOREACH(const User::Ptr& user, CompatUtility::GetServiceNotificationUsers(service)) {
			Log(LogDebug, "db_ido", "host contacts: " + user->GetName());

			Dictionary::Ptr fields_contact = boost::make_shared<Dictionary>();
			fields_contact->Set("host_id", DbValue::FromObjectInsertID(host));
			fields_contact->Set("contact_object_id", user);
			fields_contact->Set("instance_id", 0); /* DbConnection class fills in real ID */

			DbQuery query_contact;
			query_contact.Table = GetType()->GetTable() + "_contacts";
			query_contact.Type = DbQueryInsert;
			query_contact.Fields = fields_contact;
			OnQuery(query_contact);
		}

		Log(LogDebug, "db_ido", "host contactgroups: " + host->GetName());

		BOOST_FOREACH(const UserGroup::Ptr& usergroup, CompatUtility::GetServiceNotificationUserGroups(service)) {
			Log(LogDebug, "db_ido", "host contactgroups: " + usergroup->GetName());

			Dictionary::Ptr fields_contact = boost::make_shared<Dictionary>();
			fields_contact->Set("host_id", DbValue::FromObjectInsertID(host));
			fields_contact->Set("contactgroup_object_id", usergroup);
			fields_contact->Set("instance_id", 0); /* DbConnection class fills in real ID */

			DbQuery query_contact;
			query_contact.Table = GetType()->GetTable() + "_contactgroups";
			query_contact.Type = DbQueryInsert;
			query_contact.Fields = fields_contact;
			OnQuery(query_contact);
		}
	}

	/* custom variables */
	Log(LogDebug, "ido", "host customvars for '" + host->GetName() + "'");

	DbQuery query_del3;
	query_del3.Table = "customvariables";
	query_del3.Type = DbQueryDelete;
	query_del3.WhereCriteria = boost::make_shared<Dictionary>();
	query_del3.WhereCriteria->Set("object_id", host);
	OnQuery(query_del3);

	Dictionary::Ptr customvars;
	{
		ObjectLock olock(host);
		customvars = CompatUtility::GetCustomVariableConfig(host);
	}

	if (customvars) {
		ObjectLock olock (customvars);

		String key;
		Value value;
		BOOST_FOREACH(boost::tie(key, value), customvars) {
			Log(LogDebug, "db_ido", "host customvar key: '" + key + "' value: '" + Convert::ToString(value) + "'");

			Dictionary::Ptr fields3 = boost::make_shared<Dictionary>();
			fields3->Set("varname", Convert::ToString(key));
			fields3->Set("varvalue", Convert::ToString(value));
			fields3->Set("config_type", 1);
			fields3->Set("has_been_modified", 0);
			fields3->Set("object_id", host);
			fields3->Set("instance_id", 0); /* DbConnection class fills in real ID */

			DbQuery query3;
			query3.Table = "customvariables";
			query3.Type = DbQueryInsert;
			query3.Fields = fields3;
			OnQuery(query3);
		}
	}
}

void HostDbObject::OnStatusUpdate(void)
{
}
