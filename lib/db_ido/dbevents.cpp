/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "db_ido/dbevents.hpp"
#include "db_ido/dbtype.hpp"
#include "db_ido/dbvalue.hpp"
#include "base/convert.hpp"
#include "base/objectlock.hpp"
#include "base/initialize.hpp"
#include "base/configtype.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include "remote/endpoint.hpp"
#include "icinga/notification.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/externalcommandprocessor.hpp"
#include "icinga/compatutility.hpp"
#include "icinga/icingaapplication.hpp"
#include <boost/algorithm/string/join.hpp>

using namespace icinga;

INITIALIZE_ONCE(&DbEvents::StaticInitialize);

void DbEvents::StaticInitialize()
{
	/* Status */
	Comment::OnCommentAdded.connect(std::bind(&DbEvents::AddComment, _1));
	Comment::OnCommentRemoved.connect(std::bind(&DbEvents::RemoveComment, _1));
	Downtime::OnDowntimeAdded.connect(std::bind(&DbEvents::AddDowntime, _1));
	Downtime::OnDowntimeRemoved.connect(std::bind(&DbEvents::RemoveDowntime, _1));
	Downtime::OnDowntimeTriggered.connect(std::bind(&DbEvents::TriggerDowntime, _1));
	Checkable::OnAcknowledgementSet.connect(std::bind(&DbEvents::AddAcknowledgement, _1, _4));
	Checkable::OnAcknowledgementCleared.connect(std::bind(&DbEvents::RemoveAcknowledgement, _1));

	Checkable::OnNextCheckUpdated.connect(std::bind(&DbEvents::NextCheckUpdatedHandler, _1));
	Checkable::OnFlappingChanged.connect(std::bind(&DbEvents::FlappingChangedHandler, _1));
	Checkable::OnNotificationSentToAllUsers.connect(std::bind(&DbEvents::LastNotificationChangedHandler, _1, _2));

	Checkable::OnEnableActiveChecksChanged.connect(std::bind(&DbEvents::EnableActiveChecksChangedHandler, _1));
	Checkable::OnEnablePassiveChecksChanged.connect(std::bind(&DbEvents::EnablePassiveChecksChangedHandler, _1));
	Checkable::OnEnableNotificationsChanged.connect(std::bind(&DbEvents::EnableNotificationsChangedHandler, _1));
	Checkable::OnEnablePerfdataChanged.connect(std::bind(&DbEvents::EnablePerfdataChangedHandler, _1));
	Checkable::OnEnableFlappingChanged.connect(std::bind(&DbEvents::EnableFlappingChangedHandler, _1));

	Checkable::OnReachabilityChanged.connect(std::bind(&DbEvents::ReachabilityChangedHandler, _1, _2, _3));

	/* History */
	Comment::OnCommentAdded.connect(std::bind(&DbEvents::AddCommentHistory, _1));
	Downtime::OnDowntimeAdded.connect(std::bind(&DbEvents::AddDowntimeHistory, _1));
	Checkable::OnAcknowledgementSet.connect(std::bind(&DbEvents::AddAcknowledgementHistory, _1, _2, _3, _4, _5, _6));

	Checkable::OnNotificationSentToAllUsers.connect(std::bind(&DbEvents::AddNotificationHistory, _1, _2, _3, _4, _5, _6, _7));

	Checkable::OnStateChange.connect(std::bind(&DbEvents::AddStateChangeHistory, _1, _2, _3));

	Checkable::OnNewCheckResult.connect(std::bind(&DbEvents::AddCheckResultLogHistory, _1, _2));
	Checkable::OnNotificationSentToUser.connect(std::bind(&DbEvents::AddNotificationSentLogHistory, _1, _2, _3, _4, _5, _6, _7));
	Checkable::OnFlappingChanged.connect(std::bind(&DbEvents::AddFlappingChangedLogHistory, _1));
	Checkable::OnEnableFlappingChanged.connect(std::bind(&DbEvents::AddEnableFlappingChangedLogHistory, _1));
	Downtime::OnDowntimeTriggered.connect(std::bind(&DbEvents::AddTriggerDowntimeLogHistory, _1));
	Downtime::OnDowntimeRemoved.connect(std::bind(&DbEvents::AddRemoveDowntimeLogHistory, _1));

	Checkable::OnFlappingChanged.connect(std::bind(&DbEvents::AddFlappingChangedHistory, _1));
	Checkable::OnEnableFlappingChanged.connect(std::bind(&DbEvents::AddEnableFlappingChangedHistory, _1));
	Checkable::OnNewCheckResult.connect(std::bind(&DbEvents::AddCheckableCheckHistory, _1, _2));

	Checkable::OnEventCommandExecuted.connect(std::bind(&DbEvents::AddEventHandlerHistory, _1));

	ExternalCommandProcessor::OnNewExternalCommand.connect(std::bind(&DbEvents::AddExternalCommandHistory, _1, _2, _3));
}

/* check events */
void DbEvents::NextCheckUpdatedHandler(const Checkable::Ptr& checkable)
{
	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	DbQuery query1;
	if (service)
		query1.Table = "servicestatus";
	else
		query1.Table = "hoststatus";

	query1.Type = DbQueryUpdate;
	query1.Category = DbCatState;
	query1.StatusUpdate = true;
	query1.Object = DbObject::GetOrCreateByObject(checkable);

	Dictionary::Ptr fields1 = new Dictionary();
	fields1->Set("next_check", DbValue::FromTimestamp(checkable->GetNextCheck()));

	query1.Fields = fields1;

	query1.WhereCriteria = new Dictionary();
	if (service)
		query1.WhereCriteria->Set("service_object_id", service);
	else
		query1.WhereCriteria->Set("host_object_id", host);

	DbObject::OnQuery(query1);
}

void DbEvents::FlappingChangedHandler(const Checkable::Ptr& checkable)
{
	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	DbQuery query1;
	if (service)
		query1.Table = "servicestatus";
	else
		query1.Table = "hoststatus";

	query1.Type = DbQueryUpdate;
	query1.Category = DbCatState;
	query1.StatusUpdate = true;
	query1.Object = DbObject::GetOrCreateByObject(checkable);

	Dictionary::Ptr fields1 = new Dictionary();
	fields1->Set("is_flapping", checkable->IsFlapping());
	fields1->Set("percent_state_change", checkable->GetFlappingCurrent());

	query1.Fields = fields1;

	query1.WhereCriteria = new Dictionary();
	if (service)
		query1.WhereCriteria->Set("service_object_id", service);
	else
		query1.WhereCriteria->Set("host_object_id", host);

	query1.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */

	DbObject::OnQuery(query1);
}

void DbEvents::LastNotificationChangedHandler(const Notification::Ptr& notification, const Checkable::Ptr& checkable)
{
	std::pair<unsigned long, unsigned long> now_bag = ConvertTimestamp(Utility::GetTime());
	std::pair<unsigned long, unsigned long> time_bag = ConvertTimestamp(notification->GetNextNotification());

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	DbQuery query1;
	if (service)
		query1.Table = "servicestatus";
	else
		query1.Table = "hoststatus";

	query1.Type = DbQueryUpdate;
	query1.Category = DbCatState;
	query1.StatusUpdate = true;
	query1.Object = DbObject::GetOrCreateByObject(checkable);

	Dictionary::Ptr fields1 = new Dictionary();
	fields1->Set("last_notification", DbValue::FromTimestamp(now_bag.first));
	fields1->Set("next_notification", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("current_notification_number", notification->GetNotificationNumber());

	query1.Fields = fields1;

	query1.WhereCriteria = new Dictionary();
	if (service)
		query1.WhereCriteria->Set("service_object_id", service);
	else
		query1.WhereCriteria->Set("host_object_id", host);

	query1.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */

	DbObject::OnQuery(query1);
}

void DbEvents::ReachabilityChangedHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, std::set<Checkable::Ptr> children)
{
	int is_reachable = 0;

	if (cr->GetState() == ServiceOK)
		is_reachable = 1;

	Log(LogDebug, "DbEvents")
		<< "Updating reachability for checkable '" << checkable->GetName() << "': " << (is_reachable ? "" : "not" ) << " reachable for " << children.size() << " children.";

	for (const Checkable::Ptr& child : children) {
		Log(LogDebug, "DbEvents")
			<< "Updating reachability for checkable '" << child->GetName() << "': " << (is_reachable ? "" : "not" ) << " reachable.";

		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(child);

		DbQuery query1;
		if (service)
			query1.Table = "servicestatus";
		else
			query1.Table = "hoststatus";

		query1.Type = DbQueryUpdate;
		query1.Category = DbCatState;
		query1.StatusUpdate = true;
		query1.Object = DbObject::GetOrCreateByObject(child);

		Dictionary::Ptr fields1 = new Dictionary();
		fields1->Set("is_reachable", is_reachable);

		query1.Fields = fields1;

		query1.WhereCriteria = new Dictionary();
		if (service)
			query1.WhereCriteria->Set("service_object_id", service);
		else
			query1.WhereCriteria->Set("host_object_id", host);

		query1.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */

		DbObject::OnQuery(query1);
	}
}

