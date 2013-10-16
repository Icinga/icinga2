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

#include "db_ido/servicedbobject.h"
#include "db_ido/dbtype.h"
#include "db_ido/dbvalue.h"
#include "base/convert.h"
#include "base/objectlock.h"
#include "base/initialize.h"
#include "base/dynamictype.h"
#include "base/utility.h"
#include "icinga/notification.h"
#include "icinga/checkcommand.h"
#include "icinga/eventcommand.h"
#include "icinga/externalcommandprocessor.h"
#include "icinga/compatutility.h"
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string/join.hpp>

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
	Service::OnCommentAdded.connect(boost::bind(&ServiceDbObject::AddCommentHistory, _1, _2));
	Service::OnDowntimeAdded.connect(boost::bind(&ServiceDbObject::AddDowntimeHistory, _1, _2));
	Service::OnAcknowledgementSet.connect(boost::bind(&ServiceDbObject::AddAcknowledgementHistory, _1, _2, _3, _4, _5));
	Service::OnNotificationSentToUser.connect(bind(&ServiceDbObject::AddContactNotificationHistory, _1, _2));
	Service::OnNotificationSentToAllUsers.connect(bind(&ServiceDbObject::AddNotificationHistory, _1, _2, _3, _4, _5, _6));

	Service::OnStateChange.connect(boost::bind(&ServiceDbObject::AddStateChangeHistory, _1, _2, _3));

	Service::OnNewCheckResult.connect(bind(&ServiceDbObject::AddCheckResultLogHistory, _1, _2));
	Service::OnNotificationSentToUser.connect(bind(&ServiceDbObject::AddNotificationSentLogHistory, _1, _2, _3, _4, _5, _6));
	Service::OnFlappingChanged.connect(bind(&ServiceDbObject::AddFlappingLogHistory, _1, _2));
	Service::OnDowntimeTriggered.connect(boost::bind(&ServiceDbObject::AddTriggerDowntimeLogHistory, _1, _2));
	Service::OnDowntimeRemoved.connect(boost::bind(&ServiceDbObject::AddRemoveDowntimeLogHistory, _1, _2));

	Service::OnFlappingChanged.connect(bind(&ServiceDbObject::AddFlappingHistory, _1, _2));
	Service::OnNewCheckResult.connect(bind(&ServiceDbObject::AddServiceCheckHistory, _1, _2));

	Service::OnEventCommandExecuted.connect(bind(&ServiceDbObject::AddEventHandlerHistory, _1));

	ExternalCommandProcessor::OnNewExternalCommand.connect(boost::bind(&ServiceDbObject::AddExternalCommandHistory, _1, _2, _3));
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
	fields->Set("modified_service_attributes", attrs->Get("modified_attributes"));

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
	Log(LogDebug, "db_ido", "service dependencies for '" + service->GetName() + "'");

	DbQuery query_del1;
	query_del1.Table = GetType()->GetTable() + "dependencies";
	query_del1.Type = DbQueryDelete;
	query_del1.WhereCriteria = boost::make_shared<Dictionary>();
	query_del1.WhereCriteria->Set("dependent_service_object_id", service);
	OnQuery(query_del1);

	BOOST_FOREACH(const Service::Ptr& parent, service->GetParentServices()) {
		Log(LogDebug, "db_ido", "service parents: " + parent->GetName());

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

	/* service contacts, contactgroups */
	Log(LogDebug, "db_ido", "service contacts: " + service->GetName());

	BOOST_FOREACH(const User::Ptr& user, CompatUtility::GetServiceNotificationUsers(service)) {
		Log(LogDebug, "db_ido", "service contacts: " + user->GetName());

		Dictionary::Ptr fields_contact = boost::make_shared<Dictionary>();
		fields_contact->Set("service_id", DbValue::FromObjectInsertID(service));
		fields_contact->Set("contact_object_id", user);
		fields_contact->Set("instance_id", 0); /* DbConnection class fills in real ID */

		DbQuery query_contact;
		query_contact.Table = GetType()->GetTable() + "_contacts";
		query_contact.Type = DbQueryInsert;
		query_contact.Fields = fields_contact;
		OnQuery(query_contact);
	}

	Log(LogDebug, "db_ido", "service contactgroups: " + service->GetName());

	BOOST_FOREACH(const UserGroup::Ptr& usergroup, CompatUtility::GetServiceNotificationUserGroups(service)) {
		Log(LogDebug, "db_ido", "service contactgroups: " + usergroup->GetName());

		Dictionary::Ptr fields_contact = boost::make_shared<Dictionary>();
		fields_contact->Set("service_id", DbValue::FromObjectInsertID(service));
		fields_contact->Set("contactgroup_object_id", usergroup);
		fields_contact->Set("instance_id", 0); /* DbConnection class fills in real ID */

		DbQuery query_contact;
		query_contact.Table = GetType()->GetTable() + "_contactgroups";
		query_contact.Type = DbQueryInsert;
		query_contact.Fields = fields_contact;
		OnQuery(query_contact);
	}

	/* custom variables */
	Log(LogDebug, "db_ido", "service customvars for '" + service->GetName() + "'");

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
			Log(LogDebug, "db_ido", "service customvar key: '" + key + "' value: '" + Convert::ToString(value) + "'");

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

	if (host->GetCheckService() != service)
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

	if (host->GetCheckService() != service)
		return;

	DbObject::Ptr dbobj = GetOrCreateByObject(host);

	if (!dbobj)
		return;

	dbobj->SendStatusUpdate();
}

