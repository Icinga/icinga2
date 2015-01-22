/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "db_ido/hostdbobject.hpp"
#include "db_ido/dbtype.hpp"
#include "db_ido/dbvalue.hpp"
#include "db_ido/dbevents.hpp"
#include "icinga/host.hpp"
#include "icinga/service.hpp"
#include "icinga/notification.hpp"
#include "icinga/dependency.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/compatutility.hpp"
#include "base/convert.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_DBTYPE(Host, "host", DbObjectTypeHost, "host_object_id", HostDbObject);

HostDbObject::HostDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr HostDbObject::GetConfigFields(void) const
{
	Dictionary::Ptr fields = new Dictionary();
	Host::Ptr host = static_pointer_cast<Host>(GetObject());

	fields->Set("alias", CompatUtility::GetHostAlias(host));
	fields->Set("display_name", host->GetDisplayName());
	fields->Set("address", host->GetAddress());
	fields->Set("address6", host->GetAddress6());

	fields->Set("check_command_object_id", host->GetCheckCommand());
	fields->Set("check_command_args", CompatUtility::GetCheckableCommandArgs(host));
	fields->Set("eventhandler_command_object_id", host->GetEventCommand());
	fields->Set("eventhandler_command_args", Empty);
	fields->Set("notification_timeperiod_object_id", Notification::GetByName(CompatUtility::GetCheckableNotificationNotificationPeriod(host)));
	fields->Set("check_timeperiod_object_id", host->GetCheckPeriod());
	fields->Set("failure_prediction_options", Empty);
	fields->Set("check_interval", CompatUtility::GetCheckableCheckInterval(host));
	fields->Set("retry_interval", CompatUtility::GetCheckableRetryInterval(host));
	fields->Set("max_check_attempts", host->GetMaxCheckAttempts());

	fields->Set("first_notification_delay", Empty);

	fields->Set("notification_interval", CompatUtility::GetCheckableNotificationNotificationInterval(host));
	fields->Set("notify_on_down", CompatUtility::GetHostNotifyOnDown(host));
	fields->Set("notify_on_unreachable", CompatUtility::GetHostNotifyOnDown(host));

	fields->Set("notify_on_recovery", CompatUtility::GetCheckableNotifyOnRecovery(host));
	fields->Set("notify_on_flapping", CompatUtility::GetCheckableNotifyOnFlapping(host));
	fields->Set("notify_on_downtime", CompatUtility::GetCheckableNotifyOnDowntime(host));

	fields->Set("stalk_on_up", Empty);
	fields->Set("stalk_on_down", Empty);
	fields->Set("stalk_on_unreachable", Empty);

	fields->Set("flap_detection_enabled", CompatUtility::GetCheckableFlapDetectionEnabled(host));
	fields->Set("flap_detection_on_up", Empty);
	fields->Set("flap_detection_on_down", Empty);
	fields->Set("flap_detection_on_unreachable", Empty);
	fields->Set("low_flap_threshold", CompatUtility::GetCheckableLowFlapThreshold(host));
	fields->Set("high_flap_threshold", CompatUtility::GetCheckableHighFlapThreshold(host));

	fields->Set("process_performance_data", 0);

	fields->Set("freshness_checks_enabled", CompatUtility::GetCheckableFreshnessChecksEnabled(host));
	fields->Set("freshness_threshold", CompatUtility::GetCheckableFreshnessThreshold(host));
	fields->Set("passive_checks_enabled", CompatUtility::GetCheckablePassiveChecksEnabled(host));
	fields->Set("event_handler_enabled", CompatUtility::GetCheckableEventHandlerEnabled(host));
	fields->Set("active_checks_enabled", CompatUtility::GetCheckableActiveChecksEnabled(host));

	fields->Set("retain_status_information", 1);
	fields->Set("retain_nonstatus_information", 1);

	fields->Set("notifications_enabled", CompatUtility::GetCheckableNotificationsEnabled(host));

	fields->Set("obsess_over_host", 0);
	fields->Set("failure_prediction_enabled", 0);

	fields->Set("notes", host->GetNotes());
	fields->Set("notes_url", host->GetNotesUrl());
	fields->Set("action_url", host->GetActionUrl());
	fields->Set("icon_image", host->GetIconImage());
	fields->Set("icon_image_alt", host->GetIconImageAlt());

	return fields;
}

