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
#include <boost/algorithm/string/join.hpp>

using namespace icinga;

REGISTER_DBTYPE(Service, "service", DbObjectTypeService, "service_object_id", ServiceDbObject);

INITIALIZE_ONCE(&ServiceDbObject::StaticInitialize);

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
	Dictionary::Ptr fields = make_shared<Dictionary>();
	Service::Ptr service = static_pointer_cast<Service>(GetObject());
	Host::Ptr host = service->GetHost();

	fields->Set("host_object_id", host);
	fields->Set("display_name", service->GetDisplayName());
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
	fields->Set("notify_on_warning", CompatUtility::GetServiceNotifyOnWarning(service));
	fields->Set("notify_on_unknown", CompatUtility::GetServiceNotifyOnUnknown(service));
	fields->Set("notify_on_critical", CompatUtility::GetServiceNotifyOnCritical(service));
	fields->Set("notify_on_recovery", CompatUtility::GetServiceNotifyOnRecovery(service));
	fields->Set("notify_on_flapping", CompatUtility::GetServiceNotifyOnFlapping(service));
	fields->Set("notify_on_downtime", CompatUtility::GetServiceNotifyOnDowntime(service));
	fields->Set("stalk_on_ok", 0);
	fields->Set("stalk_on_warning", 0);
	fields->Set("stalk_on_unknown", 0);
	fields->Set("stalk_on_critical", 0);
	fields->Set("is_volatile", CompatUtility::GetServiceIsVolatile(service));
	fields->Set("flap_detection_enabled", CompatUtility::GetServiceFlapDetectionEnabled(service));
	fields->Set("flap_detection_on_ok", Empty);
	fields->Set("flap_detection_on_warning", Empty);
	fields->Set("flap_detection_on_unknown", Empty);
	fields->Set("flap_detection_on_critical", Empty);
	fields->Set("low_flap_threshold", CompatUtility::GetServiceLowFlapThreshold(service));
	fields->Set("high_flap_threshold", CompatUtility::GetServiceHighFlapThreshold(service));
	fields->Set("process_performance_data", CompatUtility::GetServiceProcessPerformanceData(service));
	fields->Set("freshness_checks_enabled", CompatUtility::GetServiceFreshnessChecksEnabled(service));
	fields->Set("freshness_threshold", CompatUtility::GetServiceFreshnessThreshold(service));
	fields->Set("passive_checks_enabled", CompatUtility::GetServicePassiveChecksEnabled(service));
	fields->Set("event_handler_enabled", CompatUtility::GetServiceEventHandlerEnabled(service));
	fields->Set("active_checks_enabled", CompatUtility::GetServiceActiveChecksEnabled(service));
	fields->Set("retain_status_information", Empty);
	fields->Set("retain_nonstatus_information", Empty);
	fields->Set("notifications_enabled", CompatUtility::GetServiceNotificationsEnabled(service));
	fields->Set("obsess_over_service", Empty);
	fields->Set("failure_prediction_enabled", Empty);
	fields->Set("notes", CompatUtility::GetCustomAttributeConfig(service, "notes"));
	fields->Set("notes_url", CompatUtility::GetCustomAttributeConfig(service, "notes_url"));
	fields->Set("action_url", CompatUtility::GetCustomAttributeConfig(service, "action_url"));
	fields->Set("icon_image", CompatUtility::GetCustomAttributeConfig(service, "icon_image"));
	fields->Set("icon_image_alt", CompatUtility::GetCustomAttributeConfig(service, "icon_image_alt"));

	return fields;
}

