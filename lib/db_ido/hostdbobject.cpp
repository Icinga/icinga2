/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "db_ido/hostdbobject.hpp"
#include "db_ido/hostgroupdbobject.hpp"
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
#include "icinga/pluginutility.hpp"
#include "base/convert.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/json.hpp"

using namespace icinga;

REGISTER_DBTYPE(Host, "host", DbObjectTypeHost, "host_object_id", HostDbObject);

HostDbObject::HostDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr HostDbObject::GetConfigFields() const
{
	Dictionary::Ptr fields = new Dictionary();
	Host::Ptr host = static_pointer_cast<Host>(GetObject());

	/* Compatibility fallback. */
	String displayName = host->GetDisplayName();

	unsigned long notificationStateFilter = CompatUtility::GetCheckableNotificationTypeFilter(host);
	unsigned long notificationTypeFilter = CompatUtility::GetCheckableNotificationTypeFilter(host);

	return new Dictionary({
		{ "alias", !displayName.IsEmpty() ? displayName : host->GetName() },
		{ "display_name", displayName },
		{ "address", host->GetAddress() },
		{ "address6", host->GetAddress6() },
		{ "check_command_object_id", host->GetCheckCommand() },
		{ "eventhandler_command_object_id", host->GetEventCommand() },
		{ "check_timeperiod_object_id", host->GetCheckPeriod() },
		{ "check_interval", host->GetCheckInterval() / 60.0 },
		{ "retry_interval", host->GetRetryInterval() / 60.0 },
		{ "max_check_attempts", host->GetMaxCheckAttempts() },
		{ "flap_detection_enabled", host->GetEnableFlapping() },
		{ "low_flap_threshold", host->GetFlappingThresholdLow() },
		{ "high_flap_threshold", host->GetFlappingThresholdLow() },
		{ "process_performance_data", host->GetEnablePerfdata() },
		{ "freshness_checks_enabled", 1 },
		{ "freshness_threshold", Convert::ToLong(host->GetCheckInterval()) },
		{ "event_handler_enabled", host->GetEnableEventHandler() },
		{ "passive_checks_enabled", host->GetEnablePassiveChecks() },
		{ "active_checks_enabled", host->GetEnableActiveChecks() },
		{ "notifications_enabled", host->GetEnableNotifications() },
		{ "notes", host->GetNotes() },
		{ "notes_url", host->GetNotesUrl() },
		{ "action_url", host->GetActionUrl() },
		{ "icon_image", host->GetIconImage() },
		{ "icon_image_alt", host->GetIconImageAlt() },
		{ "notification_interval", CompatUtility::GetCheckableNotificationNotificationInterval(host) },
		{ "notify_on_down", (notificationStateFilter & (ServiceWarning | ServiceCritical)) ? 1 : 0 },
		{ "notify_on_unreachable", 1 }, /* We don't have this filter and state, and as such we don't filter such notifications. */
		{ "notify_on_recovery", (notificationTypeFilter & NotificationRecovery) ? 1 : 0 },
		{ "notify_on_flapping", (notificationTypeFilter & (NotificationFlappingStart | NotificationFlappingEnd)) ? 1 : 0 },
		{ "notify_on_downtime", (notificationTypeFilter & (NotificationDowntimeStart | NotificationDowntimeEnd | NotificationDowntimeRemoved)) ? 1 : 0 }
	});
}

