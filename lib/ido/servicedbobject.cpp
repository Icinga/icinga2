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
#include "ido/dbvalue.h"
#include "base/convert.h"
#include "base/objectlock.h"
#include "base/initialize.h"
#include "base/dynamictype.h"
#include "base/utility.h"
#include "icinga/notification.h"
#include "icinga/checkcommand.h"
#include "icinga/eventcommand.h"
#include "icinga/compatutility.h"
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

using namespace icinga;

REGISTER_DBTYPE(Service, "service", DbObjectTypeService, "service_object_id", ServiceDbObject);

INITIALIZE_ONCE(ServiceDbObject, &ServiceDbObject::StaticInitialize);

void ServiceDbObject::StaticInitialize(void)
{
	/* Status */
	Service::OnCommentAdded.connect(boost::bind(&ServiceDbObject::AddComment, _1, _2));
	Service::OnCommentRemoved.connect(boost::bind(&ServiceDbObject::RemoveComment, _1, _2));
	Service::OnDowntimeAdded.connect(boost::bind(&ServiceDbObject::AddDowntime, _1, _2));
	Service::OnDowntimeRemoved.connect(boost::bind(&ServiceDbObject::RemoveDowntime, _1, _2));
	Service::OnDowntimeTriggered.connect(boost::bind(&ServiceDbObject::TriggerDowntime, _1, _2));

	/* History */
	Service::OnAcknowledgementSet.connect(boost::bind(&ServiceDbObject::AddAcknowledgement, _1, _2, _3, _4, _5, _6));

}

ServiceDbObject::ServiceDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr ServiceDbObject::GetConfigFields(void) const
{
	Dictionary::Ptr fields = boost::make_shared<Dictionary>();
	Service::Ptr service = static_pointer_cast<Service>(GetObject());

	Host::Ptr host = service->GetHost();

	if (!host)
		return Dictionary::Ptr();

	Dictionary::Ptr attrs;

	{
		ObjectLock olock(service);
		attrs = CompatUtility::GetServiceConfigAttributes(service);
	}

	fields->Set("host_object_id", host);
	fields->Set("display_name", service->GetDisplayName());
	fields->Set("check_command_object_id", service->GetCheckCommand());
	fields->Set("check_command_args", Empty);
	fields->Set("eventhandler_command_object_id", service->GetEventCommand());
	fields->Set("eventhandler_command_args", Empty);
	fields->Set("notification_timeperiod_object_id", Notification::GetByName(attrs->Get("notification_period")));
	fields->Set("check_timeperiod_object_id", service->GetCheckPeriod());
	fields->Set("failure_prediction_options", Empty);
	fields->Set("check_interval", attrs->Get("check_interval"));
	fields->Set("retry_interval", attrs->Get("retry_interval"));
	fields->Set("max_check_attempts", attrs->Get("max_check_attempts"));
	fields->Set("first_notification_delay", Empty);
	fields->Set("notification_interval", attrs->Get("notification_interval"));
	fields->Set("notify_on_warning", attrs->Get("notify_on_warning"));
	fields->Set("notify_on_unknown", attrs->Get("notify_on_unknown"));
	fields->Set("notify_on_critical", attrs->Get("notify_on_critical"));
	fields->Set("notify_on_recovery", attrs->Get("notify_on_recovery"));
	fields->Set("notify_on_flapping", attrs->Get("notify_on_flapping"));
	fields->Set("notify_on_downtime", attrs->Get("notify_on_downtime"));
	fields->Set("stalk_on_ok", 0);
	fields->Set("stalk_on_warning", 0);
	fields->Set("stalk_on_unknown", 0);
	fields->Set("stalk_on_critical", 0);
	fields->Set("is_volatile", attrs->Get("is_volatile"));
	fields->Set("flap_detection_enabled", attrs->Get("flap_detection_enabled"));
	fields->Set("flap_detection_on_ok", Empty);
	fields->Set("flap_detection_on_warning", Empty);
	fields->Set("flap_detection_on_unknown", Empty);
	fields->Set("flap_detection_on_critical", Empty);
	fields->Set("low_flap_threshold", attrs->Get("low_flap_threshold"));
	fields->Set("high_flap_threshold", attrs->Get("high_flap_threshold"));
	fields->Set("process_performance_data", attrs->Get("process_performance_data"));
	fields->Set("freshness_checks_enabled", attrs->Get("freshness_checks_enabled"));
	fields->Set("freshness_threshold", Empty);
	fields->Set("passive_checks_enabled", attrs->Get("passive_checks_enabled"));
	fields->Set("event_handler_enabled", attrs->Get("event_handler_enabled"));
	fields->Set("active_checks_enabled", attrs->Get("active_checks_enabled"));
	fields->Set("retain_status_information", Empty);
	fields->Set("retain_nonstatus_information", Empty);
	fields->Set("notifications_enabled", attrs->Get("notifications_enabled"));
	fields->Set("obsess_over_service", Empty);
	fields->Set("failure_prediction_enabled", Empty);
	fields->Set("notes", attrs->Get("notes"));
	fields->Set("notes_url", attrs->Get("notes_url"));
	fields->Set("action_url", attrs->Get("action_url"));
	fields->Set("icon_image", attrs->Get("icon_image"));
	fields->Set("icon_image_alt", attrs->Get("icon_image_alt"));

	return fields;
}

