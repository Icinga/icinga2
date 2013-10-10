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

String DbConnection::GetTablePrefix(void) const
{
	if (m_TablePrefix.IsEmpty())
		return "icinga_";
	else
		return m_TablePrefix;
}

void DbConnection::InsertRuntimeVariable(const String& key, const Value& value)
{
	DbQuery query;
	query.Table = "runtimevariables";
	query.Type = DbQueryInsert;
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
	query1.WhereCriteria = boost::make_shared<Dictionary>();
	query1.WhereCriteria->Set("instance_id", 0);  /* DbConnection class fills in real ID */
	DbObject::OnQuery(query1);

	DbQuery query2;
	query2.Table = "programstatus";
	query2.Type = DbQueryInsert;

	query2.Fields = boost::make_shared<Dictionary>();
	query2.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */
	query2.Fields->Set("status_update_time", DbValue::FromTimestamp(Utility::GetTime()));
	query2.Fields->Set("program_start_time", DbValue::FromTimestamp(IcingaApplication::GetInstance()->GetStartTime()));
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

	if (GetCleanUpAcknowledgementsAge() > 0) {
		CleanUpExecuteQuery("acknowledgements", "entry_time", now - GetCleanUpAcknowledgementsAge());
		Log(LogDebug, "db_ido", "GetCleanUpAcknowledgementsAge: " + Convert::ToString(GetCleanUpAcknowledgementsAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanUpAcknowledgementsAge()));
	}
	if (GetCleanUpCommentHistoryAge() > 0) {
		CleanUpExecuteQuery("commenthistory", "entry_time", now - GetCleanUpCommentHistoryAge());
		Log(LogDebug, "db_ido", "GetCleanUpCommentHistoryAge: " + Convert::ToString(GetCleanUpCommentHistoryAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanUpCommentHistoryAge()));
	}
	if (GetCleanUpContactNotificationsAge() > 0) {
		CleanUpExecuteQuery("contactnotifications", "start_time", now - GetCleanUpContactNotificationsAge());
		Log(LogDebug, "db_ido", "GetCleanUpContactNotificationsAge: " + Convert::ToString(GetCleanUpContactNotificationsAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanUpContactNotificationsAge()));
	}
	if (GetCleanUpContactNotificationMethodsAge() > 0) {
		CleanUpExecuteQuery("contactnotificationmethods", "start_time", now - GetCleanUpContactNotificationMethodsAge());
		Log(LogDebug, "db_ido", "GetCleanUpContactNotificationMethodsAge: " + Convert::ToString(GetCleanUpContactNotificationMethodsAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanUpContactNotificationMethodsAge()));
	}
	if (GetCleanUpDowntimeHistoryAge() > 0) {
		CleanUpExecuteQuery("downtimehistory", "entry_time", now - GetCleanUpDowntimeHistoryAge());
		Log(LogDebug, "db_ido", "CleanUpDowntimeHistoryAge: " + Convert::ToString(GetCleanUpDowntimeHistoryAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanUpDowntimeHistoryAge()));
	}
	if (GetCleanUpEventHandlersAge() > 0) {
		CleanUpExecuteQuery("eventhandlers", "start_time", now - GetCleanUpEventHandlersAge());
		Log(LogDebug, "db_ido", "GetCleanUpEventHandlersAge: " + Convert::ToString(GetCleanUpEventHandlersAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanUpEventHandlersAge()));
	}
	if (GetCleanUpExternalCommandsAge() > 0) {
		CleanUpExecuteQuery("externalcommands", "entry_time", now - GetCleanUpExternalCommandsAge());
		Log(LogDebug, "db_ido", "GetCleanUpExternalCommandsAge: " + Convert::ToString(GetCleanUpExternalCommandsAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanUpExternalCommandsAge()));
	}
	if (GetCleanUpFlappingHistoryAge() > 0) {
		CleanUpExecuteQuery("flappinghistory", "event_time", now - GetCleanUpFlappingHistoryAge());
		Log(LogDebug, "db_ido", "GetCleanUpFlappingHistoryAge: " + Convert::ToString(GetCleanUpFlappingHistoryAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanUpFlappingHistoryAge()));
	}
	if (GetCleanUpHostChecksAge() > 0) {
		CleanUpExecuteQuery("hostchecks", "start_time", now - GetCleanUpHostChecksAge());
		Log(LogDebug, "db_ido", "GetCleanUpHostChecksAge: " + Convert::ToString(GetCleanUpHostChecksAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanUpHostChecksAge()));
	}
	if (GetCleanUpLogEntriesAge() > 0) {
		CleanUpExecuteQuery("logentries", "logentry_time", now - GetCleanUpLogEntriesAge());
		Log(LogDebug, "db_ido", "GetCleanUpLogEntriesAge: " + Convert::ToString(GetCleanUpLogEntriesAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanUpLogEntriesAge()));
	}
	if (GetCleanUpNotificationsAge() > 0) {
		CleanUpExecuteQuery("notifications", "start_time", now - GetCleanUpNotificationsAge());
		Log(LogDebug, "db_ido", "GetCleanUpNotificationsAge: " + Convert::ToString(GetCleanUpNotificationsAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanUpNotificationsAge()));
	}
	if (GetCleanUpProcessEventsAge() > 0) {
		CleanUpExecuteQuery("processevents", "event_time", now - GetCleanUpProcessEventsAge());
		Log(LogDebug, "db_ido", "GetCleanUpProcessEventsAge: " + Convert::ToString(GetCleanUpProcessEventsAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanUpProcessEventsAge()));
	}
	if (GetCleanUpStateHistoryAge() > 0) {
		CleanUpExecuteQuery("statehistory", "state_time", now - GetCleanUpStateHistoryAge());
		Log(LogDebug, "db_ido", "GetCleanUpStateHistoryAge: " + Convert::ToString(GetCleanUpStateHistoryAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanUpStateHistoryAge()));
	}
	if (GetCleanUpServiceChecksAge() > 0) {
		CleanUpExecuteQuery("servicechecks", "start_time", now - GetCleanUpServiceChecksAge());
		Log(LogDebug, "db_ido", "GetCleanUpServiceChecksAge: " + Convert::ToString(GetCleanUpServiceChecksAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanUpServiceChecksAge()));
	}
	if (GetCleanUpSystemCommandsAge() > 0) {
		CleanUpExecuteQuery("systemcommands", "start_time", now - GetCleanUpSystemCommandsAge());
		Log(LogDebug, "db_ido", "GetCleanUpSystemCommandsAge: " + Convert::ToString(GetCleanUpSystemCommandsAge()) +
		    " now: " + Convert::ToString(now) +
		    " old: " + Convert::ToString(now - GetCleanUpSystemCommandsAge()));
	}
}

