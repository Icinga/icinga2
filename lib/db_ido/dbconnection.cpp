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

#include "db_ido/dbconnection.hpp"
#include "db_ido/dbconnection.tcpp"
#include "db_ido/dbvalue.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/host.hpp"
#include "icinga/service.hpp"
#include "base/configtype.hpp"
#include "base/convert.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(DbConnection);

Timer::Ptr DbConnection::m_ProgramStatusTimer;
boost::once_flag DbConnection::m_OnceFlag = BOOST_ONCE_INIT;

DbConnection::DbConnection(void)
	: m_QueryStats(15 * 60), m_PendingQueries(0), m_PendingQueriesTimestamp(0)
{ }

void DbConnection::OnConfigLoaded(void)
{
	ConfigObject::OnConfigLoaded();

	if (!GetEnableHa()) {
		Log(LogDebug, "DbConnection")
		    << "HA functionality disabled. Won't pause IDO connection: " << GetName();

		SetHAMode(HARunEverywhere);
	}

	boost::call_once(m_OnceFlag, InitializeDbTimer);
}

void DbConnection::Start(bool runtimeCreated)
{
	ObjectImpl<DbConnection>::Start(runtimeCreated);

	DbObject::OnQuery.connect(boost::bind(&DbConnection::ExecuteQuery, this, _1));
	DbObject::OnMultipleQueries.connect(boost::bind(&DbConnection::ExecuteMultipleQueries, this, _1));
	ConfigObject::OnActiveChanged.connect(boost::bind(&DbConnection::UpdateObject, this, _1));
}

void DbConnection::StatsLoggerTimerHandler(void)
{
	if (!GetConnected())
		return;

	int pending = GetPendingQueryCount();

	double now = Utility::GetTime();
	double gradient = (pending - m_PendingQueries) / (now - m_PendingQueriesTimestamp);
	double timeToZero = -pending / gradient;

	String timeInfo;

	if (pending > GetQueryCount(5)) {
		timeInfo = " empty in ";
		if (timeToZero < 0)
			timeInfo += "infinite time, your database isn't able to keep up";
		else
			timeInfo += Utility::FormatDuration(timeToZero);
	}

	m_PendingQueries = pending;
	m_PendingQueriesTimestamp = now;

	Log(LogInformation, GetReflectionType()->GetName())
	    << "Query queue items: " << pending
	    << ", query rate: " << std::setw(2) << GetQueryCount(60) / 60.0 << "/s"
	    << " (" << GetQueryCount(60) << "/min " << GetQueryCount(5 * 60) << "/5min " << GetQueryCount(15 * 60) << "/15min);"
	    << timeInfo;
}

void DbConnection::Resume(void)
{
	ConfigObject::Resume();

	Log(LogInformation, "DbConnection")
	    << "Resuming IDO connection: " << GetName();

	m_CleanUpTimer = new Timer();
	m_CleanUpTimer->SetInterval(60);
	m_CleanUpTimer->OnTimerExpired.connect(boost::bind(&DbConnection::CleanUpHandler, this));
	m_CleanUpTimer->Start();

	m_StatsLoggerTimer = new Timer();
	m_StatsLoggerTimer->SetInterval(15);
	m_StatsLoggerTimer->OnTimerExpired.connect(boost::bind(&DbConnection::StatsLoggerTimerHandler, this));
	m_StatsLoggerTimer->Start();
}

void DbConnection::Pause(void)
{
	ConfigObject::Pause();

	Log(LogInformation, "DbConnection")
	     << "Pausing IDO connection: " << GetName();

	m_CleanUpTimer.reset();

	DbQuery query1;
	query1.Table = "programstatus";
	query1.IdColumn = "programstatus_id";
	query1.Type = DbQueryUpdate;
	query1.Category = DbCatProgramStatus;
	query1.WhereCriteria = new Dictionary();
	query1.WhereCriteria->Set("instance_id", 0);  /* DbConnection class fills in real ID */

	query1.Fields = new Dictionary();
	query1.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */
	query1.Fields->Set("program_end_time", DbValue::FromTimestamp(Utility::GetTime()));

	query1.Priority = PriorityHigh;

	ExecuteQuery(query1);

	NewTransaction();
}

void DbConnection::InitializeDbTimer(void)
{
	m_ProgramStatusTimer = new Timer();
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
	query.Fields = new Dictionary();
	query.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */
	query.Fields->Set("varname", key);
	query.Fields->Set("varvalue", value);
	DbObject::OnQuery(query);
}