Dictionary::Ptr ServiceDbObject::GetStatusFields(void) const
{
	Dictionary::Ptr fields = boost::make_shared<Dictionary>();
	Service::Ptr service = static_pointer_cast<Service>(GetObject());
	Dictionary::Ptr attrs;

	{
		ObjectLock olock(service);
		attrs = CompatUtility::GetServiceStatusAttributes(service, CompatTypeService);
	}

	fields->Set("output", attrs->Get("plugin_output"));
	fields->Set("long_output", attrs->Get("long_plugin_output"));
	fields->Set("perfdata", attrs->Get("performance_data"));
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
	fields->Set("last_time_ok", DbValue::FromTimestamp(attrs->Get("last_time_ok")));
	fields->Set("last_time_warning", DbValue::FromTimestamp(attrs->Get("last_time_warn")));
	fields->Set("last_time_critical", DbValue::FromTimestamp(attrs->Get("last_time_critical")));
	fields->Set("last_time_unknown", DbValue::FromTimestamp(attrs->Get("last_time_unknown")));
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
	fields->Set("event_handler_enabled", attrs->Get("event_handler_enabled"));
	fields->Set("flap_detection_enabled", attrs->Get("flap_detection_enabled"));
	fields->Set("is_flapping", attrs->Get("is_flapping"));
	fields->Set("percent_state_change", attrs->Get("percent_state_change"));
	fields->Set("latency", attrs->Get("check_latency"));
	fields->Set("execution_time", attrs->Get("check_execution_time"));
	fields->Set("scheduled_downtime_depth", attrs->Get("scheduled_downtime_depth"));
	fields->Set("process_performance_data", attrs->Get("process_performance_data"));
	fields->Set("event_handler", attrs->Get("event_handler"));
	fields->Set("check_command", attrs->Get("check_command"));
	fields->Set("normal_check_interval", attrs->Get("check_interval"));
	fields->Set("retry_check_interval", attrs->Get("retry_interval"));
	fields->Set("check_timeperiod_object_id", service->GetCheckPeriod());

	return fields;
}

bool ServiceDbObject::IsStatusAttribute(const String& attribute) const
{
	return (attribute == "last_result");
}