void DbConnection::CleanUpExecuteQuery(const String& table, const String& time_key, double time_value)
{
	/* Default handler does nothing. */
}

Dictionary::Ptr DbConnection::GetCleanUp(void) const
{
	if (!m_CleanUp)
		return Empty;
	else
		return m_CleanUp;
}

Value DbConnection::GetCleanUpAcknowledgementsAge(void) const
{
	Dictionary::Ptr cleanup = GetCleanUp();

	if (!cleanup || cleanup->Get("acknowledgement_age").IsEmpty())
		return CleanUpAgeNone;
	else
		return cleanup->Get("acknowledgement_age");
}

Value DbConnection::GetCleanUpCommentHistoryAge(void) const
{
	Dictionary::Ptr cleanup = GetCleanUp();

	if (!cleanup || cleanup->Get("commenthistory_age").IsEmpty())
		return CleanUpAgeNone;
	else
		return cleanup->Get("commenthistory_age");
}

Value DbConnection::GetCleanUpContactNotificationsAge(void) const
{
	Dictionary::Ptr cleanup = GetCleanUp();

	if (!cleanup || cleanup->Get("contactnotifications_age").IsEmpty())
		return CleanUpAgeNone;
	else
		return cleanup->Get("contactnotifications_age");
}

Value DbConnection::GetCleanUpContactNotificationMethodsAge(void) const
{
	Dictionary::Ptr cleanup = GetCleanUp();

	if (!cleanup || cleanup->Get("contactnotificationmethods_age").IsEmpty())
		return CleanUpAgeNone;
	else
		return cleanup->Get("contactnotificationmethods_age");
}