void DbConnection::ProgramStatusHandler(void)
{
	Log(LogNotice, "DbConnection")
	     << "Updating programstatus table.";

	std::vector<DbQuery> queries;

	DbQuery query1;
	query1.Table = "programstatus";
	query1.Type = DbQueryDelete;
	query1.Category = DbCatProgramStatus;
	query1.WhereCriteria = new Dictionary();
	query1.WhereCriteria->Set("instance_id", 0);  /* DbConnection class fills in real ID */
	query1.Priority = PriorityHigh;
	queries.push_back(query1);

	DbQuery query2;
	query2.Table = "programstatus";
	query2.IdColumn = "programstatus_id";
	query2.Type = DbQueryInsert;
	query2.Category = DbCatProgramStatus;

	query2.Fields = new Dictionary();
	query2.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */
	query2.Fields->Set("program_version", Application::GetAppVersion());
	query2.Fields->Set("status_update_time", DbValue::FromTimestamp(Utility::GetTime()));
	query2.Fields->Set("program_start_time", DbValue::FromTimestamp(Application::GetStartTime()));
	query2.Fields->Set("is_currently_running", 1);
	query2.Fields->Set("endpoint_name", IcingaApplication::GetInstance()->GetNodeName());
	query2.Fields->Set("process_id", Utility::GetPid());
	query2.Fields->Set("daemon_mode", 1);
	query2.Fields->Set("last_command_check", DbValue::FromTimestamp(Utility::GetTime()));
	query2.Fields->Set("notifications_enabled", (IcingaApplication::GetInstance()->GetEnableNotifications() ? 1 : 0));
	query2.Fields->Set("active_host_checks_enabled", (IcingaApplication::GetInstance()->GetEnableHostChecks() ? 1 : 0));
	query2.Fields->Set("passive_host_checks_enabled", 1);
	query2.Fields->Set("active_service_checks_enabled", (IcingaApplication::GetInstance()->GetEnableServiceChecks() ? 1 : 0));
	query2.Fields->Set("passive_service_checks_enabled", 1);
	query2.Fields->Set("event_handlers_enabled", (IcingaApplication::GetInstance()->GetEnableEventHandlers() ? 1 : 0));
	query2.Fields->Set("flap_detection_enabled", (IcingaApplication::GetInstance()->GetEnableFlapping() ? 1 : 0));
	query2.Fields->Set("process_performance_data", (IcingaApplication::GetInstance()->GetEnablePerfdata() ? 1 : 0));
	query2.Priority = PriorityHigh;
	queries.push_back(query2);

	DbObject::OnMultipleQueries(queries);

	DbQuery query3;
	query3.Table = "runtimevariables";
	query3.Type = DbQueryDelete;
	query3.Category = DbCatProgramStatus;
	query3.WhereCriteria = new Dictionary();
	query3.WhereCriteria->Set("instance_id", 0);  /* DbConnection class fills in real ID */
	DbObject::OnQuery(query3);

	InsertRuntimeVariable("total_services", std::distance(ConfigType::GetObjectsByType<Service>().first, ConfigType::GetObjectsByType<Service>().second));
	InsertRuntimeVariable("total_scheduled_services", std::distance(ConfigType::GetObjectsByType<Service>().first, ConfigType::GetObjectsByType<Service>().second));
	InsertRuntimeVariable("total_hosts", std::distance(ConfigType::GetObjectsByType<Host>().first, ConfigType::GetObjectsByType<Host>().second));
	InsertRuntimeVariable("total_scheduled_hosts", std::distance(ConfigType::GetObjectsByType<Host>().first, ConfigType::GetObjectsByType<Host>().second));
}

void DbConnection::CleanUpHandler(void)
{
	long now = static_cast<long>(Utility::GetTime());

	struct {
		String name;
		String time_column;
	} tables[] = {
		{ "acknowledgements", "entry_time" },
		{ "commenthistory", "entry_time" },
		{ "contactnotifications", "start_time" },
		{ "contactnotificationmethods", "start_time" },
		{ "downtimehistory", "entry_time" },
		{ "eventhandlers", "start_time" },
		{ "externalcommands", "entry_time" },
		{ "flappinghistory" "event_time" },
		{ "hostchecks", "start_time" },
		{ "logentries", "logentry_time" },
		{ "notifications", "start_time" },
		{ "processevents", "event_time" },
		{ "statehistory", "state_time" },
		{ "servicechecks", "start_time" },
		{ "systemcommands", "start_time" }
	};

	for (size_t i = 0; i < sizeof(tables) / sizeof(tables[0]); i++) {
		double max_age = GetCleanup()->Get(tables[i].name + "_age");

		if (max_age == 0)
			continue;

		CleanUpExecuteQuery(tables[i].name, tables[i].time_column, now - max_age);
		Log(LogNotice, "DbConnection")
		    << "Cleanup (" << tables[i].name << "): " << max_age
		    << " now: " << now
		    << " old: " << now - max_age;
	}

}

void DbConnection::CleanUpExecuteQuery(const String&, const String&, double)
{
	/* Default handler does nothing. */
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
	SetInsertID(dbobj->GetType(), GetObjectID(dbobj), dbref);
}

void DbConnection::SetInsertID(const DbType::Ptr& type, const DbReference& objid, const DbReference& dbref)
{
	if (!objid.IsValid())
		return;

	if (dbref.IsValid())
		m_InsertIDs[std::make_pair(type, objid)] = dbref;
	else
		m_InsertIDs.erase(std::make_pair(type, objid));
}

DbReference DbConnection::GetInsertID(const DbObject::Ptr& dbobj) const
{
	return GetInsertID(dbobj->GetType(), GetObjectID(dbobj));
}

