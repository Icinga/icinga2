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

#include "db_ido/dbconnection.h"
#include "db_ido/dbvalue.h"
#include "icinga/icingaapplication.h"
#include "icinga/host.h"
#include "icinga/service.h"
#include "base/dynamictype.h"
#include "base/convert.h"
#include "base/utility.h"
#include "base/initialize.h"
#include "base/logger_fwd.h"
#include <boost/foreach.hpp>

using namespace icinga;

Timer::Ptr DbConnection::m_ProgramStatusTimer;

INITIALIZE_ONCE(DbConnection, &DbConnection::StaticInitialize);

void DbConnection::Start(void)
{
	DynamicObject::Start();

	DbObject::OnQuery.connect(boost::bind(&DbConnection::ExecuteQuery, this, _1));

	m_CleanUpTimer = boost::make_shared<Timer>();
	m_CleanUpTimer->SetInterval(60);
	m_CleanUpTimer->OnTimerExpired.connect(boost::bind(&DbConnection::CleanUpHandler, this));
	m_CleanUpTimer->Start();
}

void DbConnection::StaticInitialize(void)
{
	m_ProgramStatusTimer = boost::make_shared<Timer>();
	m_ProgramStatusTimer->SetInterval(10);
	m_ProgramStatusTimer->OnTimerExpired.connect(boost::bind(&DbConnection::ProgramStatusHandler));
	m_ProgramStatusTimer->Start();
}

void DbConnection::InsertRuntimeVariable(const String& key, const Value& value)
{
	DbQuery query;
	query.Table = "runtimevariables";
	query.Type = DbQueryInsert;
	query.Category = DbCatProgramStatus;
	query.Fields = boost::make_shared<Dictionary>();
	query.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */
	query.Fields->Set("varname", key);
	query.Fields->Set("varvalue", value);
	DbObject::OnQuery(query);
}

void DbConnection::ProgramStatusHandler(void)
{
	DbQuery query1;
	query1.Table = "programstatus";
	query1.Type = DbQueryDelete;
	query1.Category = DbCatProgramStatus;
	query1.WhereCriteria = boost::make_shared<Dictionary>();
	query1.WhereCriteria->Set("instance_id", 0);  /* DbConnection class fills in real ID */
	DbObject::OnQuery(query1);

	DbQuery query2;
	query2.Table = "programstatus";
	query2.Type = DbQueryInsert;
	query2.Category = DbCatProgramStatus;

	query2.Fields = boost::make_shared<Dictionary>();
	query2.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */
	query2.Fields->Set("status_update_time", DbValue::FromTimestamp(Utility::GetTime()));
	query2.Fields->Set("program_start_time", DbValue::FromTimestamp(Application::GetStartTime()));
	query2.Fields->Set("is_currently_running", 1);
	query2.Fields->Set("process_id", Utility::GetPid());
	query2.Fields->Set("daemon_mode", 1);
	query2.Fields->Set("last_command_check", DbValue::FromTimestamp(Utility::GetTime()));
	query2.Fields->Set("notifications_enabled", 1);
	query2.Fields->Set("active_service_checks_enabled", 1);
	query2.Fields->Set("passive_service_checks_enabled", 1);
	query2.Fields->Set("event_handlers_enabled", 1);
	query2.Fields->Set("flap_detection_enabled", 1);
	query2.Fields->Set("failure_prediction_enabled", 1);
	query2.Fields->Set("process_performance_data", 1);
	DbObject::OnQuery(query2);

	DbQuery query3;
	query3.Table = "runtimevariables";
	query3.Type = DbQueryDelete;
	query3.Category = DbCatProgramStatus;
	query3.WhereCriteria = boost::make_shared<Dictionary>();
	query3.WhereCriteria->Set("instance_id", 0);  /* DbConnection class fills in real ID */
	DbObject::OnQuery(query3);

	InsertRuntimeVariable("total_services", static_cast<long>(DynamicType::GetObjects<Service>().size()));
	InsertRuntimeVariable("total_scheduled_services", static_cast<long>(DynamicType::GetObjects<Service>().size()));
	InsertRuntimeVariable("total_hosts", static_cast<long>(DynamicType::GetObjects<Host>().size()));
	InsertRuntimeVariable("total_scheduled_hosts", static_cast<long>(DynamicType::GetObjects<Host>().size()));
}

