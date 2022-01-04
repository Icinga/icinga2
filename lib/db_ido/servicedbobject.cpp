/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
#include "icinga/pluginutility.hpp"
#include "icinga/icingaapplication.hpp"
#include "remote/endpoint.hpp"
#include "base/convert.hpp"
#include "base/objectlock.hpp"
#include "base/initialize.hpp"
#include "base/configtype.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include "base/json.hpp"
#include <boost/algorithm/string/join.hpp>

using namespace icinga;

REGISTER_DBTYPE(Service, "service", DbObjectTypeService, "service_object_id", ServiceDbObject);

ServiceDbObject::ServiceDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr ServiceDbObject::GetConfigFields() const
{
	Service::Ptr service = static_pointer_cast<Service>(GetObject());
	Host::Ptr host = service->GetHost();

	unsigned long notificationStateFilter = CompatUtility::GetCheckableNotificationTypeFilter(service);
	unsigned long notificationTypeFilter = CompatUtility::GetCheckableNotificationTypeFilter(service);

	return new Dictionary({
		{ "host_object_id", host },
		{ "display_name", service->GetDisplayName() },
		{ "check_command_object_id", service->GetCheckCommand() },
		{ "eventhandler_command_object_id", service->GetEventCommand() },
		{ "check_timeperiod_object_id", service->GetCheckPeriod() },
		{ "check_interval", service->GetCheckInterval() / 60.0 },
		{ "retry_interval", service->GetRetryInterval() / 60.0 },
		{ "max_check_attempts", service->GetMaxCheckAttempts() },
		{ "is_volatile", service->GetVolatile() },
		{ "flap_detection_enabled", service->GetEnableFlapping() },
		{ "low_flap_threshold", service->GetFlappingThresholdLow() },
		{ "high_flap_threshold", service->GetFlappingThresholdLow() },
		{ "process_performance_data", service->GetEnablePerfdata() },
		{ "freshness_checks_enabled", 1 },
		{ "freshness_threshold", Convert::ToLong(service->GetCheckInterval()) },
		{ "event_handler_enabled", service->GetEnableEventHandler() },
		{ "passive_checks_enabled", service->GetEnablePassiveChecks() },
		{ "active_checks_enabled", service->GetEnableActiveChecks() },
		{ "notifications_enabled", service->GetEnableNotifications() },
		{ "notes", service->GetNotes() },
		{ "notes_url", service->GetNotesUrl() },
		{ "action_url", service->GetActionUrl() },
		{ "icon_image", service->GetIconImage() },
		{ "icon_image_alt", service->GetIconImageAlt() },
		{ "notification_interval", CompatUtility::GetCheckableNotificationNotificationInterval(service) },
		{ "notify_on_warning", (notificationStateFilter & ServiceWarning) ? 1 : 0 },
		{ "notify_on_unknown", (notificationStateFilter & ServiceUnknown) ? 1 : 0 },
		{ "notify_on_critical", (notificationStateFilter & ServiceCritical) ? 1 : 0 },
		{ "notify_on_recovery", (notificationTypeFilter & NotificationRecovery) ? 1 : 0 },
		{ "notify_on_flapping", (notificationTypeFilter & (NotificationFlappingStart | NotificationFlappingEnd)) ? 1 : 0 },
		{ "notify_on_downtime", (notificationTypeFilter & (NotificationDowntimeStart | NotificationDowntimeEnd | NotificationDowntimeRemoved)) ? 1 : 0 }
	});
}

