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

using namespace icinga;

REGISTER_TYPE(DbConnection);

Timer::Ptr DbConnection::m_ProgramStatusTimer;
boost::once_flag DbConnection::m_OnceFlag = BOOST_ONCE_INIT;

DbConnection::DbConnection()
	: m_IDCacheValid(false), m_QueryStats(15 * 60), m_ActiveChangedHandler(false)
{ }

void DbConnection::OnConfigLoaded()
{
	ConfigObject::OnConfigLoaded();

	Value categories = GetCategories();

	SetCategoryFilter(FilterArrayToInt(categories, DbQuery::GetCategoryFilterMap(), DbCatEverything));

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

	Log(LogInformation, "DbConnection")
		<< "'" << GetName() << "' started.";

	DbObject::OnQuery.connect(std::bind(&DbConnection::ExecuteQuery, this, _1));
	DbObject::OnMultipleQueries.connect(std::bind(&DbConnection::ExecuteMultipleQueries, this, _1));
}

void DbConnection::Stop(bool runtimeRemoved)
{
	Log(LogInformation, "DbConnection")
		<< "'" << GetName() << "' stopped.";

	ObjectImpl<DbConnection>::Stop(runtimeRemoved);
}

void DbConnection::EnableActiveChangedHandler()
{
	if (!m_ActiveChangedHandler) {
		ConfigObject::OnActiveChanged.connect(std::bind(&DbConnection::UpdateObject, this, _1));
		m_ActiveChangedHandler = true;
	}
}

void DbConnection::Resume()
{
	ConfigObject::Resume();

	Log(LogInformation, "DbConnection")
		<< "Resuming IDO connection: " << GetName();

	m_CleanUpTimer = new Timer();
	m_CleanUpTimer->SetInterval(60);
	m_CleanUpTimer->OnTimerExpired.connect(std::bind(&DbConnection::CleanUpHandler, this));
	m_CleanUpTimer->Start();
}

void DbConnection::Pause()
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

void DbConnection::InitializeDbTimer()
{
	m_ProgramStatusTimer = new Timer();
	m_ProgramStatusTimer->SetInterval(10);
	m_ProgramStatusTimer->OnTimerExpired.connect(std::bind(&DbConnection::UpdateProgramStatus));
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

void DbConnection::UpdateProgramStatus()
{
	Log(LogNotice, "DbConnection")
		<< "Updating programstatus table.";

	std::vector<DbQuery> queries;

	DbQuery query1;
	query1.Table = "programstatus";
	query1.IdColumn = "programstatus_id";
	query1.Type = DbQueryInsert | DbQueryUpdate;
	query1.Category = DbCatProgramStatus;

	query1.Fields = new Dictionary();
	query1.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */
	query1.Fields->Set("program_version", Application::GetAppVersion());
	query1.Fields->Set("status_update_time", DbValue::FromTimestamp(Utility::GetTime()));
	query1.Fields->Set("program_start_time", DbValue::FromTimestamp(Application::GetStartTime()));
	query1.Fields->Set("is_currently_running", 1);
	query1.Fields->Set("endpoint_name", IcingaApplication::GetInstance()->GetNodeName());
	query1.Fields->Set("process_id", Utility::GetPid());
	query1.Fields->Set("daemon_mode", 1);
	query1.Fields->Set("last_command_check", DbValue::FromTimestamp(Utility::GetTime()));
	query1.Fields->Set("notifications_enabled", (IcingaApplication::GetInstance()->GetEnableNotifications() ? 1 : 0));
	query1.Fields->Set("active_host_checks_enabled", (IcingaApplication::GetInstance()->GetEnableHostChecks() ? 1 : 0));
	query1.Fields->Set("passive_host_checks_enabled", 1);
	query1.Fields->Set("active_service_checks_enabled", (IcingaApplication::GetInstance()->GetEnableServiceChecks() ? 1 : 0));
	query1.Fields->Set("passive_service_checks_enabled", 1);
	query1.Fields->Set("event_handlers_enabled", (IcingaApplication::GetInstance()->GetEnableEventHandlers() ? 1 : 0));
	query1.Fields->Set("flap_detection_enabled", (IcingaApplication::GetInstance()->GetEnableFlapping() ? 1 : 0));
	query1.Fields->Set("process_performance_data", (IcingaApplication::GetInstance()->GetEnablePerfdata() ? 1 : 0));
	query1.WhereCriteria = new Dictionary();
	query1.WhereCriteria->Set("instance_id", 0);  /* DbConnection class fills in real ID */
	query1.Priority = PriorityHigh;
	queries.emplace_back(std::move(query1));

	DbQuery query2;
	query2.Type = DbQueryNewTransaction;
	queries.emplace_back(std::move(query2));

	DbObject::OnMultipleQueries(queries);

	DbQuery query3;
	query3.Table = "runtimevariables";
	query3.Type = DbQueryDelete;
	query3.Category = DbCatProgramStatus;
	query3.WhereCriteria = new Dictionary();
	query3.WhereCriteria->Set("instance_id", 0);  /* DbConnection class fills in real ID */
	DbObject::OnQuery(query3);

	InsertRuntimeVariable("total_services", ConfigType::Get<Service>()->GetObjectCount());
	InsertRuntimeVariable("total_scheduled_services", ConfigType::Get<Service>()->GetObjectCount());
	InsertRuntimeVariable("total_hosts", ConfigType::Get<Host>()->GetObjectCount());
	InsertRuntimeVariable("total_scheduled_hosts", ConfigType::Get<Host>()->GetObjectCount());
}

void DbConnection::CleanUpHandler()
{
	auto now = static_cast<long>(Utility::GetTime());

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
		{ "flappinghistory", "event_time" },
		{ "hostchecks", "start_time" },
		{ "logentries", "logentry_time" },
		{ "notifications", "start_time" },
		{ "processevents", "event_time" },
		{ "statehistory", "state_time" },
		{ "servicechecks", "start_time" },
		{ "systemcommands", "start_time" }
	};

	for (auto& table : tables) {
		double max_age = GetCleanup()->Get(table.name + "_age");

		if (max_age == 0)
			continue;

		CleanUpExecuteQuery(table.name, table.time_column, now - max_age);
		Log(LogNotice, "DbConnection")
			<< "Cleanup (" << table.name << "): " << max_age
			<< " now: " << now
			<< " old: " << now - max_age;
	}

}