Dictionary::Ptr ServiceDbObject::GetStatusFields(void) const
{
	Dictionary::Ptr fields = make_shared<Dictionary>();
	Service::Ptr service = static_pointer_cast<Service>(GetObject());
	CheckResult::Ptr cr = service->GetLastCheckResult();

	if (cr) {
		fields->Set("output", CompatUtility::GetCheckResultOutput(cr));
		fields->Set("long_output", CompatUtility::GetCheckResultLongOutput(cr));
		fields->Set("perfdata", CompatUtility::GetCheckResultPerfdata(cr));
		fields->Set("check_source", cr->GetCheckSource());
	}

	fields->Set("current_state", CompatUtility::GetServiceCurrentState(service));
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
	fields->Set("last_time_ok", DbValue::FromTimestamp(static_cast<int>(service->GetLastStateOK())));
	fields->Set("last_time_warning", DbValue::FromTimestamp(static_cast<int>(service->GetLastStateWarning())));
	fields->Set("last_time_critical", DbValue::FromTimestamp(static_cast<int>(service->GetLastStateCritical())));
	fields->Set("last_time_unknown", DbValue::FromTimestamp(static_cast<int>(service->GetLastStateUnknown())));
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
	fields->Set("process_performance_data", CompatUtility::GetServiceProcessPerformanceData(service));
	fields->Set("event_handler", CompatUtility::GetServiceEventHandler(service));
	fields->Set("check_command", CompatUtility::GetServiceCheckCommand(service));
	fields->Set("normal_check_interval", CompatUtility::GetServiceCheckInterval(service));
	fields->Set("retry_check_interval", CompatUtility::GetServiceRetryInterval(service));
	fields->Set("check_timeperiod_object_id", service->GetCheckPeriod());
	fields->Set("modified_service_attributes", service->GetModifiedAttributes());

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

	BOOST_FOREACH(const Service::Ptr& parent, service->GetParentServices()) {
		Log(LogDebug, "db_ido", "service parents: " + parent->GetName());

                /* service dependencies */
                Dictionary::Ptr fields1 = make_shared<Dictionary>();
                fields1->Set("service_object_id", parent);
                fields1->Set("dependent_service_object_id", service);
                fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

                DbQuery query1;
                query1.Table = GetType()->GetTable() + "dependencies";
                query1.Type = DbQueryInsert;
		query1.Category = DbCatConfig;
                query1.Fields = fields1;
                OnQuery(query1);
	}

	/* service contacts, contactgroups */
	Log(LogDebug, "db_ido", "service contacts: " + service->GetName());

	BOOST_FOREACH(const User::Ptr& user, CompatUtility::GetServiceNotificationUsers(service)) {
		Log(LogDebug, "db_ido", "service contacts: " + user->GetName());

		Dictionary::Ptr fields_contact = make_shared<Dictionary>();
		fields_contact->Set("service_id", DbValue::FromObjectInsertID(service));
		fields_contact->Set("contact_object_id", user);
		fields_contact->Set("instance_id", 0); /* DbConnection class fills in real ID */

		DbQuery query_contact;
		query_contact.Table = GetType()->GetTable() + "_contacts";
		query_contact.Type = DbQueryInsert;
		query_contact.Category = DbCatConfig;
		query_contact.Fields = fields_contact;
		OnQuery(query_contact);
	}

	Log(LogDebug, "db_ido", "service contactgroups: " + service->GetName());

	BOOST_FOREACH(const UserGroup::Ptr& usergroup, CompatUtility::GetServiceNotificationUserGroups(service)) {
		Log(LogDebug, "db_ido", "service contactgroups: " + usergroup->GetName());

		Dictionary::Ptr fields_contact = make_shared<Dictionary>();
		fields_contact->Set("service_id", DbValue::FromObjectInsertID(service));
		fields_contact->Set("contactgroup_object_id", usergroup);
		fields_contact->Set("instance_id", 0); /* DbConnection class fills in real ID */

		DbQuery query_contact;
		query_contact.Table = GetType()->GetTable() + "_contactgroups";
		query_contact.Type = DbQueryInsert;
		query_contact.Category = DbCatConfig;
		query_contact.Fields = fields_contact;
		OnQuery(query_contact);
	}

	/* custom variables */
	Log(LogDebug, "db_ido", "service customvars for '" + service->GetName() + "'");

	Dictionary::Ptr customvars;

	{
		ObjectLock olock(service);
		customvars = CompatUtility::GetCustomVariableConfig(service);
	}

	if (customvars) {
		ObjectLock olock(customvars);

		BOOST_FOREACH(const Dictionary::Pair& kv, customvars) {
			Log(LogDebug, "db_ido", "service customvar key: '" + kv.first + "' value: '" + Convert::ToString(kv.second) + "'");

			Dictionary::Ptr fields2 = make_shared<Dictionary>();
			fields2->Set("varname", Convert::ToString(kv.first));
			fields2->Set("varvalue", Convert::ToString(kv.second));
			fields2->Set("config_type", 1);
			fields2->Set("has_been_modified", 0);
			fields2->Set("object_id", service);
			fields2->Set("instance_id", 0); /* DbConnection class fills in real ID */

			DbQuery query2;
			query2.Table = "customvariables";
			query2.Type = DbQueryInsert;
			query2.Category = DbCatConfig;
			query2.Fields = fields2;
			OnQuery(query2);
		}
	}

	/* update comments and downtimes on config change */
	AddComments(service);
	AddDowntimes(service);
}

void ServiceDbObject::OnStatusUpdate(void)
{
	Service::Ptr service = static_pointer_cast<Service>(GetObject());
	Host::Ptr host = service->GetHost();

	/* update the host if hostcheck service */
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

	if (comments->GetLength() > 0)
		RemoveComments(service);

	ObjectLock olock(comments);

	BOOST_FOREACH(const Dictionary::Pair& kv, comments) {
		AddComment(service, kv.second);
	}
}