void DbConnection::CleanUpHandler(void)
{
	long now = static_cast<long>(Utility::GetTime());

	if (GetCleanupAcknowledgementsAge() > 0) {
		CleanUpExecuteQuery("acknowledgements", "entry_time", now - GetCleanupAcknowledgementsAge());
		Log(LogDebug, "db_ido", "GetCleanupAcknowledgementsAge: " + Convert::ToString(GetCleanupAcknowledgementsAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanupAcknowledgementsAge()));
	}
	if (GetCleanupCommentHistoryAge() > 0) {
		CleanUpExecuteQuery("commenthistory", "entry_time", now - GetCleanupCommentHistoryAge());
		Log(LogDebug, "db_ido", "GetCleanupCommentHistoryAge: " + Convert::ToString(GetCleanupCommentHistoryAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanupCommentHistoryAge()));
	}
	if (GetCleanupContactNotificationsAge() > 0) {
		CleanUpExecuteQuery("contactnotifications", "start_time", now - GetCleanupContactNotificationsAge());
		Log(LogDebug, "db_ido", "GetCleanupContactNotificationsAge: " + Convert::ToString(GetCleanupContactNotificationsAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanupContactNotificationsAge()));
	}
	if (GetCleanupContactNotificationMethodsAge() > 0) {
		CleanUpExecuteQuery("contactnotificationmethods", "start_time", now - GetCleanupContactNotificationMethodsAge());
		Log(LogDebug, "db_ido", "GetCleanupContactNotificationMethodsAge: " + Convert::ToString(GetCleanupContactNotificationMethodsAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanupContactNotificationMethodsAge()));
	}
	if (GetCleanupDowntimeHistoryAge() > 0) {
		CleanUpExecuteQuery("downtimehistory", "entry_time", now - GetCleanupDowntimeHistoryAge());
		Log(LogDebug, "db_ido", "CleanUpDowntimeHistoryAge: " + Convert::ToString(GetCleanupDowntimeHistoryAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanupDowntimeHistoryAge()));
	}
	if (GetCleanupEventHandlersAge() > 0) {
		CleanUpExecuteQuery("eventhandlers", "start_time", now - GetCleanupEventHandlersAge());
		Log(LogDebug, "db_ido", "GetCleanupEventHandlersAge: " + Convert::ToString(GetCleanupEventHandlersAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanupEventHandlersAge()));
	}
	if (GetCleanupExternalCommandsAge() > 0) {
		CleanUpExecuteQuery("externalcommands", "entry_time", now - GetCleanupExternalCommandsAge());
		Log(LogDebug, "db_ido", "GetCleanupExternalCommandsAge: " + Convert::ToString(GetCleanupExternalCommandsAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanupExternalCommandsAge()));
	}
	if (GetCleanupFlappingHistoryAge() > 0) {
		CleanUpExecuteQuery("flappinghistory", "event_time", now - GetCleanupFlappingHistoryAge());
		Log(LogDebug, "db_ido", "GetCleanupFlappingHistoryAge: " + Convert::ToString(GetCleanupFlappingHistoryAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanupFlappingHistoryAge()));
	}
	if (GetCleanupHostChecksAge() > 0) {
		CleanUpExecuteQuery("hostchecks", "start_time", now - GetCleanupHostChecksAge());
		Log(LogDebug, "db_ido", "GetCleanupHostChecksAge: " + Convert::ToString(GetCleanupHostChecksAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanupHostChecksAge()));
	}
	if (GetCleanupLogEntriesAge() > 0) {
		CleanUpExecuteQuery("logentries", "logentry_time", now - GetCleanupLogEntriesAge());
		Log(LogDebug, "db_ido", "GetCleanupLogEntriesAge: " + Convert::ToString(GetCleanupLogEntriesAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanupLogEntriesAge()));
	}
	if (GetCleanupNotificationsAge() > 0) {
		CleanUpExecuteQuery("notifications", "start_time", now - GetCleanupNotificationsAge());
		Log(LogDebug, "db_ido", "GetCleanupNotificationsAge: " + Convert::ToString(GetCleanupNotificationsAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanupNotificationsAge()));
	}
	if (GetCleanupProcessEventsAge() > 0) {
		CleanUpExecuteQuery("processevents", "event_time", now - GetCleanupProcessEventsAge());
		Log(LogDebug, "db_ido", "GetCleanupProcessEventsAge: " + Convert::ToString(GetCleanupProcessEventsAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanupProcessEventsAge()));
	}
	if (GetCleanupStateHistoryAge() > 0) {
		CleanUpExecuteQuery("statehistory", "state_time", now - GetCleanupStateHistoryAge());
		Log(LogDebug, "db_ido", "GetCleanupStateHistoryAge: " + Convert::ToString(GetCleanupStateHistoryAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanupStateHistoryAge()));
	}
	if (GetCleanupServiceChecksAge() > 0) {
		CleanUpExecuteQuery("servicechecks", "start_time", now - GetCleanupServiceChecksAge());
		Log(LogDebug, "db_ido", "GetCleanupServiceChecksAge: " + Convert::ToString(GetCleanupServiceChecksAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanupServiceChecksAge()));
	}
	if (GetCleanupSystemCommandsAge() > 0) {
		CleanUpExecuteQuery("systemcommands", "start_time", now - GetCleanupSystemCommandsAge());
		Log(LogDebug, "db_ido", "GetCleanupSystemCommandsAge: " + Convert::ToString(GetCleanupSystemCommandsAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanupSystemCommandsAge()));
	}
}