Dictionary::Ptr ServiceDbObject::GetStatusFields() const
{
	Dictionary::Ptr fields = new Dictionary();
	Service::Ptr service = static_pointer_cast<Service>(GetObject());
	CheckResult::Ptr cr = service->GetLastCheckResult();

	if (cr) {
		fields->Set("output", CompatUtility::GetCheckResultOutput(cr));
		fields->Set("long_output", CompatUtility::GetCheckResultLongOutput(cr));
		fields->Set("perfdata", PluginUtility::FormatPerfdata(cr->GetPerformanceData()));
		fields->Set("check_source", cr->GetCheckSource());
		fields->Set("latency", cr->CalculateLatency());
		fields->Set("execution_time", cr->CalculateExecutionTime());
	}

	fields->Set("current_state", service->GetState());
	fields->Set("has_been_checked", service->HasBeenChecked());
	fields->Set("should_be_scheduled", service->GetEnableActiveChecks());
	fields->Set("current_check_attempt", service->GetCheckAttempt());
	fields->Set("max_check_attempts", service->GetMaxCheckAttempts());
	fields->Set("last_check", DbValue::FromTimestamp(service->GetLastCheck()));
	fields->Set("next_check", DbValue::FromTimestamp(service->GetNextCheck()));
	fields->Set("check_type", !service->GetEnableActiveChecks()); /* 0 .. active, 1 .. passive */
	fields->Set("last_state_change", DbValue::FromTimestamp(service->GetLastStateChange()));
	fields->Set("last_hard_state_change", DbValue::FromTimestamp(service->GetLastHardStateChange()));
	fields->Set("last_hard_state", service->GetLastHardState());
	fields->Set("last_time_ok", DbValue::FromTimestamp(service->GetLastStateOK()));
	fields->Set("last_time_warning", DbValue::FromTimestamp(service->GetLastStateWarning()));
	fields->Set("last_time_critical", DbValue::FromTimestamp(service->GetLastStateCritical()));
	fields->Set("last_time_unknown", DbValue::FromTimestamp(service->GetLastStateUnknown()));
	fields->Set("state_type", service->GetStateType());
	fields->Set("notifications_enabled", service->GetEnableNotifications());
	fields->Set("problem_has_been_acknowledged", service->GetAcknowledgement() != AcknowledgementNone);
	fields->Set("acknowledgement_type", service->GetAcknowledgement());
	fields->Set("passive_checks_enabled", service->GetEnablePassiveChecks());
	fields->Set("active_checks_enabled", service->GetEnableActiveChecks());
	fields->Set("event_handler_enabled", service->GetEnableEventHandler());
	fields->Set("flap_detection_enabled", service->GetEnableFlapping());
	fields->Set("is_flapping", service->IsFlapping());
	fields->Set("percent_state_change", service->GetFlappingCurrent());
	fields->Set("scheduled_downtime_depth", service->GetDowntimeDepth());
	fields->Set("process_performance_data", service->GetEnablePerfdata());
	fields->Set("normal_check_interval", service->GetCheckInterval() / 60.0);
	fields->Set("retry_check_interval", service->GetRetryInterval() / 60.0);
	fields->Set("check_timeperiod_object_id", service->GetCheckPeriod());
	fields->Set("is_reachable", service->GetLastReachable());
	fields->Set("original_attributes", JsonEncode(service->GetOriginalAttributes()));

	fields->Set("current_notification_number", CompatUtility::GetCheckableNotificationNotificationNumber(service));
	fields->Set("last_notification", DbValue::FromTimestamp(CompatUtility::GetCheckableNotificationLastNotification(service)));
	fields->Set("next_notification", DbValue::FromTimestamp(CompatUtility::GetCheckableNotificationNextNotification(service)));

	EventCommand::Ptr eventCommand = service->GetEventCommand();

	if (eventCommand)
		fields->Set("event_handler", eventCommand->GetName());

	CheckCommand::Ptr checkCommand = service->GetCheckCommand();

	if (checkCommand)
		fields->Set("check_command", checkCommand->GetName());

	return fields;
}

