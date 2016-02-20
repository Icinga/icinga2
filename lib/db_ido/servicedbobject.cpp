/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "db_ido/servicedbobject.hpp"
#include "db_ido/servicegroupdbobject.hpp"
#include "db_ido/dbtype.hpp"
#include "db_ido/dbvalue.hpp"
#include "db_ido/dbevents.hpp"
#include "icinga/notification.hpp"
#include "icinga/dependency.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/externalcommandprocessor.hpp"
#include "icinga/compatutility.hpp"
#include "icinga/icingaapplication.hpp"
#include "remote/endpoint.hpp"
#include "base/convert.hpp"
#include "base/objectlock.hpp"
#include "base/initialize.hpp"
#include "base/configtype.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include "base/json.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>

using namespace icinga;

REGISTER_DBTYPE(Service, "service", DbObjectTypeService, "service_object_id", ServiceDbObject);

ServiceDbObject::ServiceDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr ServiceDbObject::GetConfigFields(void) const
{
	Dictionary::Ptr fields = new Dictionary();
	Service::Ptr service = static_pointer_cast<Service>(GetObject());
	Host::Ptr host = service->GetHost();

	fields->Set("host_object_id", host);
	fields->Set("display_name", service->GetDisplayName());
	fields->Set("check_command_object_id", service->GetCheckCommand());
	fields->Set("check_command_args", CompatUtility::GetCheckableCommandArgs(service));
	fields->Set("eventhandler_command_object_id", service->GetEventCommand());
	fields->Set("eventhandler_command_args", Empty);
	fields->Set("notification_timeperiod_object_id", Notification::GetByName(CompatUtility::GetCheckableNotificationNotificationPeriod(service)));
	fields->Set("check_timeperiod_object_id", service->GetCheckPeriod());
	fields->Set("failure_prediction_options", Empty);
	fields->Set("check_interval", CompatUtility::GetCheckableCheckInterval(service));
	fields->Set("retry_interval", CompatUtility::GetCheckableRetryInterval(service));
	fields->Set("max_check_attempts", service->GetMaxCheckAttempts());
	fields->Set("first_notification_delay", Empty);
	fields->Set("notification_interval", CompatUtility::GetCheckableNotificationNotificationInterval(service));
	fields->Set("notify_on_warning", CompatUtility::GetCheckableNotifyOnWarning(service));
	fields->Set("notify_on_unknown", CompatUtility::GetCheckableNotifyOnUnknown(service));
	fields->Set("notify_on_critical", CompatUtility::GetCheckableNotifyOnCritical(service));
	fields->Set("notify_on_recovery", CompatUtility::GetCheckableNotifyOnRecovery(service));
	fields->Set("notify_on_flapping", CompatUtility::GetCheckableNotifyOnFlapping(service));
	fields->Set("notify_on_downtime", CompatUtility::GetCheckableNotifyOnDowntime(service));
	fields->Set("stalk_on_ok", 0);
	fields->Set("stalk_on_warning", 0);
	fields->Set("stalk_on_unknown", 0);
	fields->Set("stalk_on_critical", 0);
	fields->Set("is_volatile", CompatUtility::GetCheckableIsVolatile(service));
	fields->Set("flap_detection_enabled", CompatUtility::GetCheckableFlapDetectionEnabled(service));
	fields->Set("flap_detection_on_ok", Empty);
	fields->Set("flap_detection_on_warning", Empty);
	fields->Set("flap_detection_on_unknown", Empty);
	fields->Set("flap_detection_on_critical", Empty);
	fields->Set("low_flap_threshold", CompatUtility::GetCheckableLowFlapThreshold(service));
	fields->Set("high_flap_threshold", CompatUtility::GetCheckableHighFlapThreshold(service));
	fields->Set("process_performance_data", CompatUtility::GetCheckableProcessPerformanceData(service));
	fields->Set("freshness_checks_enabled", CompatUtility::GetCheckableFreshnessChecksEnabled(service));
	fields->Set("freshness_threshold", CompatUtility::GetCheckableFreshnessThreshold(service));
	fields->Set("passive_checks_enabled", CompatUtility::GetCheckablePassiveChecksEnabled(service));
	fields->Set("event_handler_enabled", CompatUtility::GetCheckableEventHandlerEnabled(service));
	fields->Set("active_checks_enabled", CompatUtility::GetCheckableActiveChecksEnabled(service));
	fields->Set("retain_status_information", Empty);
	fields->Set("retain_nonstatus_information", Empty);
	fields->Set("notifications_enabled", CompatUtility::GetCheckableNotificationsEnabled(service));
	fields->Set("obsess_over_service", Empty);
	fields->Set("failure_prediction_enabled", Empty);
	fields->Set("notes", service->GetNotes());
	fields->Set("notes_url", service->GetNotesUrl());
	fields->Set("action_url", service->GetActionUrl());
	fields->Set("icon_image", service->GetIconImage());
	fields->Set("icon_image_alt", service->GetIconImageAlt());

	return fields;
}