Dictionary::Ptr HostDbObject::GetStatusFields() const
{
	Dictionary::Ptr fields = new Dictionary();
	Host::Ptr host = static_pointer_cast<Host>(GetObject());

	CheckResult::Ptr cr = host->GetLastCheckResult();

	if (cr) {
		fields->Set("output", CompatUtility::GetCheckResultOutput(cr));
		fields->Set("long_output", CompatUtility::GetCheckResultLongOutput(cr));
		fields->Set("perfdata", PluginUtility::FormatPerfdata(cr->GetPerformanceData()));
		fields->Set("check_source", cr->GetCheckSource());
		fields->Set("latency", cr->CalculateLatency());
		fields->Set("execution_time", cr->CalculateExecutionTime());
	}

	int currentState = host->GetState();

	if (currentState != HostUp && !host->GetLastReachable())
		currentState = 2; /* hardcoded compat state */

	fields->Set("current_state", currentState);
	fields->Set("has_been_checked", host->HasBeenChecked());
	fields->Set("should_be_scheduled", host->GetEnableActiveChecks());
	fields->Set("current_check_attempt", host->GetCheckAttempt());
	fields->Set("max_check_attempts", host->GetMaxCheckAttempts());
	fields->Set("last_check", DbValue::FromTimestamp(host->GetLastCheck()));
	fields->Set("next_check", DbValue::FromTimestamp(host->GetNextCheck()));
	fields->Set("check_type", !host->GetEnableActiveChecks()); /* 0 .. active, 1 .. passive */
	fields->Set("last_state_change", DbValue::FromTimestamp(host->GetLastStateChange()));
	fields->Set("last_hard_state_change", DbValue::FromTimestamp(host->GetLastHardStateChange()));
	fields->Set("last_hard_state", host->GetLastHardState());
	fields->Set("last_time_up", DbValue::FromTimestamp(host->GetLastStateUp()));
	fields->Set("last_time_down", DbValue::FromTimestamp(host->GetLastStateDown()));
	fields->Set("last_time_unreachable", DbValue::FromTimestamp(host->GetLastStateUnreachable()));
	fields->Set("state_type", host->GetStateType());
	fields->Set("notifications_enabled", host->GetEnableNotifications());
	fields->Set("problem_has_been_acknowledged", host->GetAcknowledgement() != AcknowledgementNone);
	fields->Set("acknowledgement_type", host->GetAcknowledgement());
	fields->Set("passive_checks_enabled", host->GetEnablePassiveChecks());
	fields->Set("active_checks_enabled", host->GetEnableActiveChecks());
	fields->Set("event_handler_enabled", host->GetEnableEventHandler());
	fields->Set("flap_detection_enabled", host->GetEnableFlapping());
	fields->Set("is_flapping", host->IsFlapping());
	fields->Set("percent_state_change", host->GetFlappingCurrent());
	fields->Set("scheduled_downtime_depth", host->GetDowntimeDepth());
	fields->Set("process_performance_data", host->GetEnablePerfdata());
	fields->Set("normal_check_interval", host->GetCheckInterval() / 60.0);
	fields->Set("retry_check_interval", host->GetRetryInterval() / 60.0);
	fields->Set("check_timeperiod_object_id", host->GetCheckPeriod());
	fields->Set("is_reachable", host->GetLastReachable());
	fields->Set("original_attributes", JsonEncode(host->GetOriginalAttributes()));

	fields->Set("current_notification_number", CompatUtility::GetCheckableNotificationNotificationNumber(host));
	fields->Set("last_notification", DbValue::FromTimestamp(CompatUtility::GetCheckableNotificationLastNotification(host)));
	fields->Set("next_notification", DbValue::FromTimestamp(CompatUtility::GetCheckableNotificationNextNotification(host)));

	EventCommand::Ptr eventCommand = host->GetEventCommand();

	if (eventCommand)
		fields->Set("event_handler", eventCommand->GetName());

	CheckCommand::Ptr checkCommand = host->GetCheckCommand();

	if (checkCommand)
		fields->Set("check_command", checkCommand->GetName());

	return fields;
}