/* enable changed events */
void DbEvents::EnableActiveChecksChangedHandler(const Checkable::Ptr& checkable)
{
	EnableChangedHandlerInternal(checkable, "active_checks_enabled", checkable->GetEnableActiveChecks());
}

void DbEvents::EnablePassiveChecksChangedHandler(const Checkable::Ptr& checkable)
{
	EnableChangedHandlerInternal(checkable, "passive_checks_enabled", checkable->GetEnablePassiveChecks());
}

void DbEvents::EnableNotificationsChangedHandler(const Checkable::Ptr& checkable)
{
	EnableChangedHandlerInternal(checkable, "notifications_enabled", checkable->GetEnableNotifications());
}

void DbEvents::EnablePerfdataChangedHandler(const Checkable::Ptr& checkable)
{
	EnableChangedHandlerInternal(checkable, "process_performance_data", checkable->GetEnablePerfdata());
}

void DbEvents::EnableFlappingChangedHandler(const Checkable::Ptr& checkable)
{
	EnableChangedHandlerInternal(checkable, "flap_detection_enabled", checkable->GetEnableFlapping());
}

void DbEvents::EnableChangedHandlerInternal(const Checkable::Ptr& checkable, const String& fieldName, bool enabled)
{
	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	DbQuery query1;
	if (service)
		query1.Table = "servicestatus";
	else
		query1.Table = "hoststatus";

	query1.Type = DbQueryUpdate;
	query1.Category = DbCatState;
	query1.StatusUpdate = true;
	query1.Object = DbObject::GetOrCreateByObject(checkable);

	Dictionary::Ptr fields1 = new Dictionary();
	fields1->Set(fieldName, enabled);
	query1.Fields = fields1;

	query1.WhereCriteria = new Dictionary();
	if (service)
		query1.WhereCriteria->Set("service_object_id", service);
	else
		query1.WhereCriteria->Set("host_object_id", host);

	query1.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */

	DbObject::OnQuery(query1);
}


/* comments */
void DbEvents::AddComments(const Checkable::Ptr& checkable)
{
	std::set<Comment::Ptr> comments = checkable->GetComments();

	std::vector<DbQuery> queries;

	for (const Comment::Ptr& comment : comments) {
		AddCommentInternal(queries, comment, false);
	}

	DbObject::OnMultipleQueries(queries);
}

void DbEvents::AddComment(const Comment::Ptr& comment)
{
	std::vector<DbQuery> queries;
	AddCommentInternal(queries, comment, false);
	DbObject::OnMultipleQueries(queries);
}

void DbEvents::AddCommentHistory(const Comment::Ptr& comment)
{
	std::vector<DbQuery> queries;
	AddCommentInternal(queries, comment, true);
	DbObject::OnMultipleQueries(queries);
}

void DbEvents::AddCommentInternal(std::vector<DbQuery>& queries, const Comment::Ptr& comment, bool historical)
{
	Checkable::Ptr checkable = comment->GetCheckable();

	unsigned long entry_time = static_cast<long>(comment->GetEntryTime());
	unsigned long entry_time_usec = (comment->GetEntryTime() - entry_time) * 1000 * 1000;

	Dictionary::Ptr fields1 = new Dictionary();
	fields1->Set("entry_time", DbValue::FromTimestamp(entry_time));
	fields1->Set("entry_time_usec", entry_time_usec);
	fields1->Set("entry_type", comment->GetEntryType());
	fields1->Set("object_id", checkable);

	if (checkable->GetReflectionType() == Host::TypeInstance) {
		fields1->Set("comment_type", 2);
		/* requires idoutils 1.10 schema fix */
		fields1->Set("internal_comment_id", comment->GetLegacyId());
	} else if (checkable->GetReflectionType() == Service::TypeInstance) {
		fields1->Set("comment_type", 1);
		fields1->Set("internal_comment_id", comment->GetLegacyId());
	} else {
		Log(LogDebug, "DbEvents", "unknown object type for adding comment.");
		return;
	}

	fields1->Set("name", comment->GetName());
	fields1->Set("comment_time", DbValue::FromTimestamp(entry_time)); /* same as entry_time */
	fields1->Set("author_name", comment->GetAuthor());
	fields1->Set("comment_data", comment->GetText());
	fields1->Set("is_persistent", comment->GetPersistent() ? 1 : 0);
	fields1->Set("comment_source", 1); /* external */
	fields1->Set("expires", (comment->GetExpireTime() > 0) ? 1 : 0);
	fields1->Set("expiration_time", DbValue::FromTimestamp(comment->GetExpireTime()));
	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	String node = IcingaApplication::GetInstance()->GetNodeName();

	Endpoint::Ptr endpoint = Endpoint::GetByName(node);
	if (endpoint)
		fields1->Set("endpoint_object_id", endpoint);

	DbQuery query1;
	if (!historical) {
		query1.Table = "comments";
		query1.Type = DbQueryInsert | DbQueryUpdate;

		fields1->Set("session_token", 0); /* DbConnection class fills in real ID */

		query1.WhereCriteria = new Dictionary();
		query1.WhereCriteria->Set("object_id", checkable);
		query1.WhereCriteria->Set("name", comment->GetName());
		query1.WhereCriteria->Set("entry_time", DbValue::FromTimestamp(entry_time));
	} else {
		query1.Table = "commenthistory";
		query1.Type = DbQueryInsert;
	}
	query1.Category = DbCatComment;
	query1.Fields = fields1;
	queries.emplace_back(std::move(query1));
}

void DbEvents::RemoveComment(const Comment::Ptr& comment)
{
	std::vector<DbQuery> queries;
	RemoveCommentInternal(queries, comment);
	DbObject::OnMultipleQueries(queries);
}

void DbEvents::RemoveCommentInternal(std::vector<DbQuery>& queries, const Comment::Ptr& comment)
{
	Checkable::Ptr checkable = comment->GetCheckable();

	unsigned long entry_time = static_cast<long>(comment->GetEntryTime());

	/* Status */
	DbQuery query1;
	query1.Table = "comments";
	query1.Type = DbQueryDelete;
	query1.Category = DbCatComment;
	query1.WhereCriteria = new Dictionary();
	query1.WhereCriteria->Set("object_id", checkable);
	query1.WhereCriteria->Set("entry_time", DbValue::FromTimestamp(entry_time));
	query1.WhereCriteria->Set("name", comment->GetName());
	queries.emplace_back(std::move(query1));

	/* History - update deletion time for service/host */
	std::pair<unsigned long, unsigned long> time_bag = ConvertTimestamp(Utility::GetTime());

	DbQuery query2;
	query2.Table = "commenthistory";
	query2.Type = DbQueryUpdate;
	query2.Category = DbCatComment;

	Dictionary::Ptr fields2 = new Dictionary();
	fields2->Set("deletion_time", DbValue::FromTimestamp(time_bag.first));
	fields2->Set("deletion_time_usec", time_bag.second);
	query2.Fields = fields2;

	query2.WhereCriteria = new Dictionary();
	query2.WhereCriteria->Set("object_id", checkable);
	query2.WhereCriteria->Set("entry_time", DbValue::FromTimestamp(entry_time));
	query2.WhereCriteria->Set("name", comment->GetName());
	queries.emplace_back(std::move(query2));
}

/* downtimes */
void DbEvents::AddDowntimes(const Checkable::Ptr& checkable)
{
	std::set<Downtime::Ptr> downtimes = checkable->GetDowntimes();

	std::vector<DbQuery> queries;

	for (const Downtime::Ptr& downtime : downtimes) {
		AddDowntimeInternal(queries, downtime, false);
	}

	DbObject::OnMultipleQueries(queries);
}

void DbEvents::AddDowntime(const Downtime::Ptr& downtime)
{
	std::vector<DbQuery> queries;
	AddDowntimeInternal(queries, downtime, false);
	DbObject::OnMultipleQueries(queries);
}