Dictionary::Ptr HostDbObject::GetStatusFields(void) const
{
	Dictionary::Ptr fields = new Dictionary();
	Host::Ptr host = static_pointer_cast<Host>(GetObject());

	CheckResult::Ptr cr = host->GetLastCheckResult();

	if (cr) {
		fields->Set("output", CompatUtility::GetCheckResultOutput(cr));
		fields->Set("long_output", CompatUtility::GetCheckResultLongOutput(cr));
		fields->Set("perfdata", CompatUtility::GetCheckResultPerfdata(cr));
		fields->Set("check_source", cr->GetCheckSource());
	}

	fields->Set("current_state", host->IsReachable() ? host->GetState() : 2);
	fields->Set("has_been_checked", CompatUtility::GetCheckableHasBeenChecked(host));
	fields->Set("should_be_scheduled", host->GetEnableActiveChecks());
	fields->Set("current_check_attempt", host->GetCheckAttempt());
	fields->Set("max_check_attempts", host->GetMaxCheckAttempts());

	if (cr)
		fields->Set("last_check", DbValue::FromTimestamp(cr->GetScheduleEnd()));

	fields->Set("next_check", DbValue::FromTimestamp(host->GetNextCheck()));
	fields->Set("check_type", CompatUtility::GetCheckableCheckType(host));
	fields->Set("last_state_change", DbValue::FromTimestamp(host->GetLastStateChange()));
	fields->Set("last_hard_state_change", DbValue::FromTimestamp(host->GetLastHardStateChange()));
	fields->Set("last_hard_state", host->GetLastHardState());
	fields->Set("last_time_up", DbValue::FromTimestamp(static_cast<int>(host->GetLastStateUp())));
	fields->Set("last_time_down", DbValue::FromTimestamp(static_cast<int>(host->GetLastStateDown())));
	fields->Set("last_time_unreachable", DbValue::FromTimestamp(static_cast<int>(host->GetLastStateUnreachable())));
	fields->Set("state_type", host->GetStateType());
	fields->Set("last_notification", DbValue::FromTimestamp(CompatUtility::GetCheckableNotificationLastNotification(host)));
	fields->Set("next_notification", DbValue::FromTimestamp(CompatUtility::GetCheckableNotificationNextNotification(host)));
	fields->Set("no_more_notifications", Empty);
	fields->Set("notifications_enabled", CompatUtility::GetCheckableNotificationsEnabled(host));
	{
		ObjectLock olock(host);
		fields->Set("problem_has_been_acknowledged", CompatUtility::GetCheckableProblemHasBeenAcknowledged(host));
		fields->Set("acknowledgement_type", CompatUtility::GetCheckableAcknowledgementType(host));
	}
	fields->Set("current_notification_number", CompatUtility::GetCheckableNotificationNotificationNumber(host));
	fields->Set("passive_checks_enabled", CompatUtility::GetCheckablePassiveChecksEnabled(host));
	fields->Set("active_checks_enabled", CompatUtility::GetCheckableActiveChecksEnabled(host));
	fields->Set("event_handler_enabled", CompatUtility::GetCheckableEventHandlerEnabled(host));
	fields->Set("flap_detection_enabled", CompatUtility::GetCheckableFlapDetectionEnabled(host));
	fields->Set("is_flapping", CompatUtility::GetCheckableIsFlapping(host));
	fields->Set("percent_state_change", CompatUtility::GetCheckablePercentStateChange(host));

	if (cr) {
		fields->Set("latency", Convert::ToString(Service::CalculateLatency(cr)));
		fields->Set("execution_time", Convert::ToString(Service::CalculateExecutionTime(cr)));
	}

	fields->Set("scheduled_downtime_depth", host->GetDowntimeDepth());
	fields->Set("failure_prediction_enabled", Empty);
	fields->Set("process_performance_data", 0); /* this is a host which does not process any perf data */
	fields->Set("obsess_over_host", Empty);
	fields->Set("modified_host_attributes", host->GetModifiedAttributes());
	fields->Set("event_handler", CompatUtility::GetCheckableEventHandler(host));
	fields->Set("check_command", CompatUtility::GetCheckableCheckCommand(host));
	fields->Set("normal_check_interval", CompatUtility::GetCheckableCheckInterval(host));
	fields->Set("retry_check_interval", CompatUtility::GetCheckableRetryInterval(host));
	fields->Set("check_timeperiod_object_id", host->GetCheckPeriod());
	fields->Set("is_reachable", CompatUtility::GetCheckableIsReachable(host));

	return fields;
}