void ServiceDbObject::OnConfigUpdateHeavy()
{
	Service::Ptr service = static_pointer_cast<Service>(GetObject());

	/* groups */
	Array::Ptr groups = service->GetGroups();

	std::vector<DbQuery> queries;

	DbQuery query1;
	query1.Table = DbType::GetByName("ServiceGroup")->GetTable() + "_members";
	query1.Type = DbQueryDelete;
	query1.Category = DbCatConfig;
	query1.WhereCriteria = new Dictionary({
		{ "service_object_id", service }
	});
	queries.emplace_back(std::move(query1));

	if (groups) {
		ObjectLock olock(groups);
		for (const auto& groupName : groups) {
			ServiceGroup::Ptr group = ServiceGroup::GetByName(groupName);

			DbQuery query2;
			query2.Table = DbType::GetByName("ServiceGroup")->GetTable() + "_members";
			query2.Type = DbQueryInsert;
			query2.Category = DbCatConfig;
			query2.Fields = new Dictionary({
				{ "instance_id", 0 }, /* DbConnection class fills in real ID */
				{ "servicegroup_id", DbValue::FromObjectInsertID(group) },
				{ "service_object_id", service }
			});
			query2.WhereCriteria = new Dictionary({
				{ "instance_id", 0 }, /* DbConnection class fills in real ID */
				{ "servicegroup_id", DbValue::FromObjectInsertID(group) },
				{ "service_object_id", service }
			});
			queries.emplace_back(std::move(query2));
		}
	}

	DbObject::OnMultipleQueries(queries);

	/* service dependencies */
	queries.clear();

	DbQuery query2;
	query2.Table = GetType()->GetTable() + "dependencies";
	query2.Type = DbQueryDelete;
	query2.Category = DbCatConfig;
	query2.WhereCriteria = new Dictionary({
		{ "dependent_service_object_id", service }
	});
	queries.emplace_back(std::move(query2));

	for (const auto& dep : service->GetDependencies()) {
		Checkable::Ptr parent = dep->GetParent();

		if (!parent) {
			Log(LogDebug, "ServiceDbObject")
				<< "Missing parent for dependency '" << dep->GetName() << "'.";
			continue;
		}

		Log(LogDebug, "ServiceDbObject")
			<< "service parents: " << parent->GetName();

		int stateFilter = dep->GetStateFilter();

		/* service dependencies */
		DbQuery query1;
		query1.Table = GetType()->GetTable() + "dependencies";
		query1.Type = DbQueryInsert;
		query1.Category = DbCatConfig;
		query1.Fields = new Dictionary({
			{ "service_object_id", parent },
			{ "dependent_service_object_id", service },
			{ "inherits_parent", 1 },
			{ "timeperiod_object_id", dep->GetPeriod() },
			{ "fail_on_ok", (stateFilter & StateFilterOK) ? 1 : 0 },
			{ "fail_on_warning", (stateFilter & StateFilterWarning) ? 1 : 0 },
			{ "fail_on_critical", (stateFilter & StateFilterCritical) ? 1 : 0 },
			{ "fail_on_unknown", (stateFilter & StateFilterUnknown) ? 1 : 0 },
			{ "instance_id", 0 } /* DbConnection class fills in real ID */
		});
		queries.emplace_back(std::move(query1));
	}

	DbObject::OnMultipleQueries(queries);

	/* service contacts, contactgroups */
	queries.clear();

	DbQuery query3;
	query3.Table = GetType()->GetTable() + "_contacts";
	query3.Type = DbQueryDelete;
	query3.Category = DbCatConfig;
	query3.WhereCriteria = new Dictionary({
		{ "service_id", DbValue::FromObjectInsertID(service) }
	});
	queries.emplace_back(std::move(query3));

	for (const auto& user : CompatUtility::GetCheckableNotificationUsers(service)) {
		DbQuery query_contact;
		query_contact.Table = GetType()->GetTable() + "_contacts";
		query_contact.Type = DbQueryInsert;
		query_contact.Category = DbCatConfig;
		query_contact.Fields = new Dictionary({
			{ "service_id", DbValue::FromObjectInsertID(service) },
			{ "contact_object_id", user },
			{ "instance_id", 0 } /* DbConnection class fills in real ID */

		});
		queries.emplace_back(std::move(query_contact));
	}

	DbObject::OnMultipleQueries(queries);

	queries.clear();

	DbQuery query4;
	query4.Table = GetType()->GetTable() + "_contactgroups";
	query4.Type = DbQueryDelete;
	query4.Category = DbCatConfig;
	query4.WhereCriteria = new Dictionary({
		{ "service_id", DbValue::FromObjectInsertID(service) }
	});
	queries.emplace_back(std::move(query4));

	for (const auto& usergroup : CompatUtility::GetCheckableNotificationUserGroups(service)) {
		DbQuery query_contact;
		query_contact.Table = GetType()->GetTable() + "_contactgroups";
		query_contact.Type = DbQueryInsert;
		query_contact.Category = DbCatConfig;
		query_contact.Fields = new Dictionary({
			{ "service_id", DbValue::FromObjectInsertID(service) },
			{ "contactgroup_object_id", usergroup },
			{ "instance_id", 0 } /* DbConnection class fills in real ID */
		});
		queries.emplace_back(std::move(query_contact));
	}

	DbObject::OnMultipleQueries(queries);

	DoCommonConfigUpdate();
}

void ServiceDbObject::OnConfigUpdateLight()
{
	DoCommonConfigUpdate();
}

void ServiceDbObject::DoCommonConfigUpdate()
{
	Service::Ptr service = static_pointer_cast<Service>(GetObject());

	/* update comments and downtimes on config change */
	DbEvents::AddComments(service);
	DbEvents::AddDowntimes(service);
}

String ServiceDbObject::CalculateConfigHash(const Dictionary::Ptr& configFields) const
{
	String hashData = DbObject::CalculateConfigHash(configFields);

	Service::Ptr service = static_pointer_cast<Service>(GetObject());

	Array::Ptr groups = service->GetGroups();

	if (groups) {
		groups = groups->ShallowClone();
		ObjectLock oLock (groups);
		std::sort(groups->Begin(), groups->End());
		hashData += DbObject::HashValue(groups);
	}

	ArrayData dependencies;

	/* dependencies */
	for (const auto& dep : service->GetDependencies()) {
		Checkable::Ptr parent = dep->GetParent();

		if (!parent)
			continue;

		dependencies.push_back(new Array({
			parent->GetName(),
			dep->GetStateFilter(),
			dep->GetPeriodRaw()
		}));
	}

	std::sort(dependencies.begin(), dependencies.end());

	hashData += DbObject::HashValue(new Array(std::move(dependencies)));

	ArrayData users;

	for (const auto& user : CompatUtility::GetCheckableNotificationUsers(service)) {
		users.push_back(user->GetName());
	}

	std::sort(users.begin(), users.end());

	hashData += DbObject::HashValue(new Array(std::move(users)));

	ArrayData userGroups;

	for (const auto& usergroup : CompatUtility::GetCheckableNotificationUserGroups(service)) {
		userGroups.push_back(usergroup->GetName());
	}

	std::sort(userGroups.begin(), userGroups.end());

	hashData += DbObject::HashValue(new Array(std::move(userGroups)));

	return SHA256(hashData);
}