void ServiceDbObject::AddComment(const Service::Ptr& service, const Comment::Ptr& comment)
{
	AddCommentInternal(service, comment, false);
}

void ServiceDbObject::AddCommentHistory(const Service::Ptr& service, const Comment::Ptr& comment)
{
	AddCommentInternal(service, comment, true);
}

void ServiceDbObject::AddCommentInternal(const Service::Ptr& service, const Comment::Ptr& comment, bool historical)
{
	Host::Ptr host = service->GetHost();

	if (!comment) {
		Log(LogWarning, "db_ido", "comment does not exist. not adding it.");
		return;
	}

	Log(LogDebug, "db_ido", "adding service comment (id = " + Convert::ToString(comment->GetLegacyId()) + ") for '" + service->GetName() + "'");

	/* add the service comment */
	AddCommentByType(service, comment, historical);

	/* add the hostcheck service comment to the host as well */
	if (host->GetCheckService() == service) {
		Log(LogDebug, "db_ido", "adding host comment (id = " + Convert::ToString(comment->GetLegacyId()) + ") for '" + host->GetName() + "'");
		AddCommentByType(host, comment, historical);
	}
}

void ServiceDbObject::AddCommentByType(const DynamicObject::Ptr& object, const Comment::Ptr& comment, bool historical)
{
	unsigned long entry_time = static_cast<long>(comment->GetEntryTime());
	unsigned long entry_time_usec = (comment->GetEntryTime() - entry_time) * 1000 * 1000;

	Dictionary::Ptr fields1 = make_shared<Dictionary>();
	fields1->Set("entry_time", DbValue::FromTimestamp(entry_time));
	fields1->Set("entry_time_usec", entry_time_usec);
	fields1->Set("entry_type", comment->GetEntryType());
	fields1->Set("object_id", object);

	if (object->GetType() == DynamicType::GetByName("Host")) {
		fields1->Set("comment_type", 2);
		/* requires idoutils 1.10 schema fix */
		fields1->Set("internal_comment_id", comment->GetLegacyId());
	} else if (object->GetType() == DynamicType::GetByName("Service")) {
		fields1->Set("comment_type", 1);
		fields1->Set("internal_comment_id", comment->GetLegacyId());
	} else {
		Log(LogDebug, "db_ido", "unknown object type for adding comment.");
		return;
	}

	fields1->Set("comment_time", DbValue::FromTimestamp(entry_time)); /* same as entry_time */
	fields1->Set("author_name", comment->GetAuthor());
	fields1->Set("comment_data", comment->GetText());
	fields1->Set("is_persistent", 1);
	fields1->Set("comment_source", 1); /* external */
	fields1->Set("expires", (comment->GetExpireTime() > 0) ? 1 : 0);
	fields1->Set("expiration_time", comment->GetExpireTime());
	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	DbQuery query1;
	if (!historical) {
		query1.Table = "comments";
	} else {
		query1.Table = "commenthistory";
	}
	query1.Type = DbQueryInsert;
	query1.Category = DbCatComment;
	query1.Fields = fields1;
	OnQuery(query1);
}

void ServiceDbObject::RemoveComments(const Service::Ptr& service)
{
	Host::Ptr host = service->GetHost();

	Log(LogDebug, "db_ido", "removing service comments for '" + service->GetName() + "'");

	DbQuery query1;
	query1.Table = "comments";
	query1.Type = DbQueryDelete;
	query1.Category = DbCatComment;
	query1.WhereCriteria = make_shared<Dictionary>();
	query1.WhereCriteria->Set("object_id", service);
	OnQuery(query1);

	/* delete hostcheck service's host comments */
	if (host->GetCheckService() == service) {
		query1.WhereCriteria = query1.WhereCriteria->ShallowClone();
		query1.WhereCriteria->Set("object_id", host);
		OnQuery(query1);
	}
}