/* comments */
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
	AddCommentInternal(service, comment, false);
}

void ServiceDbObject::AddCommentHistory(const Service::Ptr& service, const Dictionary::Ptr& comment)
{
	AddCommentInternal(service, comment, true);
}

void ServiceDbObject::AddCommentInternal(const Service::Ptr& service, const Dictionary::Ptr& comment, bool historical)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	if (!comment) {
		Log(LogWarning, "db_ido", "comment does not exist. not adding it.");
		return;
	}

	Log(LogDebug, "db_ido", "adding service comment (id = " + comment->Get("legacy_id") + ") for '" + service->GetName() + "'");

	/* add the service comment */
	AddCommentByType(service, comment, historical);

	/* add the hostcheck service comment to the host as well */
	if (host->GetCheckService() == service) {
		Log(LogDebug, "db_ido", "adding host comment (id = " + comment->Get("legacy_id") + ") for '" + host->GetName() + "'");
		AddCommentByType(host, comment, historical);
	}
}

void ServiceDbObject::AddCommentByType(const DynamicObject::Ptr& object, const Dictionary::Ptr& comment, bool historical)
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
		Log(LogDebug, "db_ido", "unknown object type for adding comment.");
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
	if (!historical) {
		query1.Table = "comments";
	} else {
		query1.Table = "commenthistory";
	}
	query1.Type = DbQueryInsert;
	query1.Fields = fields1;
	OnQuery(query1);
}

void ServiceDbObject::RemoveComments(const Service::Ptr& service)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	Log(LogDebug, "db_ido", "removing service comments for '" + service->GetName() + "'");

	DbQuery query1;
	query1.Table = "comments";
	query1.Type = DbQueryDelete;
	query1.WhereCriteria = boost::make_shared<Dictionary>();
	query1.WhereCriteria->Set("object_id", service);
	OnQuery(query1);

	/* delete hostcheck service's host comments */
	if (host->GetCheckService() == service) {
		query1.WhereCriteria->Set("object_id", host);
		OnQuery(query1);
	}
}

void ServiceDbObject::RemoveComment(const Service::Ptr& service, const Dictionary::Ptr& comment)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	if (!comment) {
		Log(LogWarning, "db_ido", "comment does not exist. not deleting it.");
		return;
	}

	Log(LogDebug, "db_ido", "removing service comment (id = " + comment->Get("legacy_id") + ") for '" + service->GetName() + "'");

	/* Status */
	DbQuery query1;
	query1.Table = "comments";
	query1.Type = DbQueryDelete;
	query1.WhereCriteria = boost::make_shared<Dictionary>();
	query1.WhereCriteria->Set("object_id", service);
	query1.WhereCriteria->Set("internal_comment_id", comment->Get("legacy_id"));
	OnQuery(query1);

	/* delete hostcheck service's host comments */
	if (host->GetCheckService() == service) {
		query1.WhereCriteria->Set("object_id", host);
		OnQuery(query1);
	}

	/* History - update deletion time for service (and host in case) */
	unsigned long entry_time = static_cast<long>(comment->Get("entry_time"));
	double now = Utility::GetTime();
	unsigned long deletion_time = static_cast<long>(now);
	unsigned long deletion_time_usec = (now - deletion_time) * 1000 * 1000;

	DbQuery query2;
	query2.Table = "commenthistory";
	query2.Type = DbQueryUpdate;

	Dictionary::Ptr fields2 = boost::make_shared<Dictionary>();
	fields2->Set("deletion_time", DbValue::FromTimestamp(deletion_time));
	fields2->Set("deletion_time_usec", deletion_time_usec);
	query2.Fields = fields2;

	query2.WhereCriteria = boost::make_shared<Dictionary>();
	query2.WhereCriteria->Set("internal_comment_id", comment->Get("legacy_id"));
	query2.WhereCriteria->Set("comment_time", DbValue::FromTimestamp(entry_time));
	query2.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */

	OnQuery(query2);
}