void ServiceDbObject::OnConfigUpdate(void)
{
	Service::Ptr service = static_pointer_cast<Service>(GetObject());

	/* service dependencies */
	Log(LogDebug, "ido", "service dependencies for '" + service->GetName() + "'");

	DbQuery query_del1;
	query_del1.Table = GetType()->GetTable() + "dependencies";
	query_del1.Type = DbQueryDelete;
	query_del1.WhereCriteria = boost::make_shared<Dictionary>();
	query_del1.WhereCriteria->Set("dependent_service_object_id", service);
	OnQuery(query_del1);

	BOOST_FOREACH(const Service::Ptr& parent, service->GetParentServices()) {
		Log(LogDebug, "ido", "service parents: " + parent->GetName());

                /* service dependencies */
                Dictionary::Ptr fields1 = boost::make_shared<Dictionary>();
                fields1->Set("service_object_id", parent);
                fields1->Set("dependent_service_object_id", service);
                fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

                DbQuery query1;
                query1.Table = GetType()->GetTable() + "dependencies";
                query1.Type = DbQueryInsert;
                query1.Fields = fields1;
                OnQuery(query1);
	}

	/* custom variables */
	Log(LogDebug, "ido", "service customvars for '" + service->GetName() + "'");

	DbQuery query_del2;
	query_del2.Table = "customvariables";
	query_del2.Type = DbQueryDelete;
	query_del2.WhereCriteria = boost::make_shared<Dictionary>();
	query_del2.WhereCriteria->Set("object_id", service);
	OnQuery(query_del2);

	Dictionary::Ptr customvars;

	{
		ObjectLock olock(service);
		customvars = CompatUtility::GetCustomVariableConfig(service);
	}

	if (customvars) {
		ObjectLock olock (customvars);

		String key;
		Value value;
		BOOST_FOREACH(boost::tie(key, value), customvars) {
			Log(LogDebug, "ido", "service customvar key: '" + key + "' value: '" + Convert::ToString(value) + "'");

			Dictionary::Ptr fields2 = boost::make_shared<Dictionary>();
			fields2->Set("varname", Convert::ToString(key));
			fields2->Set("varvalue", Convert::ToString(value));
			fields2->Set("config_type", 1);
			fields2->Set("has_been_modified", 0);
			fields2->Set("object_id", service);
			fields2->Set("instance_id", 0); /* DbConnection class fills in real ID */

			DbQuery query2;
			query2.Table = "customvariables";
			query2.Type = DbQueryInsert;
			query2.Fields = fields2;
			OnQuery(query2);
		}
	}

	/* update comments and downtimes on config change */
	RemoveComments(service);
	RemoveDowntimes(service);
	AddComments(service);
	AddDowntimes(service);
 
	/* service host config update */
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	if (host->GetHostCheckService() != service)
		return;

	DbObject::Ptr dbobj = GetOrCreateByObject(host);

	if (!dbobj)
		return;

	dbobj->SendConfigUpdate();
}

void ServiceDbObject::OnStatusUpdate(void)
{
	Service::Ptr service = static_pointer_cast<Service>(GetObject());
	Host::Ptr host = service->GetHost();

	/* update the host if hostcheck service */
	if (!host)
		return;

	if (host->GetHostCheckService() != service)
		return;

	DbObject::Ptr dbobj = GetOrCreateByObject(host);

	if (!dbobj)
		return;

	dbobj->SendStatusUpdate();
}

void ServiceDbObject::AddComments(const Service::Ptr& service)
{
	/* dump all comments */
	Dictionary::Ptr comments = service->GetComments();

	if (!comments)
		return;

	ObjectLock olock(comments);

	String comment_id;
	Dictionary::Ptr comment;
	BOOST_FOREACH(boost::tie(comment_id, comment), comments) {
		AddComment(service, comment);
	}
}

void ServiceDbObject::AddComment(const Service::Ptr& service, const Dictionary::Ptr& comment)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	if (!comment) {
		Log(LogWarning, "ido", "comment does not exist. not adding it.");
		return;
	}

	Log(LogDebug, "ido", "adding service comment (id = " + comment->Get("legacy_id") + ") for '" + service->GetName() + "'");

	/* add the service comment */
	AddCommentByType(service, comment);

	/* add the hostcheck service comment to the host as well */
	if (host->GetHostCheckService() == service) {
		Log(LogDebug, "ido", "adding host comment (id = " + comment->Get("legacy_id") + ") for '" + host->GetName() + "'");
		AddCommentByType(host, comment);
	}
}