void ServiceDbObject::RemoveComment(const Service::Ptr& service, const Comment::Ptr& comment)
{
	Host::Ptr host = service->GetHost();

	if (!comment) {
		Log(LogWarning, "db_ido", "comment does not exist. not deleting it.");
		return;
	}

	Log(LogDebug, "db_ido", "removing service comment (id = " + Convert::ToString(comment->GetLegacyId()) + ") for '" + service->GetName() + "'");

	/* Status */
	DbQuery query1;
	query1.Table = "comments";
	query1.Type = DbQueryDelete;
	query1.Category = DbCatComment;
	query1.WhereCriteria = make_shared<Dictionary>();
	query1.WhereCriteria->Set("object_id", service);
	query1.WhereCriteria->Set("internal_comment_id", comment->GetLegacyId());
	OnQuery(query1);

	/* delete hostcheck service's host comments */
	if (host->GetCheckService() == service) {
		query1.WhereCriteria = query1.WhereCriteria->ShallowClone();
		query1.WhereCriteria->Set("object_id", host);
		OnQuery(query1);
	}

	/* History - update deletion time for service (and host in case) */
	unsigned long entry_time = static_cast<long>(comment->GetEntryTime());

	double now = Utility::GetTime();
	std::pair<unsigned long, unsigned long> time_bag = CompatUtility::ConvertTimestamp(now);

	DbQuery query2;
	query2.Table = "commenthistory";
	query2.Type = DbQueryUpdate;
	query2.Category = DbCatComment;

	Dictionary::Ptr fields2 = make_shared<Dictionary>();
	fields2->Set("deletion_time", DbValue::FromTimestamp(time_bag.first));
	fields2->Set("deletion_time_usec", time_bag.second);
	query2.Fields = fields2;

	query2.WhereCriteria = make_shared<Dictionary>();
	query2.WhereCriteria->Set("internal_comment_id", comment->GetLegacyId());
	query2.WhereCriteria->Set("comment_time", DbValue::FromTimestamp(entry_time));
	query2.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */

	OnQuery(query2);
}

/* downtimes */
void ServiceDbObject::AddDowntimes(const Service::Ptr& service)
{
	/* dump all downtimes */
	Dictionary::Ptr downtimes = service->GetDowntimes();

	if (downtimes->GetLength() > 0)
		RemoveDowntimes(service);

	ObjectLock olock(downtimes);

	BOOST_FOREACH(const Dictionary::Pair& kv, downtimes) {
		AddDowntime(service, kv.second);
	}
}

void ServiceDbObject::AddDowntime(const Service::Ptr& service, const Downtime::Ptr& downtime)
{
	AddDowntimeInternal(service, downtime, false);
}

void ServiceDbObject::AddDowntimeHistory(const Service::Ptr& service, const Downtime::Ptr& downtime)
{
	AddDowntimeInternal(service, downtime, true);
}

void ServiceDbObject::AddDowntimeInternal(const Service::Ptr& service, const Downtime::Ptr& downtime, bool historical)
{
	Host::Ptr host = service->GetHost();

	if (!downtime) {
		Log(LogWarning, "db_ido", "downtime does not exist. not adding it.");
		return;
	}

	Log(LogDebug, "db_ido", "adding service downtime (id = " + Convert::ToString(downtime->GetLegacyId()) + ") for '" + service->GetName() + "'");

	/* add the service downtime */
	AddDowntimeByType(service, downtime, historical);

	/* add the hostcheck service downtime to the host as well */
	if (host->GetCheckService() == service) {
		Log(LogDebug, "db_ido", "adding host downtime (id = " + Convert::ToString(downtime->GetLegacyId()) + ") for '" + host->GetName() + "'");
		AddDowntimeByType(host, downtime, historical);
	}
}

void ServiceDbObject::AddDowntimeByType(const DynamicObject::Ptr& object, const Downtime::Ptr& downtime, bool historical)
{
	Dictionary::Ptr fields1 = make_shared<Dictionary>();
	fields1->Set("entry_time", DbValue::FromTimestamp(downtime->GetEntryTime()));
	fields1->Set("object_id", object);

	if (object->GetType() == DynamicType::GetByName("Host")) {
		fields1->Set("downtime_type", 2);
		/* requires idoutils 1.10 schema fix */
		fields1->Set("internal_downtime_id", downtime->GetLegacyId());
	} else if (object->GetType() == DynamicType::GetByName("Service")) {
		fields1->Set("downtime_type", 1);
		fields1->Set("internal_downtime_id", downtime->GetLegacyId());
	} else {
		Log(LogDebug, "db_ido", "unknown object type for adding downtime.");
		return;
	}

	fields1->Set("author_name", downtime->GetAuthor());
	fields1->Set("comment_data", downtime->GetComment());
	fields1->Set("triggered_by_id", Service::GetDowntimeByID(downtime->GetTriggeredBy()));
	fields1->Set("is_fixed", downtime->GetFixed());
	fields1->Set("duration", downtime->GetDuration());
	fields1->Set("scheduled_start_time", DbValue::FromTimestamp(downtime->GetStartTime()));
	fields1->Set("scheduled_end_time", DbValue::FromTimestamp(downtime->GetEndTime()));
	fields1->Set("was_started", Empty);
	fields1->Set("actual_start_time", Empty);
	fields1->Set("actual_start_time_usec", Empty);
	fields1->Set("is_in_effect", Empty);
	fields1->Set("trigger_time", DbValue::FromTimestamp(downtime->GetTriggerTime()));
	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	DbQuery query1;
	if (!historical) {
		query1.Table = "scheduleddowntime";
	} else {
		query1.Table = "downtimehistory";
	}
	query1.Type = DbQueryInsert;
	query1.Category = DbCatDowntime;
	query1.Fields = fields1;
	OnQuery(query1);
}