DbReference DbConnection::GetInsertID(const DbType::Ptr& type, const DbReference& objid) const
{
	if (!objid.IsValid())
		return DbReference();

	std::map<std::pair<DbType::Ptr, DbReference>, DbReference>::const_iterator it;

	it = m_InsertIDs.find(std::make_pair(type, objid));

	if (it == m_InsertIDs.end())
		return DbReference();

	return it->second;
}

void DbConnection::SetNotificationInsertID(const CustomVarObject::Ptr& obj, const DbReference& dbref)
{
	if (dbref.IsValid())
		m_NotificationInsertIDs[obj] = dbref;
	else
		m_NotificationInsertIDs.erase(obj);
}

DbReference DbConnection::GetNotificationInsertID(const CustomVarObject::Ptr& obj) const
{
	std::map<CustomVarObject::Ptr, DbReference>::const_iterator it;

	it = m_NotificationInsertIDs.find(obj);

	if (it == m_NotificationInsertIDs.end())
		return DbReference();

	return it->second;
}

void DbConnection::SetObjectActive(const DbObject::Ptr& dbobj, bool active)
{
	if (active)
		m_ActiveObjects.insert(dbobj);
	else
		m_ActiveObjects.erase(dbobj);
}

bool DbConnection::GetObjectActive(const DbObject::Ptr& dbobj) const
{
	return (m_ActiveObjects.find(dbobj) != m_ActiveObjects.end());
}

void DbConnection::ClearIDCache(void)
{
	m_ObjectIDs.clear();
	m_InsertIDs.clear();
	m_NotificationInsertIDs.clear();
	m_ActiveObjects.clear();
	m_ConfigUpdates.clear();
	m_StatusUpdates.clear();
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

void DbConnection::UpdateObject(const ConfigObject::Ptr& object)
{
	if (!GetConnected())
		return;

	DbObject::Ptr dbobj = DbObject::GetOrCreateByObject(object);

	if (dbobj) {
		bool dbActive = GetObjectActive(dbobj);
		bool active = object->IsActive();

		if (active && !dbActive) {
			ActivateObject(dbobj);
			dbobj->SendConfigUpdate();
			dbobj->SendStatusUpdate();
		} else if (!active) {
			/* Deactivate the deleted object no matter
			 * which state it had in the database.
			 */
			DeactivateObject(dbobj);
		}
	}
}

void DbConnection::UpdateAllObjects(void)
{
	ConfigType::Ptr type;
	BOOST_FOREACH(const ConfigType::Ptr& dt, ConfigType::GetTypes()) {
		BOOST_FOREACH(const ConfigObject::Ptr& object, dt->GetObjects()) {
			UpdateObject(object);
		}
	}
}

void DbConnection::PrepareDatabase(void)
{
	/*
	 * only clear tables on reconnect which
	 * cannot be updated by their existing ids
	 * for details check https://dev.icinga.org/issues/5565
	 */

	//ClearConfigTable("commands");
	//ClearConfigTable("comments");
	ClearConfigTable("contact_addresses");
	ClearConfigTable("contact_notificationcommands");
	ClearConfigTable("contactgroup_members");
	//ClearConfigTable("contactgroups");
	//ClearConfigTable("contacts");
	//ClearConfigTable("contactstatus");
	//ClearConfigTable("customvariables");
	//ClearConfigTable("customvariablestatus");
	//ClearConfigTable("endpoints");
	//ClearConfigTable("endpointstatus");
	ClearConfigTable("host_contactgroups");
	ClearConfigTable("host_contacts");
	ClearConfigTable("host_parenthosts");
	ClearConfigTable("hostdependencies");
	ClearConfigTable("hostgroup_members");
	//ClearConfigTable("hostgroups");
	//ClearConfigTable("hosts");
	//ClearConfigTable("hoststatus");
	//ClearConfigTable("scheduleddowntime");
	ClearConfigTable("service_contactgroups");
	ClearConfigTable("service_contacts");
	ClearConfigTable("servicedependencies");
	ClearConfigTable("servicegroup_members");
	//ClearConfigTable("servicegroups");
	//ClearConfigTable("services");
	//ClearConfigTable("servicestatus");
	ClearConfigTable("timeperiod_timeranges");
	//ClearConfigTable("timeperiods");

	BOOST_FOREACH(const DbType::Ptr& type, DbType::GetAllTypes()) {
		FillIDCache(type);
	}
}

void DbConnection::ValidateFailoverTimeout(double value, const ValidationUtils& utils)
{
	ObjectImpl<DbConnection>::ValidateFailoverTimeout(value, utils);

	if (value < 60)
		BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of("failover_timeout"), "Failover timeout minimum is 60s."));
}

void DbConnection::IncreaseQueryCount(void)
{
	double now = Utility::GetTime();

	boost::mutex::scoped_lock lock(m_StatsMutex);
	m_QueryStats.InsertValue(now, 1);
}

int DbConnection::GetQueryCount(RingBuffer::SizeType span) const
{
	boost::mutex::scoped_lock lock(m_StatsMutex);
	return m_QueryStats.GetValues(span);
}