void DbConnection::CleanUpExecuteQuery(const String&, const String&, double)
{
	/* Default handler does nothing. */
}

void DbConnection::SetConfigHash(const DbObject::Ptr& dbobj, const String& hash)
{
	SetConfigHash(dbobj->GetType(), GetObjectID(dbobj), hash);
}

void DbConnection::SetConfigHash(const DbType::Ptr& type, const DbReference& objid, const String& hash)
{
	if (!objid.IsValid())
		return;

	if (!hash.IsEmpty())
		m_ConfigHashes[std::make_pair(type, objid)] = hash;
	else
		m_ConfigHashes.erase(std::make_pair(type, objid));
}

String DbConnection::GetConfigHash(const DbObject::Ptr& dbobj) const
{
	return GetConfigHash(dbobj->GetType(), GetObjectID(dbobj));
}

String DbConnection::GetConfigHash(const DbType::Ptr& type, const DbReference& objid) const
{
	if (!objid.IsValid())
		return String();

	auto it = m_ConfigHashes.find(std::make_pair(type, objid));

	if (it == m_ConfigHashes.end())
		return String();

	return it->second;
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
	auto it = m_ObjectIDs.find(dbobj);

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

	auto it = m_InsertIDs.find(std::make_pair(type, objid));

	if (it == m_InsertIDs.end())
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

void DbConnection::ClearIDCache()
{
	SetIDCacheValid(false);

	m_ObjectIDs.clear();
	m_InsertIDs.clear();
	m_ActiveObjects.clear();
	m_ConfigUpdates.clear();
	m_StatusUpdates.clear();
	m_ConfigHashes.clear();
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
	if (!GetConnected() || Application::IsShuttingDown())
		return;

	DbObject::Ptr dbobj = DbObject::GetOrCreateByObject(object);

	if (dbobj) {
		bool dbActive = GetObjectActive(dbobj);
		bool active = object->IsActive();

		if (active) {
			if (!dbActive)
				ActivateObject(dbobj);

			Dictionary::Ptr configFields = dbobj->GetConfigFields();
			String configHash = dbobj->CalculateConfigHash(configFields);
			ASSERT(configHash.GetLength() <= 64);
			configFields->Set("config_hash", configHash);

			String cachedHash = GetConfigHash(dbobj);

			if (cachedHash != configHash) {
				dbobj->SendConfigUpdateHeavy(configFields);
				dbobj->SendStatusUpdate();
			} else {
				dbobj->SendConfigUpdateLight();
			}
		} else if (!active) {
			/* Deactivate the deleted object no matter
			 * which state it had in the database.
			 */
			DeactivateObject(dbobj);
		}
	}
}

void DbConnection::UpdateAllObjects()
{
	for (const Type::Ptr& type : Type::GetAllTypes()) {
		auto *dtype = dynamic_cast<ConfigType *>(type.get());

		if (!dtype)
			continue;

		for (const ConfigObject::Ptr& object : dtype->GetObjects()) {
			UpdateObject(object);
		}
	}
}

void DbConnection::PrepareDatabase()
{
	for (const DbType::Ptr& type : DbType::GetAllTypes()) {
		FillIDCache(type);
	}
}

void DbConnection::ValidateFailoverTimeout(double value, const ValidationUtils& utils)
{
	ObjectImpl<DbConnection>::ValidateFailoverTimeout(value, utils);

	if (value < 60)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "failover_timeout" }, "Failover timeout minimum is 60s."));
}

void DbConnection::ValidateCategories(const Array::Ptr& value, const ValidationUtils& utils)
{
	ObjectImpl<DbConnection>::ValidateCategories(value, utils);

	int filter = FilterArrayToInt(value, DbQuery::GetCategoryFilterMap(), 0);

	if (filter != DbCatEverything && (filter & ~(DbCatInvalid | DbCatEverything | DbCatConfig | DbCatState |
		DbCatAcknowledgement | DbCatComment | DbCatDowntime | DbCatEventHandler | DbCatExternalCommand |
		DbCatFlapping | DbCatLog | DbCatNotification | DbCatProgramStatus | DbCatRetention |
		DbCatStateHistory)) != 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "categories" }, "categories filter is invalid."));
}

void DbConnection::IncreaseQueryCount()
{
	double now = Utility::GetTime();

	boost::mutex::scoped_lock lock(m_StatsMutex);
	m_QueryStats.InsertValue(now, 1);
}

int DbConnection::GetQueryCount(RingBuffer::SizeType span)
{
	boost::mutex::scoped_lock lock(m_StatsMutex);
	return m_QueryStats.UpdateAndGetValues(Utility::GetTime(), span);
}

bool DbConnection::IsIDCacheValid() const
{
	return m_IDCacheValid;
}

void DbConnection::SetIDCacheValid(bool valid)
{
	m_IDCacheValid = valid;
}

int DbConnection::GetSessionToken()
{
	return Application::GetStartTime();
}