/* downtimes */
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
	AddDowntimeInternal(service, downtime, false);
}

void ServiceDbObject::AddDowntimeHistory(const Service::Ptr& service, const Dictionary::Ptr& downtime)
{
	AddDowntimeInternal(service, downtime, true);
}

void ServiceDbObject::AddDowntimeInternal(const Service::Ptr& service, const Dictionary::Ptr& downtime, bool historical)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	if (!downtime) {
		Log(LogWarning, "db_ido", "downtime does not exist. not adding it.");
		return;
	}

	Log(LogDebug, "db_ido", "adding service downtime (id = " + downtime->Get("legacy_id") + ") for '" + service->GetName() + "'");

	/* add the service downtime */
	AddDowntimeByType(service, downtime, historical);

	/* add the hostcheck service downtime to the host as well */
	if (host->GetCheckService() == service) {
		Log(LogDebug, "db_ido", "adding host downtime (id = " + downtime->Get("legacy_id") + ") for '" + host->GetName() + "'");
		AddDowntimeByType(host, downtime, historical);
	}
}

void ServiceDbObject::AddDowntimeByType(const DynamicObject::Ptr& object, const Dictionary::Ptr& downtime, bool historical)
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
		Log(LogDebug, "db_ido", "unknown object type for adding downtime.");
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
	if (!historical) {
		query1.Table = "scheduleddowntime";
	} else {
		query1.Table = "downtimehistory";
	}
	query1.Type = DbQueryInsert;
	query1.Fields = fields1;
	OnQuery(query1);
}

void ServiceDbObject::RemoveDowntimes(const Service::Ptr& service)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	Log(LogDebug, "db_ido", "removing service downtimes for '" + service->GetName() + "'");

	DbQuery query1;
	query1.Table = "scheduleddowntime";
	query1.Type = DbQueryDelete;
	query1.WhereCriteria = boost::make_shared<Dictionary>();
	query1.WhereCriteria->Set("object_id", service);
	OnQuery(query1);

	/* delete hostcheck service's host downtimes */
	if (host->GetCheckService() == service) {
		query1.WhereCriteria->Set("object_id", host);
		OnQuery(query1);
	}
}

void ServiceDbObject::RemoveDowntime(const Service::Ptr& service, const Dictionary::Ptr& downtime)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	if (!downtime) {
		Log(LogWarning, "db_ido", "downtime does not exist. not adding it.");
		return;
	}

	Log(LogDebug, "db_ido", "removing service downtime (id = " + downtime->Get("legacy_id") + ") for '" + service->GetName() + "'");

	/* Status */
	DbQuery query1;
	query1.Table = "scheduleddowntime";
	query1.Type = DbQueryDelete;
	query1.WhereCriteria = boost::make_shared<Dictionary>();
	query1.WhereCriteria->Set("object_id", service);
	query1.WhereCriteria->Set("internal_downtime_id", downtime->Get("legacy_id"));
	OnQuery(query1);

	/* delete hostcheck service's host comments */
	if (host->GetCheckService() == service) {
		query1.WhereCriteria->Set("object_id", host);
		OnQuery(query1);
	}

	/* History - update actual_end_time, was_cancelled for service (and host in case) */
	double now = Utility::GetTime();
	unsigned long actual_end_time = static_cast<long>(now);
	unsigned long actual_end_time_usec = (now - actual_end_time) * 1000 * 1000;

	DbQuery query3;
	query3.Table = "downtimehistory";
	query3.Type = DbQueryUpdate;

	Dictionary::Ptr fields3 = boost::make_shared<Dictionary>();
	fields3->Set("was_cancelled", downtime->Get("was_cancelled") ? 1 : 0);
	fields3->Set("actual_end_time", DbValue::FromTimestamp(actual_end_time));
	fields3->Set("actual_end_time_usec", actual_end_time_usec);
	query3.Fields = fields3;

	query3.WhereCriteria = boost::make_shared<Dictionary>();
	query3.WhereCriteria->Set("internal_downtime_id", downtime->Get("legacy_id"));
	query3.WhereCriteria->Set("entry_time", DbValue::FromTimestamp(downtime->Get("entry_time")));
	query3.WhereCriteria->Set("scheduled_start_time", DbValue::FromTimestamp(downtime->Get("start_time")));
	query3.WhereCriteria->Set("scheduled_end_time", DbValue::FromTimestamp(downtime->Get("end_time")));
	query3.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */

	OnQuery(query3);
}