void ServiceDbObject::AddCommentByType(const DynamicObject::Ptr& object, const Dictionary::Ptr& comment)
{
	unsigned long entry_time = static_cast<long>(comment->Get("entry_time"));
	unsigned long entry_time_usec = (comment->Get("entry_time") - entry_time) * 1000 * 1000;

	Dictionary::Ptr fields1 = boost::make_shared<Dictionary>();
	fields1->Set("entry_time", DbValue::FromTimestamp(entry_time));
	fields1->Set("entry_time_usec", entry_time_usec);
	fields1->Set("entry_type", comment->Get("entry_type"));
	fields1->Set("object_id", object);

	if (object->GetType() == DynamicType::GetByName("Host")) {
		fields1->Set("comment_type", 2);
		/* requires idoutils 1.10 schema fix */
		fields1->Set("internal_comment_id", comment->Get("legacy_id"));
	} else if (object->GetType() == DynamicType::GetByName("Service")) {
		fields1->Set("comment_type", 1);
		fields1->Set("internal_comment_id", comment->Get("legacy_id"));
	} else {
		Log(LogDebug, "ido", "unknown object type for adding comment.");
		return;
	}

	fields1->Set("comment_time", DbValue::FromTimestamp(entry_time)); /* same as entry_time */
	fields1->Set("author_name", comment->Get("author"));
	fields1->Set("comment_data", comment->Get("text"));
	fields1->Set("is_persistent", 1);
	fields1->Set("comment_source", 1); /* external */
	fields1->Set("expires", (comment->Get("expire_time") > 0) ? 1 : 0);
	fields1->Set("expiration_time", comment->Get("expire_time"));
	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	DbQuery query1;
	query1.Table = "comments";
	query1.Type = DbQueryInsert;
	query1.Fields = fields1;
	OnQuery(query1);
}

void ServiceDbObject::RemoveComments(const Service::Ptr& service)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	Log(LogDebug, "ido", "removing service comments for '" + service->GetName() + "'");

	DbQuery query1;
	query1.Table = "comments";
	query1.Type = DbQueryDelete;
	query1.WhereCriteria = boost::make_shared<Dictionary>();
	query1.WhereCriteria->Set("object_id", service);
	OnQuery(query1);

	/* delete hostcheck service's host comments */
	if (host->GetHostCheckService() == service) {
		DbQuery query2;
		query2.Table = "comments";
		query2.Type = DbQueryDelete;
		query2.WhereCriteria = boost::make_shared<Dictionary>();
		query2.WhereCriteria->Set("object_id", host);
		OnQuery(query2);
	}
}

void ServiceDbObject::RemoveComment(const Service::Ptr& service, const Dictionary::Ptr& comment)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	if (!comment) {
		Log(LogWarning, "ido", "comment does not exist. not adding it.");
		return;
	}

	Log(LogDebug, "ido", "removing service comment (id = " + comment->Get("legacy_id") + ") for '" + service->GetName() + "'");

	DbQuery query1;
	query1.Table = "comments";
	query1.Type = DbQueryDelete;
	query1.WhereCriteria = boost::make_shared<Dictionary>();
	query1.WhereCriteria->Set("object_id", service);
	query1.WhereCriteria->Set("internal_comment_id", comment->Get("legacy_id"));
	OnQuery(query1);

	/* delete hostcheck service's host comments */
	if (host->GetHostCheckService() == service) {
		DbQuery query2;
		query2.Table = "comments";
		query2.Type = DbQueryDelete;
		query2.WhereCriteria = boost::make_shared<Dictionary>();
		query2.WhereCriteria->Set("object_id", host);
		query2.WhereCriteria->Set("internal_comment_id", comment->Get("legacy_id"));
		OnQuery(query2);
	}
}

void ServiceDbObject::AddDowntimes(const Service::Ptr& service)
{
	/* dump all downtimes */
	Dictionary::Ptr downtimes = service->GetDowntimes();

	if (!downtimes)
		return;

	ObjectLock olock(downtimes);

	String downtime_id;
	Dictionary::Ptr downtime;
	BOOST_FOREACH(boost::tie(downtime_id, downtime), downtimes) {
		AddDowntime(service, downtime);
	}
}

void ServiceDbObject::AddDowntime(const Service::Ptr& service, const Dictionary::Ptr& downtime)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	if (!downtime) {
		Log(LogWarning, "ido", "downtime does not exist. not adding it.");
		return;
	}

	Log(LogDebug, "ido", "adding service downtime (id = " + downtime->Get("legacy_id") + ") for '" + service->GetName() + "'");

	/* add the service downtime */
	AddDowntimeByType(service, downtime);

	/* add the hostcheck service downtime to the host as well */
	if (host->GetHostCheckService() == service) {
		Log(LogDebug, "ido", "adding host downtime (id = " + downtime->Get("legacy_id") + ") for '" + host->GetName() + "'");
		AddDowntimeByType(host, downtime);
	}
}