void HostDbObject::OnConfigUpdate(void)
{
	Host::Ptr host = static_pointer_cast<Host>(GetObject());

	/* parents */
	BOOST_FOREACH(const Checkable::Ptr& checkable, host->GetParents()) {
		Host::Ptr parent = dynamic_pointer_cast<Host>(checkable);

		if (!parent)
			continue;

		Log(LogDebug, "HostDbObject")
		    << "host parents: " << parent->GetName();

		/* parents: host_id, parent_host_object_id */
		Dictionary::Ptr fields1 = new Dictionary();
		fields1->Set(GetType()->GetTable() + "_id", DbValue::FromObjectInsertID(GetObject()));
		fields1->Set("parent_host_object_id", parent);
		fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

		DbQuery query1;
		query1.Table = GetType()->GetTable() + "_parenthosts";
		query1.Type = DbQueryInsert;
		query1.Category = DbCatConfig;
		query1.Fields = fields1;
		OnQuery(query1);
	}

	/* host dependencies */
	Log(LogDebug, "HostDbObject")
	    << "host dependencies for '" << host->GetName() << "'";

	BOOST_FOREACH(const Dependency::Ptr& dep, host->GetDependencies()) {
		Checkable::Ptr parent = dep->GetParent();

		if (!parent) {
			Log(LogDebug, "HostDbObject")
			    << "Missing parent for dependency '" << dep->GetName() << "'.";
			continue;
		}

		int state_filter = dep->GetStateFilter();

		Log(LogDebug, "HostDbObject")
		    << "parent host: " << parent->GetName();

		Dictionary::Ptr fields2 = new Dictionary();
		fields2->Set("host_object_id", parent);
		fields2->Set("dependent_host_object_id", host);
		fields2->Set("inherits_parent", 1);
		fields2->Set("timeperiod_object_id", dep->GetPeriod());
		fields2->Set("fail_on_up", (state_filter & StateFilterUp) ? 1 : 0);
		fields2->Set("fail_on_down", (state_filter & StateFilterDown) ? 1 : 0);
		fields2->Set("instance_id", 0); /* DbConnection class fills in real ID */

		DbQuery query2;
		query2.Table = GetType()->GetTable() + "dependencies";
		query2.Type = DbQueryInsert;
		query2.Category = DbCatConfig;
		query2.Fields = fields2;
		OnQuery(query2);
	}

	Log(LogDebug, "HostDbObject")
	    << "host contacts: " << host->GetName();

	BOOST_FOREACH(const User::Ptr& user, CompatUtility::GetCheckableNotificationUsers(host)) {
		Log(LogDebug, "HostDbObject")
		    << "host contacts: " << user->GetName();

		Dictionary::Ptr fields_contact = new Dictionary();
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

	Log(LogDebug, "HostDbObject")
	    << "host contactgroups: " << host->GetName();

	BOOST_FOREACH(const UserGroup::Ptr& usergroup, CompatUtility::GetCheckableNotificationUserGroups(host)) {
		Log(LogDebug, "HostDbObject")
		    << "host contactgroups: " << usergroup->GetName();

		Dictionary::Ptr fields_contact = new Dictionary();
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

	/* update comments and downtimes on config change */
	DbEvents::AddComments(host);
	DbEvents::AddDowntimes(host);
}

void HostDbObject::OnStatusUpdate(void)
{
}