void ServiceDbObject::TriggerDowntime(const Service::Ptr& service, const Dictionary::Ptr& downtime)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	if (!downtime) {
		Log(LogWarning, "db_ido", "downtime does not exist. not updating it.");
		return;
	}

	Log(LogDebug, "db_ido", "updating triggered service downtime (id = " + downtime->Get("legacy_id") + ") for '" + service->GetName() + "'");

	double now = Utility::GetTime();
	unsigned long actual_start_time = static_cast<long>(now);
	unsigned long actual_start_time_usec = static_cast<long>((now - actual_start_time) * 1000 * 1000);

	/* Status */
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
	if (host->GetCheckService() == service) {
		query1.WhereCriteria->Set("object_id", host);
		OnQuery(query1);
	}

	/* History - downtime was started for service (and host in case) */
	DbQuery query3;
	query3.Table = "downtimehistory";
	query3.Type = DbQueryUpdate;

	Dictionary::Ptr fields3 = boost::make_shared<Dictionary>();
	fields3->Set("was_started", 1);
	fields3->Set("is_in_effect", 1);
	fields3->Set("actual_start_time", DbValue::FromTimestamp(actual_start_time));
	fields3->Set("actual_start_time_usec", actual_start_time_usec);
	fields3->Set("trigger_time", DbValue::FromTimestamp(downtime->Get("trigger_time")));
	query3.Fields = fields3;

	query3.WhereCriteria = boost::make_shared<Dictionary>();
	query3.WhereCriteria->Set("internal_downtime_id", downtime->Get("legacy_id"));
	query3.WhereCriteria->Set("entry_time", DbValue::FromTimestamp(downtime->Get("entry_time")));
	query3.WhereCriteria->Set("scheduled_start_time", DbValue::FromTimestamp(downtime->Get("start_time")));
	query3.WhereCriteria->Set("scheduled_end_time", DbValue::FromTimestamp(downtime->Get("end_time")));
	query3.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */

	OnQuery(query3);
}

/* acknowledgements */
void ServiceDbObject::AddAcknowledgementHistory(const Service::Ptr& service, const String& author, const String& comment,
    AcknowledgementType type, double expiry)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	Log(LogDebug, "db_ido", "add acknowledgement history for '" + service->GetName() + "'");

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

	if (host->GetCheckService() == service) {
		fields1->Set("object_id", host);
		fields1->Set("state", host->GetState());
		query1.Fields = fields1;
		OnQuery(query1);
	}
}

/* notifications */

void ServiceDbObject::AddContactNotificationHistory(const Service::Ptr& service, const User::Ptr& user)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	Log(LogDebug, "db_ido", "add contact notification history for '" + service->GetName() + "'");

	/* start and end happen at the same time */
	double now = Utility::GetTime();
	unsigned long start_time = static_cast<long>(now);
	unsigned long end_time = start_time;
	unsigned long start_time_usec = (now - start_time) * 1000 * 1000;
	unsigned long end_time_usec = start_time_usec;

	DbQuery query1;
	query1.Table = "contactnotifications";
	query1.Type = DbQueryInsert;

	Dictionary::Ptr fields1 = boost::make_shared<Dictionary>();
	fields1->Set("contact_object_id", user);
	fields1->Set("start_time", DbValue::FromTimestamp(start_time));
	fields1->Set("start_time_usec", start_time_usec);
	fields1->Set("end_time", DbValue::FromTimestamp(end_time));
	fields1->Set("end_time_usec", end_time_usec);

	fields1->Set("notification_id", 0); /* DbConnection class fills in real ID */
	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.Fields = fields1;
	OnQuery(query1);

	if (host->GetCheckService() == service) {
		query1.Fields = fields1;
		OnQuery(query1);
	}
}

void ServiceDbObject::AddNotificationHistory(const Service::Ptr& service, const std::set<User::Ptr>& users, NotificationType type,
				      const Dictionary::Ptr& cr, const String& author, const String& text)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	Log(LogDebug, "db_ido", "add notification history for '" + service->GetName() + "'");

	/* start and end happen at the same time */
	double now = Utility::GetTime();
	unsigned long start_time = static_cast<long>(now);
	unsigned long end_time = start_time;
	unsigned long start_time_usec = (now - start_time) * 1000 * 1000;
	unsigned long end_time_usec = start_time_usec;

	DbQuery query1;
	query1.Table = "notifications";
	query1.Type = DbQueryInsert;

	Dictionary::Ptr fields1 = boost::make_shared<Dictionary>();
	fields1->Set("notification_type", 1); /* service */
	fields1->Set("notification_reason", CompatUtility::MapNotificationReasonType(type));
	fields1->Set("object_id", service);
	fields1->Set("start_time", DbValue::FromTimestamp(start_time));
	fields1->Set("start_time_usec", start_time_usec);
	fields1->Set("end_time", DbValue::FromTimestamp(end_time));
	fields1->Set("end_time_usec", end_time_usec);
	fields1->Set("state", service->GetState());

	if (cr) {
		Dictionary::Ptr output_bag = CompatUtility::GetCheckResultOutput(cr);
		fields1->Set("output", output_bag->Get("output"));
		fields1->Set("long_output", output_bag->Get("long_output"));
	}

	fields1->Set("escalated", 0);
	fields1->Set("contacts_notified", static_cast<long>(users.size()));
	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.Fields = fields1;
	OnQuery(query1);

	if (host->GetCheckService() == service) {
		fields1->Set("notification_type", 2); /* host */
		fields1->Set("object_id", host);
		fields1->Set("state", host->GetState());
		query1.Fields = fields1;
		OnQuery(query1);
	}
}