void HostDbObject::OnConfigUpdateHeavy()
{
	Host::Ptr host = static_pointer_cast<Host>(GetObject());

	/* groups */
	Array::Ptr groups = host->GetGroups();

	std::vector<DbQuery> queries;

	DbQuery query1;
	query1.Table = DbType::GetByName("HostGroup")->GetTable() + "_members";
	query1.Type = DbQueryDelete;
	query1.Category = DbCatConfig;
	query1.WhereCriteria = new Dictionary();
	query1.WhereCriteria->Set("host_object_id", host);
	queries.emplace_back(std::move(query1));

	if (groups) {
		ObjectLock olock(groups);
		for (const auto& groupName : groups) {
			HostGroup::Ptr group = HostGroup::GetByName(groupName);

			DbQuery query2;
			query2.Table = DbType::GetByName("HostGroup")->GetTable() + "_members";
			query2.Type = DbQueryInsert;
			query2.Category = DbCatConfig;
			query2.Fields = new Dictionary({
				{ "instance_id", 0 }, /* DbConnection class fills in real ID */
				{ "hostgroup_id", DbValue::FromObjectInsertID(group) },
				{ "host_object_id", host }
			});
			query2.WhereCriteria = new Dictionary({
				{ "instance_id", 0 }, /* DbConnection class fills in real ID */
				{ "hostgroup_id", DbValue::FromObjectInsertID(group) },
				{ "host_object_id", host }
			});
			queries.emplace_back(std::move(query2));
		}
	}

	DbObject::OnMultipleQueries(queries);

	queries.clear();

	DbQuery query2;
	query2.Table = GetType()->GetTable() + "_parenthosts";
	query2.Type = DbQueryDelete;
	query2.Category = DbCatConfig;
	query2.WhereCriteria = new Dictionary({
		{ GetType()->GetTable() + "_id", DbValue::FromObjectInsertID(GetObject()) }
	});
	queries.emplace_back(std::move(query2));

	/* parents */
	for (const auto& checkable : host->GetParents()) {
		Host::Ptr parent = dynamic_pointer_cast<Host>(checkable);

		if (!parent)
			continue;

		Log(LogDebug, "HostDbObject")
			<< "host parents: " << parent->GetName();

		/* parents: host_id, parent_host_object_id */
		DbQuery query1;
		query1.Table = GetType()->GetTable() + "_parenthosts";
		query1.Type = DbQueryInsert;
		query1.Category = DbCatConfig;
		query1.Fields = new Dictionary({
			{ GetType()->GetTable() + "_id", DbValue::FromObjectInsertID(GetObject()) },
			{ "parent_host_object_id", parent },
			{ "instance_id", 0 } /* DbConnection class fills in real ID */
		});
		queries.emplace_back(std::move(query1));
	}

	DbObject::OnMultipleQueries(queries);

	/* host dependencies */
	Log(LogDebug, "HostDbObject")
		<< "host dependencies for '" << host->GetName() << "'";

	queries.clear();

	DbQuery query3;
	query3.Table = GetType()->GetTable() + "dependencies";
	query3.Type = DbQueryDelete;
	query3.Category = DbCatConfig;
	query3.WhereCriteria = new Dictionary({
		{ "dependent_host_object_id", host }
	});
	queries.emplace_back(std::move(query3));

	for (const auto& dep : host->GetDependencies()) {
		Checkable::Ptr parent = dep->GetParent();

		if (!parent) {
			Log(LogDebug, "HostDbObject")
				<< "Missing parent for dependency '" << dep->GetName() << "'.";
			continue;
		}

		int stateFilter = dep->GetStateFilter();

		Log(LogDebug, "HostDbObject")
			<< "parent host: " << parent->GetName();

		DbQuery query2;
		query2.Table = GetType()->GetTable() + "dependencies";
		query2.Type = DbQueryInsert;
		query2.Category = DbCatConfig;
		query2.Fields = new Dictionary({
			{ "host_object_id", parent },
			{ "dependent_host_object_id", host },
			{ "inherits_parent", 1 },
			{ "timeperiod_object_id", dep->GetPeriod() },
			{ "fail_on_up", (stateFilter & StateFilterUp) ? 1 : 0 },
			{ "fail_on_down", (stateFilter & StateFilterDown) ? 1 : 0 },
			{ "instance_id", 0 } /* DbConnection class fills in real ID */
		});
		queries.emplace_back(std::move(query2));
	}

	DbObject::OnMultipleQueries(queries);

	Log(LogDebug, "HostDbObject")
		<< "host contacts: " << host->GetName();

	queries.clear();

	DbQuery query4;
	query4.Table = GetType()->GetTable() + "_contacts";
	query4.Type = DbQueryDelete;
	query4.Category = DbCatConfig;
	query4.WhereCriteria = new Dictionary({
		{ "host_id", DbValue::FromObjectInsertID(host) }
	});
	queries.emplace_back(std::move(query4));

	for (const auto& user : CompatUtility::GetCheckableNotificationUsers(host)) {
		Log(LogDebug, "HostDbObject")
			<< "host contacts: " << user->GetName();

		DbQuery query_contact;
		query_contact.Table = GetType()->GetTable() + "_contacts";
		query_contact.Type = DbQueryInsert;
		query_contact.Category = DbCatConfig;
		query_contact.Fields = new Dictionary({
			{ "host_id", DbValue::FromObjectInsertID(host) },
			{ "contact_object_id", user },
			{ "instance_id", 0 } /* DbConnection class fills in real ID */
		});
		queries.emplace_back(std::move(query_contact));
	}

	DbObject::OnMultipleQueries(queries);

	Log(LogDebug, "HostDbObject")
		<< "host contactgroups: " << host->GetName();

	queries.clear();

	DbQuery query5;
	query5.Table = GetType()->GetTable() + "_contactgroups";
	query5.Type = DbQueryDelete;
	query5.Category = DbCatConfig;
	query5.WhereCriteria = new Dictionary({
		{ "host_id", DbValue::FromObjectInsertID(host) }
	});
	queries.emplace_back(std::move(query5));

	for (const auto& usergroup : CompatUtility::GetCheckableNotificationUserGroups(host)) {
		Log(LogDebug, "HostDbObject")
			<< "host contactgroups: " << usergroup->GetName();

		DbQuery query_contact;
		query_contact.Table = GetType()->GetTable() + "_contactgroups";
		query_contact.Type = DbQueryInsert;
		query_contact.Category = DbCatConfig;
		query_contact.Fields = new Dictionary({
			{ "host_id", DbValue::FromObjectInsertID(host) },
			{ "contactgroup_object_id", usergroup },
			{ "instance_id", 0 } /* DbConnection class fills in real ID */
		});
		queries.emplace_back(std::move(query_contact));
	}

	DbObject::OnMultipleQueries(queries);

	DoCommonConfigUpdate();
}