Dictionary::Ptr ServiceDbObject::GetStatusFields(void) const
{
	Dictionary::Ptr fields = new Dictionary();
	Service::Ptr service = static_pointer_cast<Service>(GetObject());
	CheckResult::Ptr cr = service->GetLastCheckResult();

	if (cr) {
		fields->Set("output", CompatUtility::GetCheckResultOutput(cr));
		fields->Set("long_output", CompatUtility::GetCheckResultLongOutput(cr));
		fields->Set("perfdata", CompatUtility::GetCheckResultPerfdata(cr));
		fields->Set("check_source", cr->GetCheckSource());
	}

	fields->Set("current_state", service->GetState());
	fields->Set("has_been_checked", CompatUtility::GetCheckableHasBeenChecked(service));
	fields->Set("should_be_scheduled", service->GetEnableActiveChecks());
	fields->Set("current_check_attempt", service->GetCheckAttempt());
	fields->Set("max_check_attempts", service->GetMaxCheckAttempts());

	if (cr)
		fields->Set("last_check", DbValue::FromTimestamp(cr->GetScheduleEnd()));

	fields->Set("next_check", DbValue::FromTimestamp(service->GetNextCheck()));
	fields->Set("check_type", CompatUtility::GetCheckableCheckType(service));
	fields->Set("last_state_change", DbValue::FromTimestamp(service->GetLastStateChange()));
	fields->Set("last_hard_state_change", DbValue::FromTimestamp(service->GetLastHardStateChange()));
	fields->Set("last_hard_state", service->GetLastHardState());
	fields->Set("last_time_ok", DbValue::FromTimestamp(static_cast<int>(service->GetLastStateOK())));
	fields->Set("last_time_warning", DbValue::FromTimestamp(static_cast<int>(service->GetLastStateWarning())));
	fields->Set("last_time_critical", DbValue::FromTimestamp(static_cast<int>(service->GetLastStateCritical())));
	fields->Set("last_time_unknown", DbValue::FromTimestamp(static_cast<int>(service->GetLastStateUnknown())));
	fields->Set("state_type", service->GetStateType());
	fields->Set("last_notification", DbValue::FromTimestamp(CompatUtility::GetCheckableNotificationLastNotification(service)));
	fields->Set("next_notification", DbValue::FromTimestamp(CompatUtility::GetCheckableNotificationNextNotification(service)));
	fields->Set("no_more_notifications", Empty);
	fields->Set("notifications_enabled", CompatUtility::GetCheckableNotificationsEnabled(service));
	fields->Set("problem_has_been_acknowledged", CompatUtility::GetCheckableProblemHasBeenAcknowledged(service));
	fields->Set("acknowledgement_type", CompatUtility::GetCheckableAcknowledgementType(service));
	fields->Set("current_notification_number", CompatUtility::GetCheckableNotificationNotificationNumber(service));
	fields->Set("passive_checks_enabled", CompatUtility::GetCheckablePassiveChecksEnabled(service));
	fields->Set("active_checks_enabled", CompatUtility::GetCheckableActiveChecksEnabled(service));
	fields->Set("event_handler_enabled", CompatUtility::GetCheckableEventHandlerEnabled(service));
	fields->Set("flap_detection_enabled", CompatUtility::GetCheckableFlapDetectionEnabled(service));
	fields->Set("is_flapping", CompatUtility::GetCheckableIsFlapping(service));
	fields->Set("percent_state_change", CompatUtility::GetCheckablePercentStateChange(service));

	if (cr) {
		fields->Set("latency", Convert::ToString(Service::CalculateLatency(cr)));
		fields->Set("execution_time", Convert::ToString(Service::CalculateExecutionTime(cr)));
	}

	fields->Set("scheduled_downtime_depth", service->GetDowntimeDepth());
	fields->Set("process_performance_data", CompatUtility::GetCheckableProcessPerformanceData(service));
	fields->Set("event_handler", CompatUtility::GetCheckableEventHandler(service));
	fields->Set("check_command", CompatUtility::GetCheckableCheckCommand(service));
	fields->Set("normal_check_interval", CompatUtility::GetCheckableCheckInterval(service));
	fields->Set("retry_check_interval", CompatUtility::GetCheckableRetryInterval(service));
	fields->Set("check_timeperiod_object_id", service->GetCheckPeriod());
	fields->Set("is_reachable", CompatUtility::GetCheckableIsReachable(service));

	fields->Set("original_attributes", JsonEncode(service->GetOriginalAttributes()));

	return fields;
}