/* statehistory */
void ServiceDbObject::AddStateChangeHistory(const Service::Ptr& service, const Dictionary::Ptr& cr, StateType type)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	Log(LogDebug, "db_ido", "add state change history for '" + service->GetName() + "'");

	double now = Utility::GetTime();
	unsigned long state_time = static_cast<long>(now);
	unsigned long state_time_usec = (now - state_time) * 1000 * 1000;

	DbQuery query1;
	query1.Table = "statehistory";
	query1.Type = DbQueryInsert;

	Dictionary::Ptr fields1 = boost::make_shared<Dictionary>();
	fields1->Set("state_time", DbValue::FromTimestamp(state_time));
	fields1->Set("state_time_usec", state_time_usec);
	fields1->Set("object_id", service);
	fields1->Set("state_change", 1); /* service */
	fields1->Set("state", service->GetState());
	fields1->Set("state_type", service->GetStateType());
	fields1->Set("current_check_attempt", service->GetCurrentCheckAttempt());
	fields1->Set("max_check_attempts", service->GetMaxCheckAttempts());
	fields1->Set("last_state", service->GetLastState());
	fields1->Set("last_hard_state", service->GetLastHardState());

	if (cr) {
		Dictionary::Ptr output_bag = CompatUtility::GetCheckResultOutput(cr);
		fields1->Set("output", output_bag->Get("output"));
		fields1->Set("long_output", output_bag->Get("long_output"));
	}

	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.Fields = fields1;
	OnQuery(query1);

	if (host->GetCheckService() == service) {
		fields1->Set("object_id", host);
		fields1->Set("state_change", 0); /* host */
		/* get host states instead */
		fields1->Set("state", host->GetState());
		fields1->Set("state_type", host->GetStateType());
		fields1->Set("last_state", host->GetLastState());
		fields1->Set("last_hard_state", host->GetLastHardState());
		query1.Fields = fields1;
		OnQuery(query1);
	}
}

/* logentries */
void ServiceDbObject::AddCheckResultLogHistory(const Service::Ptr& service, const Dictionary::Ptr &cr)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	Dictionary::Ptr vars_after = cr->Get("vars_after");

	long state_after = vars_after->Get("state");
	long stateType_after = vars_after->Get("state_type");
	long attempt_after = vars_after->Get("attempt");
	bool reachable_after = vars_after->Get("reachable");
	bool host_reachable_after = vars_after->Get("host_reachable");

	Dictionary::Ptr vars_before = cr->Get("vars_before");

	if (vars_before) {
		long state_before = vars_before->Get("state");
		long stateType_before = vars_before->Get("state_type");
		long attempt_before = vars_before->Get("attempt");
		bool reachable_before = vars_before->Get("reachable");

		if (state_before == state_after && stateType_before == stateType_after &&
		    attempt_before == attempt_after && reachable_before == reachable_after)
			return; /* Nothing changed, ignore this checkresult. */
	}

	LogEntryType type;
	switch (service->GetState()) {
		case StateOK:
			type = LogEntryTypeServiceOk;
			break;
		case StateUnknown:
			type = LogEntryTypeServiceUnknown;
			break;
		case StateWarning:
			type = LogEntryTypeServiceWarning;
			break;
		case StateCritical:
			type = LogEntryTypeServiceCritical;
			break;
		default:
			Log(LogCritical, "db_ido", "Unknown service state: " + Convert::ToString(state_after));
			return;
	}

        String output;

        if (cr) {
		Dictionary::Ptr output_bag = CompatUtility::GetCheckResultOutput(cr);
		output = output_bag->Get("output");
        }

	std::ostringstream msgbuf;
	msgbuf << "SERVICE ALERT: "
	       << host->GetName() << ";"
	       << service->GetShortName() << ";"
	       << Service::StateToString(static_cast<ServiceState>(state_after)) << ";"
	       << Service::StateTypeToString(static_cast<StateType>(stateType_after)) << ";"
	       << attempt_after << ";"
	       << output << ""
	       << "";

	AddLogHistory(service, msgbuf.str(), type);

	if (service == host->GetCheckService()) {
		std::ostringstream msgbuf;
		msgbuf << "HOST ALERT: "
		       << host->GetName() << ";"
		       << Host::StateToString(Host::CalculateState(static_cast<ServiceState>(state_after), host_reachable_after)) << ";"
		       << Service::StateTypeToString(static_cast<StateType>(stateType_after)) << ";"
		       << attempt_after << ";"
		       << output << ""
		       << "";

		switch (host->GetState()) {
			case HostUp:
				type = LogEntryTypeHostUp;
				break;
			case HostDown:
				type = LogEntryTypeHostDown;
				break;
			case HostUnreachable:
				type = LogEntryTypeHostUnreachable;
				break;
			default:
				Log(LogCritical, "db_ido", "Unknown host state: " + Convert::ToString(state_after));
				return;
		}

		AddLogHistory(service, msgbuf.str(), type);
	}
}

