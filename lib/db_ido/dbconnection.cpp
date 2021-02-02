/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "db_ido/dbconnection.hpp"
#include "db_ido/dbconnection-ti.cpp"
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

	m_LogStatsTimeout = 0;

	m_LogStatsTimer = new Timer();
	m_LogStatsTimer->SetInterval(10);
	m_LogStatsTimer->OnTimerExpired.connect([this](const Timer * const&) { LogStatsHandler(); });
	m_LogStatsTimer->Start();
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
	query1.WhereCriteria = new Dictionary({
		{ "instance_id", 0 }  /* DbConnection class fills in real ID */
	});

	query1.Fields = new Dictionary({
		{ "instance_id", 0 }, /* DbConnection class fills in real ID */
		{ "program_end_time", DbValue::FromTimestamp(Utility::GetTime()) }
	});

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
	query.Fields = new Dictionary({
		{ "instance_id", 0 }, /* DbConnection class fills in real ID */
		{ "varname", key },
		{ "varvalue", value }
	});
	DbObject::OnQuery(query);
}

void DbConnection::UpdateProgramStatus()
{
	IcingaApplication::Ptr icingaApplication = IcingaApplication::GetInstance();

	if (!icingaApplication)
		return;

	Log(LogNotice, "DbConnection")
		<< "Updating programstatus table.";

	std::vector<DbQuery> queries;

	DbQuery query1;
	query1.Table = "programstatus";
	query1.IdColumn = "programstatus_id";
	query1.Type = DbQueryInsert | DbQueryDelete;
	query1.Category = DbCatProgramStatus;

	query1.Fields = new Dictionary({
		{ "instance_id", 0 }, /* DbConnection class fills in real ID */
		{ "program_version", Application::GetAppVersion() },
		{ "status_update_time", DbValue::FromTimestamp(Utility::GetTime()) },
		{ "program_start_time", DbValue::FromTimestamp(Application::GetStartTime()) },
		{ "is_currently_running", 1 },
		{ "endpoint_name", icingaApplication->GetNodeName() },
		{ "process_id", Utility::GetPid() },
		{ "daemon_mode", 1 },
		{ "last_command_check", DbValue::FromTimestamp(Utility::GetTime()) },
		{ "notifications_enabled", (icingaApplication->GetEnableNotifications() ? 1 : 0) },
		{ "active_host_checks_enabled", (icingaApplication->GetEnableHostChecks() ? 1 : 0) },
		{ "passive_host_checks_enabled", 1 },
		{ "active_service_checks_enabled", (icingaApplication->GetEnableServiceChecks() ? 1 : 0) },
		{ "passive_service_checks_enabled", 1 },
		{ "event_handlers_enabled", (icingaApplication->GetEnableEventHandlers() ? 1 : 0) },
		{ "flap_detection_enabled", (icingaApplication->GetEnableFlapping() ? 1 : 0) },
		{ "process_performance_data", (icingaApplication->GetEnablePerfdata() ? 1 : 0) }
	});

	query1.WhereCriteria = new Dictionary({
		{ "instance_id", 0 }  /* DbConnection class fills in real ID */
	});

	query1.Priority = PriorityImmediate;
	queries.emplace_back(std::move(query1));

	DbQuery query2;
	query2.Type = DbQueryNewTransaction;
	queries.emplace_back(std::move(query2));

	DbObject::OnMultipleQueries(queries);

	DbQuery query3;
	query3.Table = "runtimevariables";
	query3.Type = DbQueryDelete;
	query3.Category = DbCatProgramStatus;
	query3.WhereCriteria = new Dictionary({
		{ "instance_id", 0 } /* DbConnection class fills in real ID */
	});
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

void DbConnection::LogStatsHandler()
{
	if (!GetConnected() || IsPaused())
		return;

	auto pending = m_PendingQueries.load();

	auto now = Utility::GetTime();
	bool timeoutReached = m_LogStatsTimeout < now;

	if (pending == 0u && !timeoutReached) {
		return;
	}

	auto output = round(m_OutputQueries.CalculateRate(now, 10));

	if (pending < output * 5 && !timeoutReached) {
		return;
	}

	auto input = round(m_InputQueries.CalculateRate(now, 10));

	Log(LogInformation, GetReflectionType()->GetName())
		<< "Pending queries: " << pending << " (Input: " << input
		<< "/s; Output: " << output << "/s)";

	/* Reschedule next log entry in 5 minutes. */
	if (timeoutReached) {
		m_LogStatsTimeout = now + 60 * 5;
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
		return {};

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
		return {};

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
	bool isShuttingDown = Application::IsShuttingDown();
	bool isRestarting = Application::IsRestarting();

#ifdef I2_DEBUG
	if (isShuttingDown || isRestarting) {
		//Log(LogDebug, "DbConnection")
		//	<< "Updating object '" << object->GetName() << "' \t\t active '" << Convert::ToLong(object->IsActive())
		//	<< "' shutting down '" << Convert::ToLong(isShuttingDown) << "' restarting '" << Convert::ToLong(isRestarting) << "'.";
	}
#endif /* I2_DEBUG */

	/* Wait until a database connection is established on reconnect. */
	if (!GetConnected())
		return;

	/* Don't update inactive objects during shutdown/reload/restart.
	 * They would be marked as deleted. This gets triggered with ConfigObject::StopObjects().
	 * During startup/reconnect this is fine, the handler is not active there.
	 */
	if (isShuttingDown || isRestarting)
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
			/* This may happen on reload/restart actions too
			 * and is blocked above already.
			 *
			 * Deactivate the deleted object no matter
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
			m_QueryQueue.Enqueue([this, object](){ UpdateObject(object); }, PriorityHigh);
		}
	}
}

void DbConnection::PrepareDatabase()
{
	for (const DbType::Ptr& type : DbType::GetAllTypes()) {
		FillIDCache(type);
	}
}

void DbConnection::ValidateFailoverTimeout(const Lazy<double>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<DbConnection>::ValidateFailoverTimeout(lvalue, utils);

	if (lvalue() < 30)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "failover_timeout" }, "Failover timeout minimum is 30s."));
}

void DbConnection::ValidateCategories(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<DbConnection>::ValidateCategories(lvalue, utils);

	int filter = FilterArrayToInt(lvalue(), DbQuery::GetCategoryFilterMap(), 0);

	if (filter != DbCatEverything && (filter & ~(DbCatInvalid | DbCatEverything | DbCatConfig | DbCatState |
		DbCatAcknowledgement | DbCatComment | DbCatDowntime | DbCatEventHandler | DbCatExternalCommand |
		DbCatFlapping | DbCatLog | DbCatNotification | DbCatProgramStatus | DbCatRetention |
		DbCatStateHistory)) != 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "categories" }, "categories filter is invalid."));
}

void DbConnection::IncreaseQueryCount()
{
	double now = Utility::GetTime();

	std::unique_lock<std::mutex> lock(m_StatsMutex);
	m_QueryStats.InsertValue(now, 1);
}

int DbConnection::GetQueryCount(RingBuffer::SizeType span)
{
	std::unique_lock<std::mutex> lock(m_StatsMutex);
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

void DbConnection::IncreasePendingQueries(int count)
{
	m_PendingQueries.fetch_add(count);
	m_InputQueries.InsertValue(Utility::GetTime(), count);
}

void DbConnection::DecreasePendingQueries(int count)
{
	m_PendingQueries.fetch_sub(count);
	m_OutputQueries.InsertValue(Utility::GetTime(), count);
}