void ServiceDbObject::AddDowntimeByType(const DynamicObject::Ptr& object, const Dictionary::Ptr& downtime)
{
	Dictionary::Ptr fields1 = boost::make_shared<Dictionary>();
	fields1->Set("entry_time", DbValue::FromTimestamp(downtime->Get("entry_time")));
	fields1->Set("object_id", object);

	if (object->GetType() == DynamicType::GetByName("Host")) {
		fields1->Set("downtime_type", 2);
		/* requires idoutils 1.10 schema fix */
		fields1->Set("internal_downtime_id", downtime->Get("legacy_id"));
	} else if (object->GetType() == DynamicType::GetByName("Service")) {
		fields1->Set("downtime_type", 1);
		fields1->Set("internal_downtime_id", downtime->Get("legacy_id"));
	} else {
		Log(LogDebug, "ido", "unknown object type for adding downtime.");
		return;
	}

	fields1->Set("author_name", downtime->Get("author"));
	fields1->Set("triggered_by_id", downtime->Get("triggered_by"));
	fields1->Set("is_fixed", downtime->Get("is_fixed"));
	fields1->Set("duration", downtime->Get("duration"));
	fields1->Set("scheduled_start_time", DbValue::FromTimestamp(downtime->Get("start_time")));
	fields1->Set("scheduled_end_time", DbValue::FromTimestamp(downtime->Get("end_time")));
	fields1->Set("was_started", Empty);
	fields1->Set("actual_start_time", Empty);
	fields1->Set("actual_start_time_usec", Empty);
	fields1->Set("is_in_effect", Empty);
	fields1->Set("trigger_time", DbValue::FromTimestamp(downtime->Get("trigger_time")));
	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	DbQuery query1;
	query1.Table = "scheduleddowntime";
	query1.Type = DbQueryInsert;
	query1.Fields = fields1;
	OnQuery(query1);
}

void ServiceDbObject::RemoveDowntimes(const Service::Ptr& service)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	Log(LogDebug, "ido", "removing service downtimes for '" + service->GetName() + "'");

	DbQuery query1;
	query1.Table = "scheduleddowntime";
	query1.Type = DbQueryDelete;
	query1.WhereCriteria = boost::make_shared<Dictionary>();
	query1.WhereCriteria->Set("object_id", service);
	OnQuery(query1);

	/* delete hostcheck service's host downtimes */
	if (host->GetHostCheckService() == service) {
		DbQuery query2;
		query2.Table = "scheduleddowntime";
		query2.Type = DbQueryDelete;
		query2.WhereCriteria = boost::make_shared<Dictionary>();
		query2.WhereCriteria->Set("object_id", host);
		OnQuery(query2);
	}
}

void ServiceDbObject::RemoveDowntime(const Service::Ptr& service, const Dictionary::Ptr& downtime)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	if (!downtime) {
		Log(LogWarning, "ido", "downtime does not exist. not adding it.");
		return;
	}

	Log(LogDebug, "ido", "removing service downtime (id = " + downtime->Get("legacy_id") + ") for '" + service->GetName() + "'");

	DbQuery query1;
	query1.Table = "scheduleddowntime";
	query1.Type = DbQueryDelete;
	query1.WhereCriteria = boost::make_shared<Dictionary>();
	query1.WhereCriteria->Set("object_id", service);
	query1.WhereCriteria->Set("internal_downtime_id", downtime->Get("legacy_id"));
	OnQuery(query1);

	/* delete hostcheck service's host comments */
	if (host->GetHostCheckService() == service) {
		DbQuery query2;
		query2.Table = "scheduleddowntime";
		query2.Type = DbQueryDelete;
		query2.WhereCriteria = boost::make_shared<Dictionary>();
		query2.WhereCriteria->Set("object_id", host);
		query2.WhereCriteria->Set("internal_downtime_id", downtime->Get("legacy_id"));
		OnQuery(query2);
	}
}