void ServiceDbObject::AddTriggerDowntimeLogHistory(const Service::Ptr& service, const Dictionary::Ptr& downtime)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	if (!downtime)
		return;

	std::ostringstream msgbuf;
	msgbuf << "SERVICE DOWNTIME ALERT: "
		<< host->GetName() << ";"
		<< service->GetShortName() << ";"
		<< "STARTED" << "; "
		<< "Service has entered a period of scheduled downtime."
		<< "";

	AddLogHistory(service, msgbuf.str(), LogEntryTypeInfoMessage);

	if (service == host->GetCheckService()) {
		std::ostringstream msgbuf;
		msgbuf << "HOST DOWNTIME ALERT: "
			<< host->GetName() << ";"
			<< "STARTED" << "; "
			<< "Service has entered a period of scheduled downtime."
			<< "";

		AddLogHistory(service, msgbuf.str(), LogEntryTypeInfoMessage);
	}
}

void ServiceDbObject::AddRemoveDowntimeLogHistory(const Service::Ptr& service, const Dictionary::Ptr& downtime)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	if (!downtime)
		return;

	String downtime_output;
	String downtime_state_str;

	if (downtime->Get("was_cancelled") == true) {
		downtime_output = "Scheduled downtime for service has been cancelled.";
		downtime_state_str = "CANCELLED";
	} else {
		downtime_output = "Service has exited from a period of scheduled downtime.";
		downtime_state_str = "STOPPED";
	}

	std::ostringstream msgbuf;
	msgbuf << "SERVICE DOWNTIME ALERT: "
		<< host->GetName() << ";"
		<< service->GetShortName() << ";"
		<< downtime_state_str << "; "
		<< downtime_output
		<< "";

	AddLogHistory(service, msgbuf.str(), LogEntryTypeInfoMessage);

	if (service == host->GetCheckService()) {
		std::ostringstream msgbuf;
		msgbuf << "HOST DOWNTIME ALERT: "
			<< host->GetName() << ";"
			<< downtime_state_str << "; "
			<< downtime_output
			<< "";

		AddLogHistory(service, msgbuf.str(), LogEntryTypeInfoMessage);
	}
}

void ServiceDbObject::AddNotificationSentLogHistory(const Service::Ptr& service, const User::Ptr& user,
    NotificationType const& notification_type, Dictionary::Ptr const& cr,
    const String& author, const String& comment_text)
{
        Host::Ptr host = service->GetHost();

        if (!host)
                return;

	CheckCommand::Ptr commandObj = service->GetCheckCommand();

	String check_command = "";
	if (commandObj)
		check_command = commandObj->GetName();

	String notification_type_str = Notification::NotificationTypeToString(notification_type);

	String author_comment = "";
	if (notification_type == NotificationCustom || notification_type == NotificationAcknowledgement) {
		author_comment = ";" + author + ";" + comment_text;
	}

        if (!cr)
                return;

        String output;

        if (cr) {
		Dictionary::Ptr output_bag = CompatUtility::GetCheckResultOutput(cr);
		output = output_bag->Get("output");
        }

        std::ostringstream msgbuf;
        msgbuf << "SERVICE NOTIFICATION: "
		<< user->GetName() << ";"
                << host->GetName() << ";"
                << service->GetShortName() << ";"
                << notification_type_str << " "
		<< "(" << Service::StateToString(service->GetState()) << ");"
		<< check_command << ";"
		<< output << author_comment
                << "";

        AddLogHistory(service, msgbuf.str(), LogEntryTypeServiceNotification);

        if (service == host->GetCheckService()) {
                std::ostringstream msgbuf;
                msgbuf << "HOST NOTIFICATION: "
			<< user->GetName() << ";"
                        << host->GetName() << ";"
			<< notification_type_str << " "
			<< "(" << Service::StateToString(service->GetState()) << ");"
			<< check_command << ";"
			<< output << author_comment
                        << "";

                AddLogHistory(service, msgbuf.str(), LogEntryTypeHostNotification);
        }
}