void ServiceDbObject::RemoveDowntimes(const Service::Ptr& service)
{
	Host::Ptr host = service->GetHost();

	Log(LogDebug, "db_ido", "removing service downtimes for '" + service->GetName() + "'");

	DbQuery query1;
	query1.Table = "scheduleddowntime";
	query1.Type = DbQueryDelete;
	query1.Category = DbCatDowntime;
	query1.WhereCriteria = make_shared<Dictionary>();
	query1.WhereCriteria->Set("object_id", service);
	OnQuery(query1);

	/* delete hostcheck service's host downtimes */
	if (host->GetCheckService() == service) {
		query1.WhereCriteria = query1.WhereCriteria->ShallowClone();
		query1.WhereCriteria->Set("object_id", host);
		OnQuery(query1);
	}
}

void ServiceDbObject::RemoveDowntime(const Service::Ptr& service, const Downtime::Ptr& downtime)
{
	Host::Ptr host = service->GetHost();

	if (!downtime) {
		Log(LogWarning, "db_ido", "downtime does not exist. not adding it.");
		return;
	}

	Log(LogDebug, "db_ido", "removing service downtime (id = " + Convert::ToString(downtime->GetLegacyId()) + ") for '" + service->GetName() + "'");

	/* Status */
	DbQuery query1;
	query1.Table = "scheduleddowntime";
	query1.Type = DbQueryDelete;
	query1.Category = DbCatDowntime;
	query1.WhereCriteria = make_shared<Dictionary>();
	query1.WhereCriteria->Set("object_id", service);
	query1.WhereCriteria->Set("internal_downtime_id", downtime->GetLegacyId());
	OnQuery(query1);

	/* delete hostcheck service's host comments */
	if (host->GetCheckService() == service) {
		query1.WhereCriteria = query1.WhereCriteria->ShallowClone();
		query1.WhereCriteria->Set("object_id", host);
		OnQuery(query1);
	}

	/* History - update actual_end_time, was_cancelled for service (and host in case) */
	double now = Utility::GetTime();
	std::pair<unsigned long, unsigned long> time_bag = CompatUtility::ConvertTimestamp(now);

	DbQuery query3;
	query3.Table = "downtimehistory";
	query3.Type = DbQueryUpdate;
	query3.Category = DbCatDowntime;

	Dictionary::Ptr fields3 = make_shared<Dictionary>();
	fields3->Set("was_cancelled", downtime->GetWasCancelled() ? 1 : 0);
	fields3->Set("actual_end_time", DbValue::FromTimestamp(time_bag.first));
	fields3->Set("actual_end_time_usec", time_bag.second);
	query3.Fields = fields3;

	query3.WhereCriteria = make_shared<Dictionary>();
	query3.WhereCriteria->Set("internal_downtime_id", downtime->GetLegacyId());
	query3.WhereCriteria->Set("entry_time", DbValue::FromTimestamp(downtime->GetEntryTime()));
	query3.WhereCriteria->Set("scheduled_start_time", DbValue::FromTimestamp(downtime->GetStartTime()));
	query3.WhereCriteria->Set("scheduled_end_time", DbValue::FromTimestamp(downtime->GetEndTime()));
	query3.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */

	OnQuery(query3);
}