bool ServiceDbObject::IsStatusAttribute(const String& attribute) const
{
	return (attribute == "last_result");
}

void ServiceDbObject::OnConfigUpdate(void)
{
	Service::Ptr service = static_pointer_cast<Service>(GetObject());

	/* groups */
	Array::Ptr groups = service->GetGroups();

	if (groups) {
		ObjectLock olock(groups);
		BOOST_FOREACH(const String& groupName, groups) {
			ServiceGroup::Ptr group = ServiceGroup::GetByName(groupName);

			std::vector<DbQuery> queries;

			DbQuery query1;
			query1.Table = DbType::GetByName("ServiceGroup")->GetTable() + "_members";
			query1.Type = DbQueryDelete;
			query1.Category = DbCatConfig;
			query1.WhereCriteria = new Dictionary();
			query1.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */
			query1.WhereCriteria->Set("service_object_id", service);
			queries.push_back(query1);

			DbQuery query2;
			query2.Table = DbType::GetByName("ServiceGroup")->GetTable() + "_members";
			query2.Type = DbQueryInsert;
			query2.Category = DbCatConfig;
			query2.Fields = new Dictionary();
			query2.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */
			query2.Fields->Set("servicegroup_id", DbValue::FromObjectInsertID(group));
			query2.Fields->Set("service_object_id", service);
			queries.push_back(query2);

			DbObject::OnMultipleQueries(queries);
		}
	}

	/* service dependencies */
	Log(LogDebug, "ServiceDbObject")
	    << "service dependencies for '" << service->GetName() << "'";

	BOOST_FOREACH(const Dependency::Ptr& dep, service->GetDependencies()) {
		Checkable::Ptr parent = dep->GetParent();

		if (!parent) {
			Log(LogDebug, "ServiceDbObject")
			    << "Missing parent for dependency '" << dep->GetName() << "'.";
			continue;
		}

		Log(LogDebug, "ServiceDbObject")
		    << "service parents: " << parent->GetName();

		int state_filter = dep->GetStateFilter();

		/* service dependencies */
		Dictionary::Ptr fields1 = new Dictionary();
		fields1->Set("service_object_id", parent);
		fields1->Set("dependent_service_object_id", service);
		fields1->Set("inherits_parent", 1);
		fields1->Set("timeperiod_object_id", dep->GetPeriod());
		fields1->Set("fail_on_ok", (state_filter & StateFilterOK) ? 1 : 0);
		fields1->Set("fail_on_warning", (state_filter & StateFilterWarning) ? 1 : 0);
		fields1->Set("fail_on_critical", (state_filter & StateFilterCritical) ? 1 : 0);
		fields1->Set("fail_on_unknown", (state_filter & StateFilterUnknown) ? 1 : 0);
		fields1->Set("instance_id", 0); /* DbConnection class fills in real ID */

		DbQuery query1;
		query1.Table = GetType()->GetTable() + "dependencies";
		query1.Type = DbQueryInsert;
		query1.Category = DbCatConfig;
		query1.Fields = fields1;
		OnQuery(query1);
	}

	/* service contacts, contactgroups */
	Log(LogDebug, "ServiceDbObject")
	    << "service contacts: " << service->GetName();

	BOOST_FOREACH(const User::Ptr& user, CompatUtility::GetCheckableNotificationUsers(service)) {
		Log(LogDebug, "ServiceDbObject")
		    << "service contacts: " << user->GetName();

		Dictionary::Ptr fields_contact = new Dictionary();
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

	Log(LogDebug, "ServiceDbObject")
	    << "service contactgroups: " << service->GetName();

	BOOST_FOREACH(const UserGroup::Ptr& usergroup, CompatUtility::GetCheckableNotificationUserGroups(service)) {
		Log(LogDebug, "ServiceDbObject")
		    << "service contactgroups: " << usergroup->GetName();

		Dictionary::Ptr fields_contact = new Dictionary();
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

	/* update comments and downtimes on config change */
	DbEvents::AddComments(service);
	DbEvents::AddDowntimes(service);
}

void ServiceDbObject::OnStatusUpdate(void)
{
}