void DbEvents::AddDowntimeHistory(const Downtime::Ptr& downtime)
{
	std::vector<DbQuery> queries;
	AddDowntimeInternal(queries, downtime, true);
	DbObject::OnMultipleQueries(queries);
}

void DbEvents::AddDowntimeInternal(std::vector<DbQuery>& queries, const Downtime::Ptr& downtime, bool historical)
{
	Checkable::Ptr checkable = downtime->GetCheckable();

	Dictionary::Ptr fields1 = new Dictionary();
	fields1->Set("entry_time", DbValue::FromTimestamp(downtime->GetEntryTime()));
	fields1->Set("object_id", checkable);

	if (checkable->GetReflectionType() == Host::TypeInstance) {
		fields1->Set("downtime_type", 2);
		/* requires idoutils 1.10 schema fix */
		fields1->Set("internal_downtime_id", downtime->GetLegacyId());
	} else if (checkable->GetReflectionType() == Service::TypeInstance) {
		fields1->Set("downtime_type", 1);
		fields1->Set("internal_downtime_id", downtime->GetLegacyId());
	} else {
		Log(LogDebug, "DbEvents", "unknown object type for adding downtime.");
		return;
	}

	fields1->Set("author_name", downtime->GetAuthor());
	fields1->Set("comment_data", downtime->GetComment());
	fields1->Set("triggered_by_id", Downtime::GetByName(downtime->GetTriggeredBy()));
	fields1->Set("is_fixed", downtime->GetFixed() ? 1 : 0);
	fields1->Set("duration", downtime->GetDuration());
	fields1->Set("scheduled_start_time", DbValue::FromTimestamp(downtime->GetStartTime()));
	fields1->Set("scheduled_end_time", DbValue::FromTimestamp(downtime->GetEndTime()));
	fields1->Set("name", downtime->GetName());

	/* flexible downtimes are started at trigger time */
	if (downtime->GetFixed()) {
		std::pair<unsigned long, unsigned long> time_bag = ConvertTimestamp(downtime->GetStartTime());
		fields1->Set("actual_start_time", DbValue::FromTimestamp(time_bag.first));
		fields1->Set("actual_start_time_usec", time_bag.second);
		fields1->Set("was_started", ((downtime->GetStartTime() <= Utility::GetTime()) ? 1 : 0));
	}

	fields1->Set("is_in_effect", (downtime->IsInEffect() ? 1 : 0));
	fields1->Set("trigger_time", DbValue::FromTimestamp(downtime->GetTriggerTime()));
	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	String node = IcingaApplication::GetInstance()->GetNodeName();

	Endpoint::Ptr endpoint = Endpoint::GetByName(node);
	if (endpoint)
		fields1->Set("endpoint_object_id", endpoint);

	DbQuery query1;

	if (!historical) {
		query1.Table = "scheduleddowntime";
		query1.Type = DbQueryInsert | DbQueryUpdate;

		fields1->Set("session_token", 0); /* DbConnection class fills in real ID */

		query1.WhereCriteria = new Dictionary();
		query1.WhereCriteria->Set("object_id", checkable);
		query1.WhereCriteria->Set("name", downtime->GetName());
		query1.WhereCriteria->Set("entry_time", DbValue::FromTimestamp(downtime->GetEntryTime()));
	} else {
		query1.Table = "downtimehistory";
		query1.Type = DbQueryInsert;
	}

	query1.Category = DbCatDowntime;
	query1.Fields = fields1;
	queries.emplace_back(std::move(query1));

	/* host/service status */
	if (!historical) {
		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(checkable);

		DbQuery query2;
		if (service)
			query2.Table = "servicestatus";
		else
			query2.Table = "hoststatus";

		query2.Type = DbQueryUpdate;
		query2.Category = DbCatState;
		query2.StatusUpdate = true;
		query2.Object = DbObject::GetOrCreateByObject(checkable);

		Dictionary::Ptr fields2 = new Dictionary();
		fields2->Set("scheduled_downtime_depth", checkable->GetDowntimeDepth());

		query2.Fields = fields2;

		query2.WhereCriteria = new Dictionary();
		if (service)
			query2.WhereCriteria->Set("service_object_id", service);
		else
			query2.WhereCriteria->Set("host_object_id", host);

		query2.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */
		queries.emplace_back(std::move(query2));
	}
}

void DbEvents::RemoveDowntime(const Downtime::Ptr& downtime)
{
	std::vector<DbQuery> queries;
	RemoveDowntimeInternal(queries, downtime);
	DbObject::OnMultipleQueries(queries);
}

void DbEvents::RemoveDowntimeInternal(std::vector<DbQuery>& queries, const Downtime::Ptr& downtime)
{
	Checkable::Ptr checkable = downtime->GetCheckable();

	/* Status */
	DbQuery query1;
	query1.Table = "scheduleddowntime";
	query1.Type = DbQueryDelete;
	query1.Category = DbCatDowntime;
	query1.WhereCriteria = new Dictionary();

	query1.WhereCriteria->Set("object_id", checkable);
	query1.WhereCriteria->Set("entry_time", DbValue::FromTimestamp(downtime->GetEntryTime()));
	query1.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */
	query1.WhereCriteria->Set("scheduled_start_time", DbValue::FromTimestamp(downtime->GetStartTime()));
	query1.WhereCriteria->Set("scheduled_end_time", DbValue::FromTimestamp(downtime->GetEndTime()));
	query1.WhereCriteria->Set("name", downtime->GetName());
	queries.emplace_back(std::move(query1));

	/* History - update actual_end_time, was_cancelled for service (and host in case) */
	std::pair<unsigned long, unsigned long> time_bag = ConvertTimestamp(Utility::GetTime());

	DbQuery query3;
	query3.Table = "downtimehistory";
	query3.Type = DbQueryUpdate;
	query3.Category = DbCatDowntime;

	Dictionary::Ptr fields3 = new Dictionary();
	fields3->Set("was_cancelled", downtime->GetWasCancelled() ? 1 : 0);

	if (downtime->GetFixed() || (!downtime->GetFixed() && downtime->GetTriggerTime() > 0)) {
		fields3->Set("actual_end_time", DbValue::FromTimestamp(time_bag.first));
		fields3->Set("actual_end_time_usec", time_bag.second);
	}

	fields3->Set("is_in_effect", 0);
	query3.Fields = fields3;

	query3.WhereCriteria = new Dictionary();
	query3.WhereCriteria->Set("object_id", checkable);
	query3.WhereCriteria->Set("entry_time", DbValue::FromTimestamp(downtime->GetEntryTime()));
	query3.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */
	query3.WhereCriteria->Set("scheduled_start_time", DbValue::FromTimestamp(downtime->GetStartTime()));
	query3.WhereCriteria->Set("scheduled_end_time", DbValue::FromTimestamp(downtime->GetEndTime()));
	query3.WhereCriteria->Set("name", downtime->GetName());
	queries.emplace_back(std::move(query3));

	/* host/service status */
	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	DbQuery query4;
	if (service)
		query4.Table = "servicestatus";
	else
		query4.Table = "hoststatus";

	query4.Type = DbQueryUpdate;
	query4.Category = DbCatState;
	query4.StatusUpdate = true;
	query4.Object = DbObject::GetOrCreateByObject(checkable);

	Dictionary::Ptr fields4 = new Dictionary();
	fields4->Set("scheduled_downtime_depth", checkable->GetDowntimeDepth());

	query4.Fields = fields4;

	query4.WhereCriteria = new Dictionary();
	if (service)
		query4.WhereCriteria->Set("service_object_id", service);
	else
		query4.WhereCriteria->Set("host_object_id", host);

	query4.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */
	queries.emplace_back(std::move(query4));
}