void ServiceDbObject::TriggerDowntime(const Service::Ptr& service, const Dictionary::Ptr& downtime)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	if (!downtime) {
		Log(LogWarning, "ido", "downtime does not exist. not updating it.");
		return;
	}

	Log(LogDebug, "ido", "updating triggered service downtime (id = " + downtime->Get("legacy_id") + ") for '" + service->GetName() + "'");

	double now = Utility::GetTime();
	unsigned long actual_start_time = static_cast<long>(now);
	unsigned long actual_start_time_usec = static_cast<long>((now - actual_start_time) * 1000 * 1000);

	DbQuery query1;
	query1.Table = "scheduleddowntime";
	query1.Type = DbQueryUpdate;

	Dictionary::Ptr fields1 = boost::make_shared<Dictionary>();
	fields1->Set("was_started", 1);
	fields1->Set("actual_start_time", DbValue::FromTimestamp(actual_start_time));
	fields1->Set("actual_start_time_usec", actual_start_time_usec);
	fields1->Set("is_in_effect", 1);
	fields1->Set("trigger_time", DbValue::FromTimestamp(downtime->Get("trigger_time")));
	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.WhereCriteria = boost::make_shared<Dictionary>();
	query1.WhereCriteria->Set("object_id", service);
	query1.WhereCriteria->Set("internal_downtime_id", downtime->Get("legacy_id"));

	query1.Fields = fields1;
	OnQuery(query1);

	/* delete hostcheck service's host comments */
	if (host->GetHostCheckService() == service) {

		DbQuery query2;
		query2.Table = "scheduleddowntime";
		query2.Type = DbQueryUpdate;

		Dictionary::Ptr fields2 = boost::make_shared<Dictionary>();
		fields2->Set("was_started", 1);
		fields2->Set("actual_start_time", DbValue::FromTimestamp(actual_start_time));
		fields2->Set("actual_start_time_usec", actual_start_time_usec);
		fields2->Set("is_in_effect", 1);
		fields2->Set("trigger_time", DbValue::FromTimestamp(downtime->Get("trigger_time")));
		fields2->Set("instance_id", 0); /* DbConnection class fills in real ID */

		query2.WhereCriteria = boost::make_shared<Dictionary>();
		query2.WhereCriteria->Set("object_id", host);
		query2.WhereCriteria->Set("internal_downtime_id", downtime->Get("legacy_id"));

		query2.Fields = fields2;
		OnQuery(query2);
	}
}

void ServiceDbObject::AddAcknowledgement(const Service::Ptr& service, const String& author, const String& comment,
					 AcknowledgementType type, double expiry, const String& authority)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	Log(LogDebug, "ido", "add acknowledgement for '" + service->GetName() + "'");

	double now = Utility::GetTime();
	unsigned long entry_time = static_cast<long>(now);
	unsigned long entry_time_usec = (now - entry_time) * 1000 * 1000;
	unsigned long end_time = static_cast<long>(expiry);

	DbQuery query1;
	query1.Table = "acknowledgements";
	query1.Type = DbQueryInsert;

	Dictionary::Ptr fields1 = boost::make_shared<Dictionary>();
	fields1->Set("entry_time", DbValue::FromTimestamp(entry_time));
	fields1->Set("entry_time_usec", entry_time_usec);
	fields1->Set("acknowledgement_type", type);
	fields1->Set("object_id", service);
	fields1->Set("state", service->GetState());
	fields1->Set("author_name", author);
	fields1->Set("comment_data", comment);
	fields1->Set("is_sticky", type == AcknowledgementSticky ? 1 : 0);
	fields1->Set("end_time", DbValue::FromTimestamp(end_time));
	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.Fields = fields1;
	OnQuery(query1);

	if (host->GetHostCheckService() == service) {

		DbQuery query2;
		query2.Table = "acknowledgements";
		query2.Type = DbQueryInsert;

		Dictionary::Ptr fields2 = boost::make_shared<Dictionary>();
		fields2->Set("entry_time", DbValue::FromTimestamp(entry_time));
		fields2->Set("entry_time_usec", entry_time_usec);
		fields2->Set("acknowledgement_type", type);
		fields2->Set("object_id", host);
		fields2->Set("state", service->GetState());
		fields2->Set("author_name", author);
		fields2->Set("comment_data", comment);
		fields2->Set("is_sticky", type == AcknowledgementSticky ? 1 : 0);
		fields2->Set("end_time", DbValue::FromTimestamp(end_time));
		fields2->Set("instance_id", 0); /* DbConnection class fills in real ID */

		query2.Fields = fields2;
		OnQuery(query2);
	}
}