void HostDbObject::OnConfigUpdateLight()
{
	DoCommonConfigUpdate();
}

void HostDbObject::DoCommonConfigUpdate()
{
	Host::Ptr host = static_pointer_cast<Host>(GetObject());

	/* update comments and downtimes on config change */
	DbEvents::AddComments(host);
	DbEvents::AddDowntimes(host);
}

String HostDbObject::CalculateConfigHash(const Dictionary::Ptr& configFields) const
{
	String hashData = DbObject::CalculateConfigHash(configFields);

	Host::Ptr host = static_pointer_cast<Host>(GetObject());

	Array::Ptr groups = host->GetGroups();

	if (groups) {
		groups = groups->ShallowClone();
		ObjectLock oLock (groups);
		std::sort(groups->Begin(), groups->End());
		hashData += DbObject::HashValue(groups);
	}

	ArrayData parents;

	/* parents */
	for (const auto& checkable : host->GetParents()) {
		Host::Ptr parent = dynamic_pointer_cast<Host>(checkable);

		if (!parent)
			continue;

		parents.push_back(parent->GetName());
	}

	std::sort(parents.begin(), parents.end());

	hashData += DbObject::HashValue(new Array(std::move(parents)));

	ArrayData dependencies;

	/* dependencies */
	for (const auto& dep : host->GetDependencies()) {
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

	for (const auto& user : CompatUtility::GetCheckableNotificationUsers(host)) {
		users.push_back(user->GetName());
	}

	std::sort(users.begin(), users.end());

	hashData += DbObject::HashValue(new Array(std::move(users)));

	ArrayData userGroups;

	for (const auto& usergroup : CompatUtility::GetCheckableNotificationUserGroups(host)) {
		userGroups.push_back(usergroup->GetName());
	}

	std::sort(userGroups.begin(), userGroups.end());

	hashData += DbObject::HashValue(new Array(std::move(userGroups)));

	return SHA256(hashData);
}