Value DbConnection::GetCleanUpDowntimeHistoryAge(void) const
{
	Dictionary::Ptr cleanup = GetCleanUp();

	if (!cleanup || cleanup->Get("downtimehistory_age").IsEmpty())
		return CleanUpAgeNone;
	else
		return cleanup->Get("downtimehistory_age");
}

Value DbConnection::GetCleanUpEventHandlersAge(void) const
{
	Dictionary::Ptr cleanup = GetCleanUp();

	if (!cleanup || cleanup->Get("eventhandlers_age").IsEmpty())
		return CleanUpAgeNone;
	else
		return cleanup->Get("eventhandlers_age");
}

Value DbConnection::GetCleanUpExternalCommandsAge(void) const
{
	Dictionary::Ptr cleanup = GetCleanUp();

	if (!cleanup || cleanup->Get("externalcommands_age").IsEmpty())
		return CleanUpAgeNone;
	else
		return cleanup->Get("externalcommands_age");
}

Value DbConnection::GetCleanUpFlappingHistoryAge(void) const
{
	Dictionary::Ptr cleanup = GetCleanUp();

	if (!cleanup || cleanup->Get("flappinghistory_age").IsEmpty())
		return CleanUpAgeNone;
	else
		return cleanup->Get("flappinghistory_age");
}

Value DbConnection::GetCleanUpHostChecksAge(void) const
{
	Dictionary::Ptr cleanup = GetCleanUp();

	if (!cleanup || cleanup->Get("hostchecks_age").IsEmpty())
		return CleanUpAgeNone;
	else
		return cleanup->Get("hostchecks_age");
}

Value DbConnection::GetCleanUpLogEntriesAge(void) const
{
	Dictionary::Ptr cleanup = GetCleanUp();

	if (!cleanup || cleanup->Get("logentries_age").IsEmpty())
		return CleanUpAgeNone;
	else
		return cleanup->Get("logentries_age");
}

Value DbConnection::GetCleanUpNotificationsAge(void) const
{
	Dictionary::Ptr cleanup = GetCleanUp();

	if (!cleanup || cleanup->Get("notifications_age").IsEmpty())
		return CleanUpAgeNone;
	else
		return cleanup->Get("notifications_age");
}

Value DbConnection::GetCleanUpProcessEventsAge(void) const
{
	Dictionary::Ptr cleanup = GetCleanUp();

	if (!cleanup || cleanup->Get("processevents_age").IsEmpty())
		return CleanUpAgeNone;
	else
		return cleanup->Get("processevents_age");
}

Value DbConnection::GetCleanUpStateHistoryAge(void) const
{
	Dictionary::Ptr cleanup = GetCleanUp();

	if (!cleanup || cleanup->Get("statehistory_age").IsEmpty())
		return CleanUpAgeNone;
	else
		return cleanup->Get("statehistory_age");
}

Value DbConnection::GetCleanUpServiceChecksAge(void) const
{
	Dictionary::Ptr cleanup = GetCleanUp();

	if (!cleanup || cleanup->Get("servicechecks_age").IsEmpty())
		return CleanUpAgeNone;
	else
		return cleanup->Get("servicechecks_age");
}

Value DbConnection::GetCleanUpSystemCommandsAge(void) const
{
	Dictionary::Ptr cleanup = GetCleanUp();

	if (!cleanup || cleanup->Get("systemcommands_age").IsEmpty())
		return CleanUpAgeNone;
	else
		return cleanup->Get("systemcommands_age");
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

void DbConnection::InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const
{
	DynamicObject::InternalSerialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		bag->Set("table_prefix", m_TablePrefix);
		bag->Set("cleanup", m_CleanUp);
	}
}

void DbConnection::InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes)
{
	DynamicObject::InternalDeserialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		m_TablePrefix = bag->Get("table_prefix");
		m_CleanUp = bag->Get("cleanup");
	}
}