void ServiceDbObject::AddFlappingLogHistory(const Service::Ptr& service, FlappingState flapping_state)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	String flapping_state_str;
	String flapping_output;

	switch (flapping_state) {
		case FlappingStarted:
			flapping_output = "Service appears to have started flapping (" + Convert::ToString(service->GetFlappingCurrent()) + "% change >= " + Convert::ToString(service->GetFlappingThreshold()) + "% threshold)";
			flapping_state_str = "STARTED";
			break;
		case FlappingStopped:
			flapping_output = "Service appears to have stopped flapping (" + Convert::ToString(service->GetFlappingCurrent()) + "% change < " + Convert::ToString(service->GetFlappingThreshold()) + "% threshold)";
			flapping_state_str = "STOPPED";
			break;
		case FlappingDisabled:
			flapping_output = "Flap detection has been disabled";
			flapping_state_str = "DISABLED";
			break;
		default:
			Log(LogCritical, "db_ido", "Unknown flapping state: " + Convert::ToString(flapping_state));
			return;
	}

        std::ostringstream msgbuf;
        msgbuf << "SERVICE FLAPPING ALERT: "
                << host->GetName() << ";"
                << service->GetShortName() << ";"
                << flapping_state_str << "; "
                << flapping_output
                << "";

	AddLogHistory(service, msgbuf.str(), LogEntryTypeInfoMessage);

        if (service == host->GetCheckService()) {
                std::ostringstream msgbuf;
                msgbuf << "HOST FLAPPING ALERT: "
                        << host->GetName() << ";"
                        << flapping_state_str << "; "
                        << flapping_output
                        << "";

		AddLogHistory(service, msgbuf.str(), LogEntryTypeInfoMessage);
        }
}

void ServiceDbObject::AddLogHistory(const Service::Ptr& service, String buffer, LogEntryType type)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	Log(LogDebug, "db_ido", "add log entry history for '" + service->GetName() + "'");

	double now = Utility::GetTime();
	unsigned long entry_time = static_cast<long>(now);
	unsigned long entry_time_usec = (now - entry_time) * 1000 * 1000;

	DbQuery query1;
	query1.Table = "logentries";
	query1.Type = DbQueryInsert;

	Dictionary::Ptr fields1 = boost::make_shared<Dictionary>();
	fields1->Set("logentry_time", DbValue::FromTimestamp(entry_time));
	fields1->Set("entry_time", DbValue::FromTimestamp(entry_time));
	fields1->Set("entry_time_usec", entry_time_usec);
	fields1->Set("object_id", service); // added in 1.10 see #4754
	fields1->Set("logentry_type", type);
	fields1->Set("logentry_data", buffer);

	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.Fields = fields1;
	OnQuery(query1);

	if (host->GetCheckService() == service) {
		fields1->Set("object_id", host); // added in 1.10 see #4754
		query1.Fields = fields1;
		OnQuery(query1);
	}
}

/* flappinghistory */
void ServiceDbObject::AddFlappingHistory(const Service::Ptr& service, FlappingState flapping_state)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	Log(LogDebug, "db_ido", "add flapping history for '" + service->GetName() + "'");

	double now = Utility::GetTime();
	unsigned long event_time = static_cast<long>(now);
	unsigned long event_time_usec = (now - event_time) * 1000 * 1000;

	DbQuery query1;
	query1.Table = "flappinghistory";
	query1.Type = DbQueryInsert;

	Dictionary::Ptr fields1 = boost::make_shared<Dictionary>();

	fields1->Set("event_time", DbValue::FromTimestamp(event_time));
	fields1->Set("event_time_usec", event_time_usec);

	switch (flapping_state) {
		case FlappingStarted:
			fields1->Set("event_type", 1000);
			break;
		case FlappingStopped:
			fields1->Set("event_type", 1001);
			fields1->Set("reason_type", 1);
			break;
		case FlappingDisabled:
			fields1->Set("event_type", 1001);
			fields1->Set("reason_type", 2);
			break;
		default:
			Log(LogDebug, "db_ido", "Unhandled flapping state: " + Convert::ToString(flapping_state));
			return;
	}

	fields1->Set("flapping_type", 1); /* service */
	fields1->Set("object_id", service);
	fields1->Set("percent_state_change", service->GetFlappingCurrent());
	fields1->Set("low_threshold", service->GetFlappingThreshold());
	fields1->Set("high_threshold", service->GetFlappingThreshold());

	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.Fields = fields1;
	OnQuery(query1);

	if (host->GetCheckService() == service) {
		fields1->Set("object_id", host);
		fields1->Set("flapping_type", 0); /* host */
		query1.Fields = fields1;
		OnQuery(query1);
	}
}