void DbEvents::TriggerDowntime(const Downtime::Ptr& downtime)
{
	Checkable::Ptr checkable = downtime->GetCheckable();

	std::pair<unsigned long, unsigned long> time_bag = ConvertTimestamp(Utility::GetTime());

	/* Status */
	DbQuery query1;
	query1.Table = "scheduleddowntime";
	query1.Type = DbQueryUpdate;
	query1.Category = DbCatDowntime;

	Dictionary::Ptr fields1 = new Dictionary();
	fields1->Set("was_started", 1);
	fields1->Set("actual_start_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("actual_start_time_usec", time_bag.second);
	fields1->Set("is_in_effect", (downtime->IsInEffect() ? 1 : 0));
	fields1->Set("trigger_time", DbValue::FromTimestamp(downtime->GetTriggerTime()));
	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query1.WhereCriteria = new Dictionary();
	query1.WhereCriteria->Set("object_id", checkable);
	query1.WhereCriteria->Set("entry_time", DbValue::FromTimestamp(downtime->GetEntryTime()));
	query1.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */
	query1.WhereCriteria->Set("scheduled_start_time", DbValue::FromTimestamp(downtime->GetStartTime()));
	query1.WhereCriteria->Set("scheduled_end_time", DbValue::FromTimestamp(downtime->GetEndTime()));
	query1.WhereCriteria->Set("name", downtime->GetName());

	query1.Fields = fields1;
	DbObject::OnQuery(query1);

	/* History - downtime was started for service (and host in case) */
	DbQuery query3;
	query3.Table = "downtimehistory";
	query3.Type = DbQueryUpdate;
	query3.Category = DbCatDowntime;

	Dictionary::Ptr fields3 = new Dictionary();
	fields3->Set("was_started", 1);
	fields3->Set("is_in_effect", 1);
	fields3->Set("actual_start_time", DbValue::FromTimestamp(time_bag.first));
	fields3->Set("actual_start_time_usec", time_bag.second);
	fields3->Set("trigger_time", DbValue::FromTimestamp(downtime->GetTriggerTime()));
	query3.Fields = fields3;

	query3.WhereCriteria = query1.WhereCriteria;

	DbObject::OnQuery(query3);

	/* host/service status */
	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	DbQuery query4;
	if (service)
		query4.Table = "servicestatus";
	else
		query4.Table = "hoststatus";

	query4.Type = DbQueryUpdate;
	query4.Category = DbCatState;
	query4.StatusUpdate = true;
	query4.Object = DbObject::GetOrCreateByObject(checkable);

	Dictionary::Ptr fields4 = new Dictionary();
	fields4->Set("scheduled_downtime_depth", checkable->GetDowntimeDepth());

	query4.Fields = fields4;

	query4.WhereCriteria = new Dictionary();
	if (service)
		query4.WhereCriteria->Set("service_object_id", service);
	else
		query4.WhereCriteria->Set("host_object_id", host);

	query4.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */

	DbObject::OnQuery(query4);
}

/* acknowledgements */
void DbEvents::AddAcknowledgementHistory(const Checkable::Ptr& checkable, const String& author, const String& comment,
	AcknowledgementType type, bool notify, double expiry)
{
	Log(LogDebug, "DbEvents")
		<< "add acknowledgement history for '" << checkable->GetName() << "'";

	std::pair<unsigned long, unsigned long> time_bag = ConvertTimestamp(Utility::GetTime());

	unsigned long end_time = static_cast<long>(expiry);

	DbQuery query1;
	query1.Table = "acknowledgements";
	query1.Type = DbQueryInsert;
	query1.Category = DbCatAcknowledgement;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr fields1 = new Dictionary();
	fields1->Set("entry_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("entry_time_usec", time_bag.second);
	fields1->Set("acknowledgement_type", type);
	fields1->Set("object_id", checkable);
	fields1->Set("author_name", author);
	fields1->Set("comment_data", comment);
	fields1->Set("persistent_comment", 1); //always persistent
	fields1->Set("notify_contacts", notify ? 1 : 0);
	fields1->Set("is_sticky", type == AcknowledgementSticky ? 1 : 0);
	fields1->Set("end_time", DbValue::FromTimestamp(end_time));
	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	if (service) {
		fields1->Set("state", service->GetState());
	} else {
		fields1->Set("state", CompatUtility::GetHostCurrentState(host));
	}

	String node = IcingaApplication::GetInstance()->GetNodeName();

	Endpoint::Ptr endpoint = Endpoint::GetByName(node);
	if (endpoint)
		fields1->Set("endpoint_object_id", endpoint);

	query1.Fields = fields1;
	DbObject::OnQuery(query1);
}

void DbEvents::AddAcknowledgement(const Checkable::Ptr& checkable, AcknowledgementType type)
{
	Log(LogDebug, "DbEvents")
		<< "add acknowledgement for '" << checkable->GetName() << "'";

	AddAcknowledgementInternal(checkable, type, true);
}

void DbEvents::RemoveAcknowledgement(const Checkable::Ptr& checkable)
{
	Log(LogDebug, "DbEvents")
		<< "remove acknowledgement for '" << checkable->GetName() << "'";

	AddAcknowledgementInternal(checkable, AcknowledgementNone, false);
}

void DbEvents::AddAcknowledgementInternal(const Checkable::Ptr& checkable, AcknowledgementType type, bool add)
{
	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	DbQuery query1;
	if (service)
		query1.Table = "servicestatus";
	else
		query1.Table = "hoststatus";

	query1.Type = DbQueryUpdate;
	query1.Category = DbCatState;
	query1.StatusUpdate = true;
	query1.Object = DbObject::GetOrCreateByObject(checkable);

	Dictionary::Ptr fields1 = new Dictionary();
	fields1->Set("acknowledgement_type", type);
	fields1->Set("problem_has_been_acknowledged", add ? 1 : 0);
	query1.Fields = fields1;

	query1.WhereCriteria = new Dictionary();
	if (service)
		query1.WhereCriteria->Set("service_object_id", service);
	else
		query1.WhereCriteria->Set("host_object_id", host);

	query1.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */

	DbObject::OnQuery(query1);
}

/* notifications */
void DbEvents::AddNotificationHistory(const Notification::Ptr& notification, const Checkable::Ptr& checkable, const std::set<User::Ptr>& users, NotificationType type,
	const CheckResult::Ptr& cr, const String& author, const String& text)
{
	Log(LogDebug, "DbEvents")
		<< "add notification history for '" << checkable->GetName() << "'";

	/* start and end happen at the same time */
	std::pair<unsigned long, unsigned long> time_bag = ConvertTimestamp(Utility::GetTime());

	DbQuery query1;
	query1.Table = "notifications";
	query1.Type = DbQueryInsert;
	query1.Category = DbCatNotification;
	query1.NotificationInsertID = new DbValue(DbValueObjectInsertID, -1);

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr fields1 = new Dictionary();
	fields1->Set("notification_type", 1); /* service */
	fields1->Set("notification_reason", MapNotificationReasonType(type));
	fields1->Set("object_id", checkable);
	fields1->Set("start_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("start_time_usec", time_bag.second);
	fields1->Set("end_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("end_time_usec", time_bag.second);

	if (service) {
		fields1->Set("state", service->GetState());
	} else {
		fields1->Set("state", CompatUtility::GetHostCurrentState(host));
	}

	if (cr) {
		fields1->Set("output", CompatUtility::GetCheckResultOutput(cr));
		fields1->Set("long_output", CompatUtility::GetCheckResultLongOutput(cr));
	}

	fields1->Set("escalated", 0);
	fields1->Set("contacts_notified", static_cast<long>(users.size()));
	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	String node = IcingaApplication::GetInstance()->GetNodeName();

	Endpoint::Ptr endpoint = Endpoint::GetByName(node);
	if (endpoint)
		fields1->Set("endpoint_object_id", endpoint);

	query1.Fields = fields1;
	DbObject::OnQuery(query1);

	std::vector<DbQuery> queries;

	for (const User::Ptr& user : users) {
		Log(LogDebug, "DbEvents")
			<< "add contact notification history for service '" << checkable->GetName() << "' and user '" << user->GetName() << "'.";

		DbQuery query2;
		query2.Table = "contactnotifications";
		query2.Type = DbQueryInsert;
		query2.Category = DbCatNotification;

		Dictionary::Ptr fields2 = new Dictionary();
		fields2->Set("contact_object_id", user);
		fields2->Set("start_time", DbValue::FromTimestamp(time_bag.first));
		fields2->Set("start_time_usec", time_bag.second);
		fields2->Set("end_time", DbValue::FromTimestamp(time_bag.first));
		fields2->Set("end_time_usec", time_bag.second);

		fields2->Set("notification_id", query1.NotificationInsertID);
		fields2->Set("instance_id", 0); /* DbConnection class fills in real ID */

		query2.Fields = fields2;
		queries.emplace_back(std::move(query2));
	}

	DbObject::OnMultipleQueries(queries);
}

/* statehistory */
void DbEvents::AddStateChangeHistory(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type)
{
	Log(LogDebug, "DbEvents")
		<< "add state change history for '" << checkable->GetName() << "'";

	double ts = cr->GetExecutionEnd();
	std::pair<unsigned long, unsigned long> state_time_bag = ConvertTimestamp(ts);

	DbQuery query1;
	query1.Table = "statehistory";
	query1.Type = DbQueryInsert;
	query1.Category = DbCatStateHistory;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr fields1 = new Dictionary();
	fields1->Set("state_time", DbValue::FromTimestamp(state_time_bag.first));
	fields1->Set("state_time_usec", state_time_bag.second);
	fields1->Set("object_id", checkable);
	fields1->Set("state_change", 1); /* service */
	fields1->Set("state_type", checkable->GetStateType());
	fields1->Set("current_check_attempt", checkable->GetCheckAttempt());
	fields1->Set("max_check_attempts", checkable->GetMaxCheckAttempts());

	if (service) {
		fields1->Set("state", service->GetState());
		fields1->Set("last_state", service->GetLastState());
		fields1->Set("last_hard_state", service->GetLastHardState());
	} else {
		fields1->Set("state", CompatUtility::GetHostCurrentState(host));
		fields1->Set("last_state", host->GetLastState());
		fields1->Set("last_hard_state", host->GetLastHardState());
	}

	if (cr) {
		fields1->Set("output", CompatUtility::GetCheckResultOutput(cr));
		fields1->Set("long_output", CompatUtility::GetCheckResultLongOutput(cr));
		fields1->Set("check_source", cr->GetCheckSource());
	}

	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	String node = IcingaApplication::GetInstance()->GetNodeName();

	Endpoint::Ptr endpoint = Endpoint::GetByName(node);
	if (endpoint)
		fields1->Set("endpoint_object_id", endpoint);

	query1.Fields = fields1;
	DbObject::OnQuery(query1);
}

/* logentries */
void DbEvents::AddCheckResultLogHistory(const Checkable::Ptr& checkable, const CheckResult::Ptr &cr)
{
	Dictionary::Ptr vars_after = cr->GetVarsAfter();

	long state_after = vars_after->Get("state");
	long stateType_after = vars_after->Get("state_type");
	long attempt_after = vars_after->Get("attempt");
	bool reachable_after = vars_after->Get("reachable");

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
	String output;

	if (cr)
		output = CompatUtility::GetCheckResultOutput(cr);

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	std::ostringstream msgbuf;

	if (service) {
		msgbuf << "SERVICE ALERT: "
			<< host->GetName() << ";"
			<< service->GetShortName() << ";"
			<< Service::StateToString(service->GetState()) << ";"
			<< Service::StateTypeToString(service->GetStateType()) << ";"
			<< attempt_after << ";"
			<< output << ""
			<< "";

		switch (service->GetState()) {
			case ServiceOK:
				type = LogEntryTypeServiceOk;
				break;
			case ServiceUnknown:
				type = LogEntryTypeServiceUnknown;
				break;
			case ServiceWarning:
				type = LogEntryTypeServiceWarning;
				break;
			case ServiceCritical:
				type = LogEntryTypeServiceCritical;
				break;
			default:
				Log(LogCritical, "DbEvents")
					<< "Unknown service state: " << state_after;
				return;
		}
	} else {
		msgbuf << "HOST ALERT: "
			<< host->GetName() << ";"
			<< CompatUtility::GetHostStateString(host) << ";"
			<< Host::StateTypeToString(host->GetStateType()) << ";"
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
			default:
				Log(LogCritical, "DbEvents")
					<< "Unknown host state: " << state_after;
				return;
		}

		if (!reachable_after)
			type = LogEntryTypeHostUnreachable;
	}

	AddLogHistory(checkable, msgbuf.str(), type);
}

void DbEvents::AddTriggerDowntimeLogHistory(const Downtime::Ptr& downtime)
{
	Checkable::Ptr checkable = downtime->GetCheckable();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	std::ostringstream msgbuf;

	if (service) {
		msgbuf << "SERVICE DOWNTIME ALERT: "
			<< host->GetName() << ";"
			<< service->GetShortName() << ";"
			<< "STARTED" << "; "
			<< "Service has entered a period of scheduled downtime."
			<< "";
	} else {
		msgbuf << "HOST DOWNTIME ALERT: "
			<< host->GetName() << ";"
			<< "STARTED" << "; "
			<< "Service has entered a period of scheduled downtime."
			<< "";
	}

	AddLogHistory(checkable, msgbuf.str(), LogEntryTypeInfoMessage);
}

void DbEvents::AddRemoveDowntimeLogHistory(const Downtime::Ptr& downtime)
{
	Checkable::Ptr checkable = downtime->GetCheckable();

	String downtime_output;
	String downtime_state_str;

	if (downtime->GetWasCancelled()) {
		downtime_output = "Scheduled downtime for service has been cancelled.";
		downtime_state_str = "CANCELLED";
	} else {
		downtime_output = "Service has exited from a period of scheduled downtime.";
		downtime_state_str = "STOPPED";
	}

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	std::ostringstream msgbuf;

	if (service) {
		msgbuf << "SERVICE DOWNTIME ALERT: "
			<< host->GetName() << ";"
			<< service->GetShortName() << ";"
			<< downtime_state_str << "; "
			<< downtime_output
			<< "";
	} else {
		msgbuf << "HOST DOWNTIME ALERT: "
			<< host->GetName() << ";"
			<< downtime_state_str << "; "
			<< downtime_output
			<< "";
	}

	AddLogHistory(checkable, msgbuf.str(), LogEntryTypeInfoMessage);
}

void DbEvents::AddNotificationSentLogHistory(const Notification::Ptr& notification, const Checkable::Ptr& checkable, const User::Ptr& user,
	NotificationType notification_type, const CheckResult::Ptr& cr,
	const String& author, const String& comment_text)
{
	CheckCommand::Ptr commandObj = checkable->GetCheckCommand();

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

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	std::ostringstream msgbuf;

	if (service) {
		msgbuf << "SERVICE NOTIFICATION: "
			<< user->GetName() << ";"
			<< host->GetName() << ";"
			<< service->GetShortName() << ";"
			<< notification_type_str << " "
			<< "(" << Service::StateToString(service->GetState()) << ");"
			<< check_command << ";"
			<< output << author_comment
			<< "";
	} else {
		msgbuf << "HOST NOTIFICATION: "
			<< user->GetName() << ";"
			<< host->GetName() << ";"
			<< notification_type_str << " "
			<< "(" << Host::StateToString(host->GetState()) << ");"
			<< check_command << ";"
			<< output << author_comment
			<< "";
	}

	AddLogHistory(checkable, msgbuf.str(), LogEntryTypeHostNotification);
}

void DbEvents::AddFlappingChangedLogHistory(const Checkable::Ptr& checkable)
{
	String flapping_state_str;
	String flapping_output;

	if (checkable->IsFlapping()) {
		flapping_output = "Service appears to have started flapping (" + Convert::ToString(checkable->GetFlappingCurrent()) + "% change >= " + Convert::ToString(checkable->GetFlappingThresholdHigh()) + "% threshold)";
		flapping_state_str = "STARTED";
	} else {
		flapping_output = "Service appears to have stopped flapping (" + Convert::ToString(checkable->GetFlappingCurrent()) + "% change < " + Convert::ToString(checkable->GetFlappingThresholdLow()) + "% threshold)";
		flapping_state_str = "STOPPED";
	}

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	std::ostringstream msgbuf;

	if (service) {
		msgbuf << "SERVICE FLAPPING ALERT: "
			<< host->GetName() << ";"
			<< service->GetShortName() << ";"
			<< flapping_state_str << "; "
			<< flapping_output
			<< "";
	} else {
		msgbuf << "HOST FLAPPING ALERT: "
			<< host->GetName() << ";"
			<< flapping_state_str << "; "
			<< flapping_output
			<< "";
	}

	AddLogHistory(checkable, msgbuf.str(), LogEntryTypeInfoMessage);
}

void DbEvents::AddEnableFlappingChangedLogHistory(const Checkable::Ptr& checkable)
{
	if (!checkable->GetEnableFlapping())
		return;

	String flapping_output = "Flap detection has been disabled";
	String flapping_state_str = "DISABLED";

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	std::ostringstream msgbuf;

	if (service) {
		msgbuf << "SERVICE FLAPPING ALERT: "
			<< host->GetName() << ";"
			<< service->GetShortName() << ";"
			<< flapping_state_str << "; "
			<< flapping_output
			<< "";
	} else {
		msgbuf << "HOST FLAPPING ALERT: "
			<< host->GetName() << ";"
			<< flapping_state_str << "; "
			<< flapping_output
			<< "";
	}

	AddLogHistory(checkable, msgbuf.str(), LogEntryTypeInfoMessage);
}

void DbEvents::AddLogHistory(const Checkable::Ptr& checkable, const String& buffer, LogEntryType type)
{
	Log(LogDebug, "DbEvents")
		<< "add log entry history for '" << checkable->GetName() << "'";

	std::pair<unsigned long, unsigned long> time_bag = ConvertTimestamp(Utility::GetTime());

	DbQuery query1;
	query1.Table = "logentries";
	query1.Type = DbQueryInsert;
	query1.Category = DbCatLog;

	Dictionary::Ptr fields1 = new Dictionary();
	fields1->Set("logentry_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("entry_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("entry_time_usec", time_bag.second);
	fields1->Set("object_id", checkable); // added in 1.10 see #4754
	fields1->Set("logentry_type", type);
	fields1->Set("logentry_data", buffer);

	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	String node = IcingaApplication::GetInstance()->GetNodeName();

	Endpoint::Ptr endpoint = Endpoint::GetByName(node);
	if (endpoint)
		fields1->Set("endpoint_object_id", endpoint);

	query1.Fields = fields1;
	DbObject::OnQuery(query1);
}

/* flappinghistory */
void DbEvents::AddFlappingChangedHistory(const Checkable::Ptr& checkable)
{
	Log(LogDebug, "DbEvents")
		<< "add flapping history for '" << checkable->GetName() << "'";

	std::pair<unsigned long, unsigned long> time_bag = ConvertTimestamp(Utility::GetTime());

	DbQuery query1;
	query1.Table = "flappinghistory";
	query1.Type = DbQueryInsert;
	query1.Category = DbCatFlapping;

	Dictionary::Ptr fields1 = new Dictionary();

	fields1->Set("event_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("event_time_usec", time_bag.second);

	if (checkable->IsFlapping())
		fields1->Set("event_type", 1000);
	else {
		fields1->Set("event_type", 1001);
		fields1->Set("reason_type", 1);
	}

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	fields1->Set("flapping_type", service ? 1 : 0);
	fields1->Set("object_id", checkable);
	fields1->Set("percent_state_change", checkable->GetFlappingCurrent());
	fields1->Set("low_threshold", checkable->GetFlappingThresholdLow());
	fields1->Set("high_threshold", checkable->GetFlappingThresholdHigh());

	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	String node = IcingaApplication::GetInstance()->GetNodeName();

	Endpoint::Ptr endpoint = Endpoint::GetByName(node);
	if (endpoint)
		fields1->Set("endpoint_object_id", endpoint);

	query1.Fields = fields1;
	DbObject::OnQuery(query1);
}

void DbEvents::AddEnableFlappingChangedHistory(const Checkable::Ptr& checkable)
{
	Log(LogDebug, "DbEvents")
		<< "add flapping history for '" << checkable->GetName() << "'";

	std::pair<unsigned long, unsigned long> time_bag = ConvertTimestamp(Utility::GetTime());

	DbQuery query1;
	query1.Table = "flappinghistory";
	query1.Type = DbQueryInsert;
	query1.Category = DbCatFlapping;

	Dictionary::Ptr fields1 = new Dictionary();

	fields1->Set("event_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("event_time_usec", time_bag.second);

	if (!checkable->GetEnableFlapping())
		return;

	fields1->Set("event_type", 1001);
	fields1->Set("reason_type", 2);

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	fields1->Set("flapping_type", service ? 1 : 0);
	fields1->Set("object_id", checkable);
	fields1->Set("percent_state_change", checkable->GetFlappingCurrent());
	fields1->Set("low_threshold", checkable->GetFlappingThresholdLow());
	fields1->Set("high_threshold", checkable->GetFlappingThresholdHigh());

	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	String node = IcingaApplication::GetInstance()->GetNodeName();

	Endpoint::Ptr endpoint = Endpoint::GetByName(node);
	if (endpoint)
		fields1->Set("endpoint_object_id", endpoint);

	query1.Fields = fields1;
	DbObject::OnQuery(query1);
}

/* servicechecks */
void DbEvents::AddCheckableCheckHistory(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	if (!cr)
		return;

	Log(LogDebug, "DbEvents")
		<< "add checkable check history for '" << checkable->GetName() << "'";

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	std::ostringstream msgbuf;

	DbQuery query1;
	query1.Table = service ? "servicechecks" : "hostchecks";
	query1.Type = DbQueryInsert;
	query1.Category = DbCatCheck;

	Dictionary::Ptr fields1 = new Dictionary();
	fields1->Set("check_type", !checkable->GetEnableActiveChecks()); /* 0 .. active, 1 .. passive */
	fields1->Set("current_check_attempt", checkable->GetCheckAttempt());
	fields1->Set("max_check_attempts", checkable->GetMaxCheckAttempts());
	fields1->Set("state_type", checkable->GetStateType());

	double start = cr->GetExecutionStart();
	std::pair<unsigned long, unsigned long> time_bag_start = ConvertTimestamp(start);

	double end = cr->GetExecutionEnd();
	std::pair<unsigned long, unsigned long> time_bag_end = ConvertTimestamp(end);

	double execution_time = cr->CalculateExecutionTime();

	fields1->Set("start_time", DbValue::FromTimestamp(time_bag_start.first));
	fields1->Set("start_time_usec", time_bag_start.second);
	fields1->Set("end_time", DbValue::FromTimestamp(time_bag_end.first));
	fields1->Set("end_time_usec", time_bag_end.second);
	fields1->Set("command_object_id", checkable->GetCheckCommand());
	fields1->Set("command_args", Empty);
	fields1->Set("command_line", CompatUtility::GetCommandLine(checkable->GetCheckCommand()));
	fields1->Set("execution_time", Convert::ToString(execution_time));
	fields1->Set("latency", Convert::ToString(cr->CalculateLatency()));
	fields1->Set("return_code", cr->GetExitStatus());
	fields1->Set("output", CompatUtility::GetCheckResultOutput(cr));
	fields1->Set("long_output", CompatUtility::GetCheckResultLongOutput(cr));
	fields1->Set("perfdata", CompatUtility::GetCheckResultPerfdata(cr));

	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	if (service) {
		fields1->Set("service_object_id", service);
		fields1->Set("state", service->GetState());
	} else {
		fields1->Set("host_object_id", host);
		fields1->Set("state", CompatUtility::GetHostCurrentState(host));
	}

	String node = IcingaApplication::GetInstance()->GetNodeName();

	Endpoint::Ptr endpoint = Endpoint::GetByName(node);
	if (endpoint)
		fields1->Set("endpoint_object_id", endpoint);

	query1.Fields = fields1;
	DbObject::OnQuery(query1);
}

/* eventhandlers */
void DbEvents::AddEventHandlerHistory(const Checkable::Ptr& checkable)
{
	Log(LogDebug, "DbEvents")
		<< "add eventhandler history for '" << checkable->GetName() << "'";

	std::pair<unsigned long, unsigned long> time_bag = ConvertTimestamp(Utility::GetTime());

	DbQuery query1;
	query1.Table = "eventhandlers";
	query1.Type = DbQueryInsert;
	query1.Category = DbCatEventHandler;

	Dictionary::Ptr fields1 = new Dictionary();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	fields1->Set("object_id", checkable);

	if (service) {
		fields1->Set("state", service->GetState());
		fields1->Set("eventhandler_type", 1);
	} else {
		fields1->Set("state", CompatUtility::GetHostCurrentState(host));
		fields1->Set("eventhandler_type", 0);
	}

	fields1->Set("state_type", checkable->GetStateType());

	fields1->Set("start_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("start_time_usec", time_bag.second);
	fields1->Set("end_time", DbValue::FromTimestamp(time_bag.first));
	fields1->Set("end_time_usec", time_bag.second);
	fields1->Set("command_object_id", checkable->GetEventCommand());

	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	String node = IcingaApplication::GetInstance()->GetNodeName();

	Endpoint::Ptr endpoint = Endpoint::GetByName(node);
	if (endpoint)
		fields1->Set("endpoint_object_id", endpoint);

	query1.Fields = fields1;
	DbObject::OnQuery(query1);
}

/* externalcommands */
void DbEvents::AddExternalCommandHistory(double time, const String& command, const std::vector<String>& arguments)
{
	Log(LogDebug, "DbEvents", "add external command history");

	DbQuery query1;
	query1.Table = "externalcommands";
	query1.Type = DbQueryInsert;
	query1.Category = DbCatExternalCommand;

	Dictionary::Ptr fields1 = new Dictionary();

	fields1->Set("entry_time", DbValue::FromTimestamp(static_cast<long>(time)));
	fields1->Set("command_type", MapExternalCommandType(command));
	fields1->Set("command_name", command);
	fields1->Set("command_args", boost::algorithm::join(arguments, ";"));

	fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

	String node = IcingaApplication::GetInstance()->GetNodeName();

	Endpoint::Ptr endpoint = Endpoint::GetByName(node);
	if (endpoint)
		fields1->Set("endpoint_object_id", endpoint);

	query1.Fields = fields1;
	DbObject::OnQuery(query1);
}

std::pair<unsigned long, unsigned long> DbEvents::ConvertTimestamp(double time)
{
	unsigned long time_sec = static_cast<long>(time);
	unsigned long time_usec = (time - time_sec) * 1000 * 1000;

	return std::make_pair(time_sec, time_usec);
}

int DbEvents::MapNotificationReasonType(NotificationType type)
{
	switch (type) {
		case NotificationDowntimeStart:
			return 5;
		case NotificationDowntimeEnd:
			return 6;
		case NotificationDowntimeRemoved:
			return 7;
		case NotificationCustom:
			return 8;
		case NotificationAcknowledgement:
			return 1;
		case NotificationProblem:
			return 0;
		case NotificationRecovery:
			return 0;
		case NotificationFlappingStart:
			return 2;
		case NotificationFlappingEnd:
			return 3;
		default:
			return 0;
	}
}

int DbEvents::MapExternalCommandType(const String& name)
{
	if (name == "NONE")
		return 0;
	if (name == "ADD_HOST_COMMENT")
		return 1;
	if (name == "DEL_HOST_COMMENT")
		return 2;
	if (name == "ADD_SVC_COMMENT")
		return 3;
	if (name == "DEL_SVC_COMMENT")
		return 4;
	if (name == "ENABLE_SVC_CHECK")
		return 5;
	if (name == "DISABLE_SVC_CHECK")
		return 6;
	if (name == "SCHEDULE_SVC_CHECK")
		return 7;
	if (name == "DELAY_SVC_NOTIFICATION")
		return 9;
	if (name == "DELAY_HOST_NOTIFICATION")
		return 10;
	if (name == "DISABLE_NOTIFICATIONS")
		return 11;
	if (name == "ENABLE_NOTIFICATIONS")
		return 12;
	if (name == "RESTART_PROCESS")
		return 13;
	if (name == "SHUTDOWN_PROCESS")
		return 14;
	if (name == "ENABLE_HOST_SVC_CHECKS")
		return 15;
	if (name == "DISABLE_HOST_SVC_CHECKS")
		return 16;
	if (name == "SCHEDULE_HOST_SVC_CHECKS")
		return 17;
	if (name == "DELAY_HOST_SVC_NOTIFICATIONS")
		return 19;
	if (name == "DEL_ALL_HOST_COMMENTS")
		return 20;
	if (name == "DEL_ALL_SVC_COMMENTS")
		return 21;
	if (name == "ENABLE_SVC_NOTIFICATIONS")
		return 22;
	if (name == "DISABLE_SVC_NOTIFICATIONS")
		return 23;
	if (name == "ENABLE_HOST_NOTIFICATIONS")
		return 24;
	if (name == "DISABLE_HOST_NOTIFICATIONS")
		return 25;
	if (name == "ENABLE_ALL_NOTIFICATIONS_BEYOND_HOST")
		return 26;
	if (name == "DISABLE_ALL_NOTIFICATIONS_BEYOND_HOST")
		return 27;
	if (name == "ENABLE_HOST_SVC_NOTIFICATIONS")
		return 28;
	if (name == "DISABLE_HOST_SVC_NOTIFICATIONS")
		return 29;
	if (name == "PROCESS_SERVICE_CHECK_RESULT")
		return 30;
	if (name == "SAVE_STATE_INFORMATION")
		return 31;
	if (name == "READ_STATE_INFORMATION")
		return 32;
	if (name == "ACKNOWLEDGE_HOST_PROBLEM")
		return 33;
	if (name == "ACKNOWLEDGE_SVC_PROBLEM")
		return 34;
	if (name == "START_EXECUTING_SVC_CHECKS")
		return 35;
	if (name == "STOP_EXECUTING_SVC_CHECKS")
		return 36;
	if (name == "START_ACCEPTING_PASSIVE_SVC_CHECKS")
		return 37;
	if (name == "STOP_ACCEPTING_PASSIVE_SVC_CHECKS")
		return 38;
	if (name == "ENABLE_PASSIVE_SVC_CHECKS")
		return 39;
	if (name == "DISABLE_PASSIVE_SVC_CHECKS")
		return 40;
	if (name == "ENABLE_EVENT_HANDLERS")
		return 41;
	if (name == "DISABLE_EVENT_HANDLERS")
		return 42;
	if (name == "ENABLE_HOST_EVENT_HANDLER")
		return 43;
	if (name == "DISABLE_HOST_EVENT_HANDLER")
		return 44;
	if (name == "ENABLE_SVC_EVENT_HANDLER")
		return 45;
	if (name == "DISABLE_SVC_EVENT_HANDLER")
		return 46;
	if (name == "ENABLE_HOST_CHECK")
		return 47;
	if (name == "DISABLE_HOST_CHECK")
		return 48;
	if (name == "START_OBSESSING_OVER_SVC_CHECKS")
		return 49;
	if (name == "STOP_OBSESSING_OVER_SVC_CHECKS")
		return 50;
	if (name == "REMOVE_HOST_ACKNOWLEDGEMENT")
		return 51;
	if (name == "REMOVE_SVC_ACKNOWLEDGEMENT")
		return 52;
	if (name == "SCHEDULE_FORCED_HOST_SVC_CHECKS")
		return 53;
	if (name == "SCHEDULE_FORCED_SVC_CHECK")
		return 54;
	if (name == "SCHEDULE_HOST_DOWNTIME")
		return 55;
	if (name == "SCHEDULE_SVC_DOWNTIME")
		return 56;
	if (name == "ENABLE_HOST_FLAP_DETECTION")
		return 57;
	if (name == "DISABLE_HOST_FLAP_DETECTION")
		return 58;
	if (name == "ENABLE_SVC_FLAP_DETECTION")
		return 59;
	if (name == "DISABLE_SVC_FLAP_DETECTION")
		return 60;
	if (name == "ENABLE_FLAP_DETECTION")
		return 61;
	if (name == "DISABLE_FLAP_DETECTION")
		return 62;
	if (name == "ENABLE_HOSTGROUP_SVC_NOTIFICATIONS")
		return 63;
	if (name == "DISABLE_HOSTGROUP_SVC_NOTIFICATIONS")
		return 64;
	if (name == "ENABLE_HOSTGROUP_HOST_NOTIFICATIONS")
		return 65;
	if (name == "DISABLE_HOSTGROUP_HOST_NOTIFICATIONS")
		return 66;
	if (name == "ENABLE_HOSTGROUP_SVC_CHECKS")
		return 67;
	if (name == "DISABLE_HOSTGROUP_SVC_CHECKS")
		return 68;
	if (name == "CANCEL_HOST_DOWNTIME")
		return 69;
	if (name == "CANCEL_SVC_DOWNTIME")
		return 70;
	if (name == "CANCEL_ACTIVE_HOST_DOWNTIME")
		return 71;
	if (name == "CANCEL_PENDING_HOST_DOWNTIME")
		return 72;
	if (name == "CANCEL_ACTIVE_SVC_DOWNTIME")
		return 73;
	if (name == "CANCEL_PENDING_SVC_DOWNTIME")
		return 74;
	if (name == "CANCEL_ACTIVE_HOST_SVC_DOWNTIME")
		return 75;
	if (name == "CANCEL_PENDING_HOST_SVC_DOWNTIME")
		return 76;
	if (name == "FLUSH_PENDING_COMMANDS")
		return 77;
	if (name == "DEL_HOST_DOWNTIME")
		return 78;
	if (name == "DEL_SVC_DOWNTIME")
		return 79;
	if (name == "ENABLE_FAILURE_PREDICTION")
		return 80;
	if (name == "DISABLE_FAILURE_PREDICTION")
		return 81;
	if (name == "ENABLE_PERFORMANCE_DATA")
		return 82;
	if (name == "DISABLE_PERFORMANCE_DATA")
		return 83;
	if (name == "SCHEDULE_HOSTGROUP_HOST_DOWNTIME")
		return 84;
	if (name == "SCHEDULE_HOSTGROUP_SVC_DOWNTIME")
		return 85;
	if (name == "SCHEDULE_HOST_SVC_DOWNTIME")
		return 86;
	if (name == "PROCESS_HOST_CHECK_RESULT")
		return 87;
	if (name == "START_EXECUTING_HOST_CHECKS")
		return 88;
	if (name == "STOP_EXECUTING_HOST_CHECKS")
		return 89;
	if (name == "START_ACCEPTING_PASSIVE_HOST_CHECKS")
		return 90;
	if (name == "STOP_ACCEPTING_PASSIVE_HOST_CHECKS")
		return 91;
	if (name == "ENABLE_PASSIVE_HOST_CHECKS")
		return 92;
	if (name == "DISABLE_PASSIVE_HOST_CHECKS")
		return 93;
	if (name == "START_OBSESSING_OVER_HOST_CHECKS")
		return 94;
	if (name == "STOP_OBSESSING_OVER_HOST_CHECKS")
		return 95;
	if (name == "SCHEDULE_HOST_CHECK")
		return 96;
	if (name == "SCHEDULE_FORCED_HOST_CHECK")
		return 98;
	if (name == "START_OBSESSING_OVER_SVC")
		return 99;
	if (name == "STOP_OBSESSING_OVER_SVC")
		return 100;
	if (name == "START_OBSESSING_OVER_HOST")
		return 101;
	if (name == "STOP_OBSESSING_OVER_HOST")
		return 102;
	if (name == "ENABLE_HOSTGROUP_HOST_CHECKS")
		return 103;
	if (name == "DISABLE_HOSTGROUP_HOST_CHECKS")
		return 104;
	if (name == "ENABLE_HOSTGROUP_PASSIVE_SVC_CHECKS")
		return 105;
	if (name == "DISABLE_HOSTGROUP_PASSIVE_SVC_CHECKS")
		return 106;
	if (name == "ENABLE_HOSTGROUP_PASSIVE_HOST_CHECKS")
		return 107;
	if (name == "DISABLE_HOSTGROUP_PASSIVE_HOST_CHECKS")
		return 108;
	if (name == "ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS")
		return 109;
	if (name == "DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS")
		return 110;
	if (name == "ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS")
		return 111;
	if (name == "DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS")
		return 112;
	if (name == "ENABLE_SERVICEGROUP_SVC_CHECKS")
		return 113;
	if (name == "DISABLE_SERVICEGROUP_SVC_CHECKS")
		return 114;
	if (name == "ENABLE_SERVICEGROUP_HOST_CHECKS")
		return 115;
	if (name == "DISABLE_SERVICEGROUP_HOST_CHECKS")
		return 116;
	if (name == "ENABLE_SERVICEGROUP_PASSIVE_SVC_CHECKS")
		return 117;
	if (name == "DISABLE_SERVICEGROUP_PASSIVE_SVC_CHECKS")
		return 118;
	if (name == "ENABLE_SERVICEGROUP_PASSIVE_HOST_CHECKS")
		return 119;
	if (name == "DISABLE_SERVICEGROUP_PASSIVE_HOST_CHECKS")
		return 120;
	if (name == "SCHEDULE_SERVICEGROUP_HOST_DOWNTIME")
		return 121;
	if (name == "SCHEDULE_SERVICEGROUP_SVC_DOWNTIME")
		return 122;
	if (name == "CHANGE_GLOBAL_HOST_EVENT_HANDLER")
		return 123;
	if (name == "CHANGE_GLOBAL_SVC_EVENT_HANDLER")
		return 124;
	if (name == "CHANGE_HOST_EVENT_HANDLER")
		return 125;
	if (name == "CHANGE_SVC_EVENT_HANDLER")
		return 126;
	if (name == "CHANGE_HOST_CHECK_COMMAND")
		return 127;
	if (name == "CHANGE_SVC_CHECK_COMMAND")
		return 128;
	if (name == "CHANGE_NORMAL_HOST_CHECK_INTERVAL")
		return 129;
	if (name == "CHANGE_NORMAL_SVC_CHECK_INTERVAL")
		return 130;
	if (name == "CHANGE_RETRY_SVC_CHECK_INTERVAL")
		return 131;
	if (name == "CHANGE_MAX_HOST_CHECK_ATTEMPTS")
		return 132;
	if (name == "CHANGE_MAX_SVC_CHECK_ATTEMPTS")
		return 133;
	if (name == "SCHEDULE_AND_PROPAGATE_TRIGGERED_HOST_DOWNTIME")
		return 134;
	if (name == "ENABLE_HOST_AND_CHILD_NOTIFICATIONS")
		return 135;
	if (name == "DISABLE_HOST_AND_CHILD_NOTIFICATIONS")
		return 136;
	if (name == "SCHEDULE_AND_PROPAGATE_HOST_DOWNTIME")
		return 137;
	if (name == "ENABLE_SERVICE_FRESHNESS_CHECKS")
		return 138;
	if (name == "DISABLE_SERVICE_FRESHNESS_CHECKS")
		return 139;
	if (name == "ENABLE_HOST_FRESHNESS_CHECKS")
		return 140;
	if (name == "DISABLE_HOST_FRESHNESS_CHECKS")
		return 141;
	if (name == "SET_HOST_NOTIFICATION_NUMBER")
		return 142;
	if (name == "SET_SVC_NOTIFICATION_NUMBER")
		return 143;
	if (name == "CHANGE_HOST_CHECK_TIMEPERIOD")
		return 144;
	if (name == "CHANGE_SVC_CHECK_TIMEPERIOD")
		return 145;
	if (name == "PROCESS_FILE")
		return 146;
	if (name == "CHANGE_CUSTOM_HOST_VAR")
		return 147;
	if (name == "CHANGE_CUSTOM_SVC_VAR")
		return 148;
	if (name == "CHANGE_CUSTOM_CONTACT_VAR")
		return 149;
	if (name == "ENABLE_CONTACT_HOST_NOTIFICATIONS")
		return 150;
	if (name == "DISABLE_CONTACT_HOST_NOTIFICATIONS")
		return 151;
	if (name == "ENABLE_CONTACT_SVC_NOTIFICATIONS")
		return 152;
	if (name == "DISABLE_CONTACT_SVC_NOTIFICATIONS")
		return 153;
	if (name == "ENABLE_CONTACTGROUP_HOST_NOTIFICATIONS")
		return 154;
	if (name == "DISABLE_CONTACTGROUP_HOST_NOTIFICATIONS")
		return 155;
	if (name == "ENABLE_CONTACTGROUP_SVC_NOTIFICATIONS")
		return 156;
	if (name == "DISABLE_CONTACTGROUP_SVC_NOTIFICATIONS")
		return 157;
	if (name == "CHANGE_RETRY_HOST_CHECK_INTERVAL")
		return 158;
	if (name == "SEND_CUSTOM_HOST_NOTIFICATION")
		return 159;
	if (name == "SEND_CUSTOM_SVC_NOTIFICATION")
		return 160;
	if (name == "CHANGE_HOST_NOTIFICATION_TIMEPERIOD")
		return 161;
	if (name == "CHANGE_SVC_NOTIFICATION_TIMEPERIOD")
		return 162;
	if (name == "CHANGE_CONTACT_HOST_NOTIFICATION_TIMEPERIOD")
		return 163;
	if (name == "CHANGE_CONTACT_SVC_NOTIFICATION_TIMEPERIOD")
		return 164;
	if (name == "CHANGE_HOST_MODATTR")
		return 165;
	if (name == "CHANGE_SVC_MODATTR")
		return 166;
	if (name == "CHANGE_CONTACT_MODATTR")
		return 167;
	if (name == "CHANGE_CONTACT_MODHATTR")
		return 168;
	if (name == "CHANGE_CONTACT_MODSATTR")
		return 169;
	if (name == "SYNC_STATE_INFORMATION")
		return 170;
	if (name == "DEL_DOWNTIME_BY_HOST_NAME")
		return 171;
	if (name == "DEL_DOWNTIME_BY_HOSTGROUP_NAME")
		return 172;
	if (name == "DEL_DOWNTIME_BY_START_TIME_COMMENT")
		return 173;
	if (name == "ACKNOWLEDGE_HOST_PROBLEM_EXPIRE")
		return 174;
	if (name == "ACKNOWLEDGE_SVC_PROBLEM_EXPIRE")
		return 175;
	if (name == "DISABLE_NOTIFICATIONS_EXPIRE_TIME")
		return 176;
	if (name == "CUSTOM_COMMAND")
		return 999;

	return 0;
}