void DbConnection::CleanUpExecuteQuery(const String& table, const String& time_key, double time_value)
{
	/* Default handler does nothing. */
}

double DbConnection::GetCleanupAcknowledgementsAge(void) const
{
	return GetCleanup()->Get("acknowledgement_age");
}

double DbConnection::GetCleanupCommentHistoryAge(void) const
{
	return GetCleanup()->Get("commenthistory_age");
}

double DbConnection::GetCleanupContactNotificationsAge(void) const
{
	return GetCleanup()->Get("contactnotifications_age");
}

double DbConnection::GetCleanupContactNotificationMethodsAge(void) const
{
	return GetCleanup()->Get("contactnotificationmethods_age");
}

double DbConnection::GetCleanupDowntimeHistoryAge(void) const
{
	return GetCleanup()->Get("downtimehistory_age");
}

double DbConnection::GetCleanupEventHandlersAge(void) const
{
	return GetCleanup()->Get("eventhandlers_age");
}

double DbConnection::GetCleanupExternalCommandsAge(void) const
{
	return GetCleanup()->Get("externalcommands_age");
}

double DbConnection::GetCleanupFlappingHistoryAge(void) const
{
	return GetCleanup()->Get("flappinghistory_age");
}

double DbConnection::GetCleanupHostChecksAge(void) const
{
	return GetCleanup()->Get("hostchecks_age");
}

double DbConnection::GetCleanupLogEntriesAge(void) const
{
	return GetCleanup()->Get("logentries_age");
}

double DbConnection::GetCleanupNotificationsAge(void) const
{
	return GetCleanup()->Get("notifications_age");
}

double DbConnection::GetCleanupProcessEventsAge(void) const
{
	return GetCleanup()->Get("processevents_age");
}

double DbConnection::GetCleanupStateHistoryAge(void) const
{
	return GetCleanup()->Get("statehistory_age");
}

double DbConnection::GetCleanupServiceChecksAge(void) const
{
	return GetCleanup()->Get("servicechecks_age");
}

double DbConnection::GetCleanupSystemCommandsAge(void) const
{
	return GetCleanup()->Get("systemcommands_age");
}

void DbConnection::SetObjectID(const DbObject::Ptr& dbobj, const DbReference& dbref)
{
	if (dbref.IsValid())
		m_ObjectIDs[dbobj] = dbref;
	else
		m_ObjectIDs.erase(dbobj);
}

DbReference DbConnection::GetObjectID(const DbObject::Ptr& dbobj) const
{
	std::map<DbObject::Ptr, DbReference>::const_iterator it;

	it = m_ObjectIDs.find(dbobj);

	if (it == m_ObjectIDs.end())
		return DbReference();

	return it->second;
}

void DbConnection::SetInsertID(const DbObject::Ptr& dbobj, const DbReference& dbref)
{
	if (dbref.IsValid())
		m_InsertIDs[dbobj] = dbref;
	else
		m_InsertIDs.erase(dbobj);
}

DbReference DbConnection::GetInsertID(const DbObject::Ptr& dbobj) const
{
	std::map<DbObject::Ptr, DbReference>::const_iterator it;

	it = m_InsertIDs.find(dbobj);

	if (it == m_InsertIDs.end())
		return DbReference();

	return it->second;
}

void DbConnection::SetConfigUpdate(const DbObject::Ptr& dbobj, bool hasupdate)
{
	if (hasupdate)
		m_ConfigUpdates.insert(dbobj);
	else
		m_ConfigUpdates.erase(dbobj);
}

bool DbConnection::GetConfigUpdate(const DbObject::Ptr& dbobj) const
{
	return (m_ConfigUpdates.find(dbobj) != m_ConfigUpdates.end());
}

void DbConnection::SetStatusUpdate(const DbObject::Ptr& dbobj, bool hasupdate)
{
	if (hasupdate)
		m_StatusUpdates.insert(dbobj);
	else
		m_StatusUpdates.erase(dbobj);
}

bool DbConnection::GetStatusUpdate(const DbObject::Ptr& dbobj) const
{
	return (m_StatusUpdates.find(dbobj) != m_StatusUpdates.end());
}

void DbConnection::ExecuteQuery(const DbQuery&)
{
	/* Default handler does nothing. */
}

void DbConnection::UpdateAllObjects(void)
{
	DynamicType::Ptr type;
	BOOST_FOREACH(const DynamicType::Ptr& dt, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, dt->GetObjects()) {
			DbObject::Ptr dbobj = DbObject::GetOrCreateByObject(object);

			if (dbobj) {
				ActivateObject(dbobj);
				dbobj->SendConfigUpdate();
				dbobj->SendStatusUpdate();
			}
		}
	}
}