/* servicechecks */
void ServiceDbObject::AddServiceCheckHistory(const Service::Ptr& service, const Dictionary::Ptr &cr)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	Log(LogDebug, "db_ido", "add service check history for '" + service->GetName() + "'");

	DbQuery query1;
	query1.Table = "servicechecks";
	query1.Type = DbQueryInsert;

	Dictionary::Ptr fields1 = boost::make_shared<Dictionary>();
	Dictionary::Ptr attrs;

	{
		ObjectLock olock(service);
		attrs = CompatUtility::GetServiceStatusAttributes(service, CompatTypeService);
	}

	fields1->Set("check_type", attrs->Get("check_type"));
	fields1->Set("current_check_attempt", attrs->Get("current_attempt"));
	fields1->Set("max_check_attempts", attrs->Get("max_attempts"));
	fields1->Set("state", attrs->Get("current_state"));
	fields1->Set("state_type", attrs->Get("state_type"));

	double now = Utility::GetTime();
	unsigned long start_time = static_cast<long>(now);
	unsigned long start_time_usec = (now - start_time) * 1000 * 1000;

	double end = now + attrs->Get("check_execution_time");
	unsigned long end_time = static_cast<long>(end);
	unsigned long end_time_usec = (end - end_time) * 1000 * 1000;

	fields1->Set("start_time", DbValue::FromTimestamp(start_time));
	fields1->Set("start_time_usec", start_time_usec);
	fields1->Set("end_time", DbValue::FromTimestamp(end_time));
	fields1->Set("end_time_usec", end_time_usec);
	fields1->Set("command_object_id", service->GetCheckCommand());
	fields1->Set("command_args", Empty);
	fields1->Set("command_line", cr->Get("command"));
	fields1->Set("execution_time", attrs->Get("check_execution_time"));
	fields1->Set("latency", attrs->Get("check_latency"));
	fields1->Set("return_code", cr->Get("exit_state"));
	fields1->Set("output", attrs->Get("plugin_output"));
	fields1->Set("long_output", attrs->Get("long_plugin_output"));
	fields1->Set("perfdata", attrs->Get("performance_data"));

	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.Fields = fields1;
	OnQuery(query1);

	if (host->GetCheckService() == service) {
		query1.Table = "hostchecks";

		fields1->Remove("service_object_id");
		fields1->Set("host_object_id", host);
		fields1->Set("state", host->GetState());
		fields1->Set("state_type", host->GetStateType());
		query1.Fields = fields1;
		OnQuery(query1);
	}
}

/* eventhandlers */
void ServiceDbObject::AddEventHandlerHistory(const Service::Ptr& service)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	Log(LogDebug, "db_ido", "add eventhandler history for '" + service->GetName() + "'");

	double now = Utility::GetTime();
	unsigned long event_time = static_cast<long>(now);
	unsigned long event_time_usec = (now - event_time) * 1000 * 1000;

	DbQuery query1;
	query1.Table = "eventhandlers";
	query1.Type = DbQueryInsert;

	Dictionary::Ptr fields1 = boost::make_shared<Dictionary>();

	fields1->Set("eventhandler_type", 1); /* service */
	fields1->Set("object_id", service);
	fields1->Set("state", service->GetState());
	fields1->Set("state_type", service->GetStateType());

	fields1->Set("start_time", DbValue::FromTimestamp(event_time));
	fields1->Set("start_time_usec", event_time_usec);
	fields1->Set("end_time", DbValue::FromTimestamp(event_time));
	fields1->Set("end_time_usec", event_time_usec);
	fields1->Set("command_object_id", service->GetEventCommand());

	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.Fields = fields1;
	OnQuery(query1);

	if (host->GetCheckService() == service) {
		fields1->Set("eventhandler_type", 0); /* host */
		fields1->Set("object_id", host);
		fields1->Set("state", host->GetState());
		fields1->Set("state_type", host->GetStateType());
		query1.Fields = fields1;
		OnQuery(query1);
	}
}

/* externalcommands */
void ServiceDbObject::AddExternalCommandHistory(double time, const String& command, const std::vector<String>& arguments)
{
	Log(LogDebug, "db_ido", "add external command history");

	DbQuery query1;
	query1.Table = "externalcommands";
	query1.Type = DbQueryInsert;

	Dictionary::Ptr fields1 = boost::make_shared<Dictionary>();

	fields1->Set("entry_time", DbValue::FromTimestamp(static_cast<long>(time)));
	fields1->Set("command_type", CompatUtility::MapExternalCommandType(command));
	fields1->Set("command_name", command);
	fields1->Set("command_args", boost::algorithm::join(arguments, ";"));

	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.Fields = fields1;
	OnQuery(query1);
}