void ServiceDbObject::TriggerDowntime(const Service::Ptr& service, const Downtime::Ptr& downtime)
{
	Host::Ptr host = service->GetHost();

	if (!downtime) {
		Log(LogWarning, "db_ido", "downtime does not exist. not updating it.");
		return;
	}

	Log(LogDebug, "db_ido", "updating triggered service downtime (id = " + Convert::ToString(downtime->GetLegacyId()) + ") for '" + service->GetName() + "'");

	double now = Utility::GetTime();
	std::pair<unsigned long, unsigned long> time_bag = CompatUtility::ConvertTimestamp(now);

	/* Status */
	DbQuery query1;
	query1.Table = "scheduleddowntime";
	query1.Type = DbQueryUpdate;
	query1.Category = DbCatDowntime;

	Dictionary::Ptr fields1 = make_shared<Dictionary>();
	fields1->Set("was_started", 1);
	fields1->Set("actual_start_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("actual_start_time_usec", time_bag.second);
	fields1->Set("is_in_effect", 1);
	fields1->Set("trigger_time", DbValue::FromTimestamp(downtime->GetTriggerTime()));
	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.WhereCriteria = make_shared<Dictionary>();
	query1.WhereCriteria->Set("object_id", service);
	query1.WhereCriteria->Set("internal_downtime_id", downtime->GetLegacyId());

	query1.Fields = fields1;
	OnQuery(query1);

	/* delete hostcheck service's host comments */
	if (host->GetCheckService() == service) {
		query1.WhereCriteria = query1.WhereCriteria->ShallowClone();
		query1.WhereCriteria->Set("object_id", host);
		OnQuery(query1);
	}

	/* History - downtime was started for service (and host in case) */
	DbQuery query3;
	query3.Table = "downtimehistory";
	query3.Type = DbQueryUpdate;
	query3.Category = DbCatDowntime;

	Dictionary::Ptr fields3 = make_shared<Dictionary>();
	fields3->Set("was_started", 1);
	fields3->Set("is_in_effect", 1);
	fields3->Set("actual_start_time", DbValue::FromTimestamp(time_bag.first));
	fields3->Set("actual_start_time_usec", time_bag.second);
	fields3->Set("trigger_time", DbValue::FromTimestamp(downtime->GetTriggerTime()));
	query3.Fields = fields3;

	query3.WhereCriteria = make_shared<Dictionary>();
	query3.WhereCriteria->Set("internal_downtime_id", downtime->GetLegacyId());
	query3.WhereCriteria->Set("entry_time", DbValue::FromTimestamp(downtime->GetEntryTime()));
	query3.WhereCriteria->Set("scheduled_start_time", DbValue::FromTimestamp(downtime->GetStartTime()));
	query3.WhereCriteria->Set("scheduled_end_time", DbValue::FromTimestamp(downtime->GetEndTime()));
	query3.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */

	OnQuery(query3);
}

/* acknowledgements */
void ServiceDbObject::AddAcknowledgementHistory(const Service::Ptr& service, const String& author, const String& comment,
    AcknowledgementType type, double expiry)
{
	Host::Ptr host = service->GetHost();

	Log(LogDebug, "db_ido", "add acknowledgement history for '" + service->GetName() + "'");

	double now = Utility::GetTime();
	std::pair<unsigned long, unsigned long> time_bag = CompatUtility::ConvertTimestamp(now);

	unsigned long end_time = static_cast<long>(expiry);

	DbQuery query1;
	query1.Table = "acknowledgements";
	query1.Type = DbQueryInsert;
	query1.Category = DbCatAcknowledgement;

	Dictionary::Ptr fields1 = make_shared<Dictionary>();
	fields1->Set("entry_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("entry_time_usec", time_bag.second);
	fields1->Set("acknowledgement_type", type);
	fields1->Set("object_id", service);
	fields1->Set("state", CompatUtility::GetServiceCurrentState(service));
	fields1->Set("author_name", author);
	fields1->Set("comment_data", comment);
	fields1->Set("is_sticky", type == AcknowledgementSticky ? 1 : 0);
	fields1->Set("end_time", DbValue::FromTimestamp(end_time));
	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.Fields = fields1;
	OnQuery(query1);

	if (host->GetCheckService() == service) {
		fields1 = fields1->ShallowClone();
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

	Log(LogDebug, "db_ido", "add contact notification history for '" + service->GetName() + "'");

	/* start and end happen at the same time */
	double now = Utility::GetTime();
	std::pair<unsigned long, unsigned long> time_bag = CompatUtility::ConvertTimestamp(now);

	DbQuery query1;
	query1.Table = "contactnotifications";
	query1.Type = DbQueryInsert;
	query1.Category = DbCatNotification;

	Dictionary::Ptr fields1 = make_shared<Dictionary>();
	fields1->Set("contact_object_id", user);
	fields1->Set("start_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("start_time_usec", time_bag.second);
	fields1->Set("end_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("end_time_usec", time_bag.second);

	fields1->Set("notification_id", 0); /* DbConnection class fills in real ID */
	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.Fields = fields1;
	OnQuery(query1);
}

void ServiceDbObject::AddNotificationHistory(const Service::Ptr& service, const std::set<User::Ptr>& users, NotificationType type,
    const CheckResult::Ptr& cr, const String& author, const String& text)
{
	Host::Ptr host = service->GetHost();

	Log(LogDebug, "db_ido", "add notification history for '" + service->GetName() + "'");

	/* start and end happen at the same time */
	double now = Utility::GetTime();
	std::pair<unsigned long, unsigned long> time_bag = CompatUtility::ConvertTimestamp(now);

	DbQuery query1;
	query1.Table = "notifications";
	query1.Type = DbQueryInsert;
	query1.Category = DbCatNotification;

	Dictionary::Ptr fields1 = make_shared<Dictionary>();
	fields1->Set("notification_type", 1); /* service */
	fields1->Set("notification_reason", CompatUtility::MapNotificationReasonType(type));
	fields1->Set("object_id", service);
	fields1->Set("start_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("start_time_usec", time_bag.second);
	fields1->Set("end_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("end_time_usec", time_bag.second);
	fields1->Set("state", CompatUtility::GetServiceCurrentState(service));

	if (cr) {
		fields1->Set("output", CompatUtility::GetCheckResultOutput(cr));
		fields1->Set("long_output", CompatUtility::GetCheckResultLongOutput(cr));
	}

	fields1->Set("escalated", 0);
	fields1->Set("contacts_notified", static_cast<long>(users.size()));
	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.Fields = fields1;
	OnQuery(query1);

	if (host->GetCheckService() == service) {
		fields1 = fields1->ShallowClone();
		fields1->Set("notification_type", 2); /* host */
		fields1->Set("object_id", host);
		fields1->Set("state", host->GetState());
		query1.Fields = fields1;
		OnQuery(query1);
	}
}

/* statehistory */
void ServiceDbObject::AddStateChangeHistory(const Service::Ptr& service, const CheckResult::Ptr& cr, StateType type)
{
	Host::Ptr host = service->GetHost();

	Log(LogDebug, "db_ido", "add state change history for '" + service->GetName() + "'");

	double now = Utility::GetTime();
	std::pair<unsigned long, unsigned long> time_bag = CompatUtility::ConvertTimestamp(now);

	DbQuery query1;
	query1.Table = "statehistory";
	query1.Type = DbQueryInsert;
	query1.Category = DbCatStateHistory;

	Dictionary::Ptr fields1 = make_shared<Dictionary>();
	fields1->Set("state_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("state_time_usec", time_bag.second);
	fields1->Set("object_id", service);
	fields1->Set("state_change", 1); /* service */
	fields1->Set("state", CompatUtility::GetServiceCurrentState(service));
	fields1->Set("state_type", service->GetStateType());
	fields1->Set("current_check_attempt", service->GetCheckAttempt());
	fields1->Set("max_check_attempts", service->GetMaxCheckAttempts());
	fields1->Set("last_state", service->GetLastState());
	fields1->Set("last_hard_state", service->GetLastHardState());

	if (cr) {
		fields1->Set("output", CompatUtility::GetCheckResultOutput(cr));
		fields1->Set("long_output", CompatUtility::GetCheckResultLongOutput(cr));
	}

	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.Fields = fields1;
	OnQuery(query1);

	if (host->GetCheckService() == service) {
		fields1 = fields1->ShallowClone();
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
void ServiceDbObject::AddCheckResultLogHistory(const Service::Ptr& service, const CheckResult::Ptr &cr)
{
	Host::Ptr host = service->GetHost();

	Dictionary::Ptr vars_after = cr->GetVarsAfter();

	long state_after = vars_after->Get("state");
	long stateType_after = vars_after->Get("state_type");
	long attempt_after = vars_after->Get("attempt");
	bool reachable_after = vars_after->Get("reachable");
	bool host_reachable_after = vars_after->Get("host_reachable");

	Dictionary::Ptr vars_before = cr->GetVarsBefore();

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

	if (cr)
		output = CompatUtility::GetCheckResultOutput(cr);

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

void ServiceDbObject::AddTriggerDowntimeLogHistory(const Service::Ptr& service, const Downtime::Ptr& downtime)
{
	Host::Ptr host = service->GetHost();

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

void ServiceDbObject::AddRemoveDowntimeLogHistory(const Service::Ptr& service, const Downtime::Ptr& downtime)
{
	Host::Ptr host = service->GetHost();

	if (!downtime)
		return;

	String downtime_output;
	String downtime_state_str;

	if (downtime->GetWasCancelled()) {
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
    NotificationType notification_type, const CheckResult::Ptr& cr,
    const String& author, const String& comment_text)
{
        Host::Ptr host = service->GetHost();

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

	if (cr)
		output = CompatUtility::GetCheckResultOutput(cr);

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

	Log(LogDebug, "db_ido", "add log entry history for '" + service->GetName() + "'");

	double now = Utility::GetTime();
	std::pair<unsigned long, unsigned long> time_bag = CompatUtility::ConvertTimestamp(now);

	DbQuery query1;
	query1.Table = "logentries";
	query1.Type = DbQueryInsert;
	query1.Category = DbCatLog;

	Dictionary::Ptr fields1 = make_shared<Dictionary>();
	fields1->Set("logentry_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("entry_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("entry_time_usec", time_bag.second);
	fields1->Set("object_id", service); // added in 1.10 see #4754
	fields1->Set("logentry_type", type);
	fields1->Set("logentry_data", buffer);

	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.Fields = fields1;
	OnQuery(query1);

	if (host->GetCheckService() == service) {
		fields1 = fields1->ShallowClone();
		fields1->Set("object_id", host); // added in 1.10 see #4754
		query1.Fields = fields1;
		OnQuery(query1);
	}
}

/* flappinghistory */
void ServiceDbObject::AddFlappingHistory(const Service::Ptr& service, FlappingState flapping_state)
{
	Host::Ptr host = service->GetHost();

	Log(LogDebug, "db_ido", "add flapping history for '" + service->GetName() + "'");

	double now = Utility::GetTime();
	std::pair<unsigned long, unsigned long> time_bag = CompatUtility::ConvertTimestamp(now);

	DbQuery query1;
	query1.Table = "flappinghistory";
	query1.Type = DbQueryInsert;
	query1.Category = DbCatFlapping;

	Dictionary::Ptr fields1 = make_shared<Dictionary>();

	fields1->Set("event_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("event_time_usec", time_bag.second);

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
		fields1 = fields1->ShallowClone();
		fields1->Set("object_id", host);
		fields1->Set("flapping_type", 0); /* host */
		query1.Fields = fields1;
		OnQuery(query1);
	}
}

/* servicechecks */
void ServiceDbObject::AddServiceCheckHistory(const Service::Ptr& service, const CheckResult::Ptr &cr)
{
	if (!cr)
		return;

	Host::Ptr host = service->GetHost();

	Log(LogDebug, "db_ido", "add service check history for '" + service->GetName() + "'");

	DbQuery query1;
	query1.Table = "servicechecks";
	query1.Type = DbQueryInsert;
	query1.Category = DbCatCheck;

	Dictionary::Ptr fields1 = make_shared<Dictionary>();
	double execution_time = Service::CalculateExecutionTime(cr);

	fields1->Set("check_type", CompatUtility::GetServiceCheckType(service));
	fields1->Set("current_check_attempt", service->GetCheckAttempt());
	fields1->Set("max_check_attempts", service->GetMaxCheckAttempts());
	fields1->Set("state", CompatUtility::GetServiceCurrentState(service));
	fields1->Set("state_type", service->GetStateType());

	double now = Utility::GetTime();
	std::pair<unsigned long, unsigned long> time_bag = CompatUtility::ConvertTimestamp(now);

	double end = now + execution_time;
	std::pair<unsigned long, unsigned long> time_bag_end = CompatUtility::ConvertTimestamp(end);

	fields1->Set("start_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("start_time_usec", time_bag.second);
	fields1->Set("end_time", DbValue::FromTimestamp(time_bag_end.first));
	fields1->Set("end_time_usec", time_bag_end.second);
	fields1->Set("command_object_id", service->GetCheckCommand());
	fields1->Set("command_args", Empty);
	fields1->Set("command_line", cr->GetCommand());
	fields1->Set("execution_time", execution_time);
	fields1->Set("latency", Service::CalculateLatency(cr));
	fields1->Set("return_code", cr->GetExitStatus());
	fields1->Set("output", CompatUtility::GetCheckResultOutput(cr));
	fields1->Set("long_output", CompatUtility::GetCheckResultLongOutput(cr));
	fields1->Set("perfdata", CompatUtility::GetCheckResultPerfdata(cr));

	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.Fields = fields1;
	OnQuery(query1);

	if (host->GetCheckService() == service) {
		query1.Table = "hostchecks";

		fields1 = fields1->ShallowClone();
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

	Log(LogDebug, "db_ido", "add eventhandler history for '" + service->GetName() + "'");

	double now = Utility::GetTime();
	std::pair<unsigned long, unsigned long> time_bag = CompatUtility::ConvertTimestamp(now);

	DbQuery query1;
	query1.Table = "eventhandlers";
	query1.Type = DbQueryInsert;
	query1.Category = DbCatEventHandler;

	Dictionary::Ptr fields1 = make_shared<Dictionary>();

	fields1->Set("eventhandler_type", 1); /* service */
	fields1->Set("object_id", service);
	fields1->Set("state", CompatUtility::GetServiceCurrentState(service));
	fields1->Set("state_type", service->GetStateType());

	fields1->Set("start_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("start_time_usec", time_bag.second);
	fields1->Set("end_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("end_time_usec", time_bag.second);
	fields1->Set("command_object_id", service->GetEventCommand());

	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.Fields = fields1;
	OnQuery(query1);

	if (host->GetCheckService() == service) {
		fields1 = fields1->ShallowClone();
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
	query1.Category = DbCatExternalCommand;

	Dictionary::Ptr fields1 = make_shared<Dictionary>();

	fields1->Set("entry_time", DbValue::FromTimestamp(static_cast<long>(time)));
	fields1->Set("command_type", CompatUtility::MapExternalCommandType(command));
	fields1->Set("command_name", command);
	fields1->Set("command_args", boost::algorithm::join(arguments, ";"));

	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.Fields = fields1;
	OnQuery(query1);
}
