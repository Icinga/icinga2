// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "db_ido_pgsql/idopgsqlconnection.hpp"
#include "db_ido_pgsql/idopgsqlconnection-ti.cpp"
#include "db_ido/dbtype.hpp"
#include "db_ido/dbvalue.hpp"
#include "base/logger.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/application.hpp"
#include "base/configtype.hpp"
#include "base/exception.hpp"
#include "base/context.hpp"
#include "base/statsfunction.hpp"
#include "base/defer.hpp"
#include <cmath>
#include <utility>

using namespace icinga;

REGISTER_TYPE(IdoPgsqlConnection);

REGISTER_STATSFUNCTION(IdoPgsqlConnection, &IdoPgsqlConnection::StatsFunc);

const char * IdoPgsqlConnection::GetLatestSchemaVersion() const noexcept
{
	return "1.14.3";
}

const char * IdoPgsqlConnection::GetCompatSchemaVersion() const noexcept
{
	return "1.14.3";
}

IdoPgsqlConnection::IdoPgsqlConnection()
{
	m_QueryQueue.SetName("IdoPgsqlConnection, " + GetName());
}

void IdoPgsqlConnection::OnConfigLoaded()
{
	ObjectImpl<IdoPgsqlConnection>::OnConfigLoaded();

	m_QueryQueue.SetName("IdoPgsqlConnection, " + GetName());

	Library shimLibrary{"pgsql_shim"};

	auto create_pgsql_shim = shimLibrary.GetSymbolAddress<create_pgsql_shim_ptr>("create_pgsql_shim");

	m_Pgsql.reset(create_pgsql_shim());

	std::swap(m_Library, shimLibrary);
}

void IdoPgsqlConnection::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	DictionaryData nodes;

	for (const IdoPgsqlConnection::Ptr& idopgsqlconnection : ConfigType::GetObjectsByType<IdoPgsqlConnection>()) {
		size_t queryQueueItems = idopgsqlconnection->m_QueryQueue.GetLength();
		double queryQueueItemRate = idopgsqlconnection->m_QueryQueue.GetTaskCount(60) / 60.0;

		nodes.emplace_back(idopgsqlconnection->GetName(), new Dictionary({
			{ "version", idopgsqlconnection->GetSchemaVersion() },
			{ "instance_name", idopgsqlconnection->GetInstanceName() },
			{ "connected", idopgsqlconnection->GetConnected() },
			{ "query_queue_items", queryQueueItems },
			{ "query_queue_item_rate", queryQueueItemRate }
		}));

		perfdata->Add(new PerfdataValue("idopgsqlconnection_" + idopgsqlconnection->GetName() + "_queries_rate", idopgsqlconnection->GetQueryCount(60) / 60.0));
		perfdata->Add(new PerfdataValue("idopgsqlconnection_" + idopgsqlconnection->GetName() + "_queries_1min", idopgsqlconnection->GetQueryCount(60)));
		perfdata->Add(new PerfdataValue("idopgsqlconnection_" + idopgsqlconnection->GetName() + "_queries_5mins", idopgsqlconnection->GetQueryCount(5 * 60)));
		perfdata->Add(new PerfdataValue("idopgsqlconnection_" + idopgsqlconnection->GetName() + "_queries_15mins", idopgsqlconnection->GetQueryCount(15 * 60)));
		perfdata->Add(new PerfdataValue("idopgsqlconnection_" + idopgsqlconnection->GetName() + "_query_queue_items", queryQueueItems));
		perfdata->Add(new PerfdataValue("idopgsqlconnection_" + idopgsqlconnection->GetName() + "_query_queue_item_rate", queryQueueItemRate));
	}

	status->Set("idopgsqlconnection", new Dictionary(std::move(nodes)));
}

void IdoPgsqlConnection::Resume()
{
	Log(LogInformation, "IdoPgsqlConnection")
		<< "'" << GetName() << "' resumed.";

	SetConnected(false);

	m_QueryQueue.SetExceptionCallback([this](boost::exception_ptr exp) { ExceptionHandler(std::move(exp)); });

	/* Immediately try to connect on Resume() without timer. */
	m_QueryQueue.Enqueue([this]() { Reconnect(); }, PriorityImmediate);

	m_TxTimer = Timer::Create();
	m_TxTimer->SetInterval(1);
	m_TxTimer->OnTimerExpired.connect([this](const Timer * const&) { NewTransaction(); });
	m_TxTimer->Start();

	m_ReconnectTimer = Timer::Create();
	m_ReconnectTimer->SetInterval(10);
	m_ReconnectTimer->OnTimerExpired.connect([this](const Timer * const&) { ReconnectTimerHandler(); });
	m_ReconnectTimer->Start();

	/* Start with queries after connect. */
	DbConnection::Resume();

	ASSERT(m_Pgsql->isthreadsafe());
}

void IdoPgsqlConnection::Pause()
{
	DbConnection::Pause();

	m_ReconnectTimer->Stop(true);
	m_TxTimer->Stop(true);

	Log(LogInformation, "IdoPgsqlConnection")
		<< "'" << GetName() << "' paused.";
}

void IdoPgsqlConnection::ExceptionHandler(boost::exception_ptr exp)
{
	Log(LogWarning, "IdoPgsqlConnection", "Exception during database operation: Verify that your database is operational!");

	Log(LogDebug, "IdoPgsqlConnection")
		<< "Exception during database operation: " << DiagnosticInformation(std::move(exp));

	if (GetConnected()) {
		m_Pgsql->finish(m_Connection);
		SetConnected(false);
	}
}

void IdoPgsqlConnection::AssertOnWorkQueue()
{
	ASSERT(m_QueryQueue.IsWorkerThread());
}

void IdoPgsqlConnection::Disconnect()
{
	AssertOnWorkQueue();

	if (!GetConnected())
		return;

	IncreasePendingQueries(1);
	Query("COMMIT");

	m_Pgsql->finish(m_Connection);
	SetConnected(false);

	Log(LogInformation, "IdoPgsqlConnection")
		<< "Disconnected from '" << GetName() << "' database '" << GetDatabase() << "'.";
}

void IdoPgsqlConnection::NewTransaction()
{
	if (IsPaused())
		return;

	m_QueryQueue.Enqueue([this]() { InternalNewTransaction(); }, PriorityNormal, true);
}

void IdoPgsqlConnection::InternalNewTransaction()
{
	AssertOnWorkQueue();

	if (!GetConnected())
		return;

	IncreasePendingQueries(2);
	Query("COMMIT");
	Query("BEGIN");
}

void IdoPgsqlConnection::ReconnectTimerHandler()
{
	/* Only allow Reconnect events with high priority. */
	m_QueryQueue.Enqueue([this]() { Reconnect(); }, PriorityHigh);
}

void IdoPgsqlConnection::Reconnect()
{
	AssertOnWorkQueue();

	CONTEXT("Reconnecting to PostgreSQL IDO database '" << GetName() << "'");

	double startTime = Utility::GetTime();

	SetShouldConnect(true);

	bool reconnect = false;

	if (GetConnected()) {
		/* Check if we're really still connected */
		try {
			IncreasePendingQueries(1);
			Query("SELECT 1");
			return;
		} catch (const std::exception&) {
			m_Pgsql->finish(m_Connection);
			SetConnected(false);
			reconnect = true;
		}
	}

	ClearIDCache();

	String host = GetHost();
	String port = GetPort();
	String user = GetUser();
	String password = GetPassword();
	String database = GetDatabase();

	String sslMode = GetSslMode();
	String sslKey = GetSslKey();
	String sslCert = GetSslCert();
	String sslCa = GetSslCa();

	String conninfo;

	if (!host.IsEmpty())
		conninfo += " host=" + host;
	if (!port.IsEmpty())
		conninfo += " port=" + port;
	if (!user.IsEmpty())
		conninfo += " user=" + user;
	if (!password.IsEmpty())
		conninfo += " password=" + password;
	if (!database.IsEmpty())
		conninfo += " dbname=" + database;

	if (!sslMode.IsEmpty())
		conninfo += " sslmode=" + sslMode;
	if (!sslKey.IsEmpty())
		conninfo += " sslkey=" + sslKey;
	if (!sslCert.IsEmpty())
		conninfo += " sslcert=" + sslCert;
	if (!sslCa.IsEmpty())
		conninfo += " sslrootcert=" + sslCa;

	/* connection */
	m_Connection = m_Pgsql->connectdb(conninfo.CStr());

	if (!m_Connection)
		return;

	if (m_Pgsql->status(m_Connection) != CONNECTION_OK) {
		String message = m_Pgsql->errorMessage(m_Connection);
		m_Pgsql->finish(m_Connection);
		SetConnected(false);

		Log(LogCritical, "IdoPgsqlConnection")
			<< "Connection to database '" << database << "' with user '" << user << "' on '" << host << ":" << port
			<< "' failed: \"" << message << "\"";

		BOOST_THROW_EXCEPTION(std::runtime_error(message));
	}

	SetConnected(true);

	IdoPgsqlResult result;

	String dbVersionName = "idoutils";
	IncreasePendingQueries(1);
	result = Query("SELECT version FROM " + GetTablePrefix() + "dbversion WHERE name='" + Escape(dbVersionName) + "'");

	Dictionary::Ptr row = FetchRow(result, 0);

	if (!row) {
		m_Pgsql->finish(m_Connection);
		SetConnected(false);

		Log(LogCritical, "IdoPgsqlConnection", "Schema does not provide any valid version! Verify your schema installation.");

		BOOST_THROW_EXCEPTION(std::runtime_error("Invalid schema."));
	}

	String version = row->Get("version");

	SetSchemaVersion(version);

	if (Utility::CompareVersion(GetCompatSchemaVersion(), version) < 0) {
		m_Pgsql->finish(m_Connection);
		SetConnected(false);

		Log(LogCritical, "IdoPgsqlConnection")
			<< "Schema version '" << version << "' does not match the required version '"
			<< GetCompatSchemaVersion() << "' (or newer)! Please check the upgrade documentation at "
			<< "https://icinga.com/docs/icinga2/latest/doc/16-upgrading-icinga-2/#upgrading-postgresql-db";

		BOOST_THROW_EXCEPTION(std::runtime_error("Schema version mismatch."));
	}

	String instanceName = GetInstanceName();

	IncreasePendingQueries(1);
	result = Query("SELECT instance_id FROM " + GetTablePrefix() + "instances WHERE instance_name = '" + Escape(instanceName) + "'");
	row = FetchRow(result, 0);

	if (!row) {
		IncreasePendingQueries(1);
		Query("INSERT INTO " + GetTablePrefix() + "instances (instance_name, instance_description) VALUES ('" + Escape(instanceName) + "', '" + Escape(GetInstanceDescription()) + "')");
		m_InstanceID = GetSequenceValue(GetTablePrefix() + "instances", "instance_id");
	} else {
		m_InstanceID = DbReference(row->Get("instance_id"));
	}

	Endpoint::Ptr my_endpoint = Endpoint::GetLocalEndpoint();

	/* we have an endpoint in a cluster setup, so decide if we can proceed here */
	if (my_endpoint && GetHAMode() == HARunOnce) {
		/* get the current endpoint writing to programstatus table */
		IncreasePendingQueries(1);
		result = Query("SELECT UNIX_TIMESTAMP(status_update_time) AS status_update_time, endpoint_name FROM " +
			GetTablePrefix() + "programstatus WHERE instance_id = " + Convert::ToString(m_InstanceID));
		row = FetchRow(result, 0);

		String endpoint_name;

		if (row)
			endpoint_name = row->Get("endpoint_name");
		else
			Log(LogNotice, "IdoPgsqlConnection", "Empty program status table");

		/* if we did not write into the database earlier, another instance is active */
		if (endpoint_name != my_endpoint->GetName()) {
			double status_update_time;

			if (row)
				status_update_time = row->Get("status_update_time");
			else
				status_update_time = 0;

			double now = Utility::GetTime();

			double status_update_age = now - status_update_time;
			double failoverTimeout = GetFailoverTimeout();

			if (status_update_age < GetFailoverTimeout()) {
				Log(LogInformation, "IdoPgsqlConnection")
					<< "Last update by endpoint '" << endpoint_name << "' was "
					<< status_update_age << "s ago (< failover timeout of " << failoverTimeout << "s). Retrying.";

				m_Pgsql->finish(m_Connection);
				SetConnected(false);
				SetShouldConnect(false);

				return;
			}

			/* activate the IDO only, if we're authoritative in this zone */
			if (IsPaused()) {
				Log(LogNotice, "IdoPgsqlConnection")
					<< "Local endpoint '" << my_endpoint->GetName() << "' is not authoritative, bailing out.";

				m_Pgsql->finish(m_Connection);
				SetConnected(false);

				return;
			}

			SetLastFailover(now);

			Log(LogInformation, "IdoPgsqlConnection")
				<< "Last update by endpoint '" << endpoint_name << "' was "
				<< status_update_age << "s ago. Taking over '" << GetName() << "' in HA zone '" << Zone::GetLocalZone()->GetName() << "'.";
		}

		Log(LogNotice, "IdoPgsqlConnection", "Enabling IDO connection.");
	}

	Log(LogInformation, "IdoPgsqlConnection")
		<< "PGSQL IDO instance id: " << static_cast<long>(m_InstanceID) << " (schema version: '" + version + "')"
		<< (!sslMode.IsEmpty() ? ", sslmode='" + sslMode + "'" : "");

	IncreasePendingQueries(1);
	Query("BEGIN");

	/* update programstatus table */
	UpdateProgramStatus();

	/* record connection */
	IncreasePendingQueries(1);
	Query("INSERT INTO " + GetTablePrefix() + "conninfo " +
		"(instance_id, connect_time, last_checkin_time, agent_name, agent_version, connect_type, data_start_time) VALUES ("
		+ Convert::ToString(static_cast<long>(m_InstanceID)) + ", NOW(), NOW(), 'icinga2 db_ido_pgsql', '" + Escape(Application::GetAppVersion())
		+ "', '" + (reconnect ? "RECONNECT" : "INITIAL") + "', NOW())");

	/* clear config tables for the initial config dump */
	PrepareDatabase();

	std::ostringstream q1buf;
	q1buf << "SELECT object_id, objecttype_id, name1, name2, is_active FROM " + GetTablePrefix() + "objects WHERE instance_id = " << static_cast<long>(m_InstanceID);
	IncreasePendingQueries(1);
	result = Query(q1buf.str());

	std::vector<DbObject::Ptr> activeDbObjs;

	int index = 0;
	while ((row = FetchRow(result, index))) {
		index++;

		DbType::Ptr dbtype = DbType::GetByID(row->Get("objecttype_id"));

		if (!dbtype)
			continue;

		DbObject::Ptr dbobj = dbtype->GetOrCreateObjectByName(row->Get("name1"), row->Get("name2"));
		SetObjectID(dbobj, DbReference(row->Get("object_id")));
		bool active = row->Get("is_active");
		SetObjectActive(dbobj, active);

		if (active)
			activeDbObjs.push_back(dbobj);
	}

	SetIDCacheValid(true);

	EnableActiveChangedHandler();

	for (const DbObject::Ptr& dbobj : activeDbObjs) {
		if (dbobj->GetObject())
			continue;

		Log(LogNotice, "IdoPgsqlConnection")
			<< "Deactivate deleted object name1: '" << dbobj->GetName1()
			<< "' name2: '" << dbobj->GetName2() + "'.";
		DeactivateObject(dbobj);
	}

	UpdateAllObjects();

	m_QueryQueue.Enqueue([this]() { ClearTablesBySession(); }, PriorityNormal);

	m_QueryQueue.Enqueue([this, startTime]() { FinishConnect(startTime); }, PriorityNormal);
}

void IdoPgsqlConnection::FinishConnect(double startTime)
{
	AssertOnWorkQueue();

	if (!GetConnected())
		return;

	Log(LogInformation, "IdoPgsqlConnection")
		<< "Finished reconnecting to '" << GetName() << "' database '" << GetDatabase() << "' in "
		<< std::setw(2) << Utility::GetTime() - startTime << " second(s).";

	IncreasePendingQueries(2);
	Query("COMMIT");
	Query("BEGIN");
}

void IdoPgsqlConnection::ClearTablesBySession()
{
	/* delete all comments and downtimes without current session token */
	ClearTableBySession("comments");
	ClearTableBySession("scheduleddowntime");
}

void IdoPgsqlConnection::ClearTableBySession(const String& table)
{
	IncreasePendingQueries(1);
	Query("DELETE FROM " + GetTablePrefix() + table + " WHERE instance_id = " +
		Convert::ToString(static_cast<long>(m_InstanceID)) + " AND session_token <> " +
		Convert::ToString(GetSessionToken()));
}

IdoPgsqlResult IdoPgsqlConnection::Query(const String& query)
{
	AssertOnWorkQueue();

	Defer decreaseQueries ([this]() { DecreasePendingQueries(1); });

	Log(LogDebug, "IdoPgsqlConnection")
		<< "Query: " << query;

	IncreaseQueryCount();

	PGresult *result = m_Pgsql->exec(m_Connection, query.CStr());

	if (!result) {
		String message = m_Pgsql->errorMessage(m_Connection);
		Log(LogCritical, "IdoPgsqlConnection")
			<< "Error \"" << message << "\" when executing query \"" << query << "\"";

		BOOST_THROW_EXCEPTION(
			database_error()
			<< errinfo_message(message)
			<< errinfo_database_query(query)
		);
	}

	char *rowCount = m_Pgsql->cmdTuples(result);
	m_AffectedRows = atoi(rowCount);

	if (m_Pgsql->resultStatus(result) == PGRES_COMMAND_OK) {
		m_Pgsql->clear(result);
		return IdoPgsqlResult();
	}

	if (m_Pgsql->resultStatus(result) != PGRES_TUPLES_OK) {
		String message = m_Pgsql->resultErrorMessage(result);
		m_Pgsql->clear(result);

		Log(LogCritical, "IdoPgsqlConnection")
			<< "Error \"" << message << "\" when executing query \"" << query << "\"";

		BOOST_THROW_EXCEPTION(
			database_error()
			<< errinfo_message(message)
			<< errinfo_database_query(query)
		);
	}

	return IdoPgsqlResult(result, [this](PGresult* result) { m_Pgsql->clear(result); });
}

DbReference IdoPgsqlConnection::GetSequenceValue(const String& table, const String& column)
{
	AssertOnWorkQueue();

	IncreasePendingQueries(1);
	IdoPgsqlResult result = Query("SELECT CURRVAL(pg_get_serial_sequence('" + Escape(table) + "', '" + Escape(column) + "')) AS id");

	Dictionary::Ptr row = FetchRow(result, 0);

	ASSERT(row);

	Log(LogDebug, "IdoPgsqlConnection")
		<< "Sequence Value: " << row->Get("id");

	return {Convert::ToLong(row->Get("id"))};
}

int IdoPgsqlConnection::GetAffectedRows()
{
	AssertOnWorkQueue();

	return m_AffectedRows;
}

String IdoPgsqlConnection::Escape(const String& s)
{
	AssertOnWorkQueue();

	String utf8s = Utility::ValidateUTF8(s);

	size_t length = utf8s.GetLength();
	auto *to = new char[utf8s.GetLength() * 2 + 1];

	m_Pgsql->escapeStringConn(m_Connection, to, utf8s.CStr(), length, nullptr);

	String result = String(to);

	delete [] to;

	return result;
}

Dictionary::Ptr IdoPgsqlConnection::FetchRow(const IdoPgsqlResult& result, int row)
{
	AssertOnWorkQueue();

	if (row >= m_Pgsql->ntuples(result.get()))
		return nullptr;

	int columns = m_Pgsql->nfields(result.get());

	DictionaryData dict;

	for (int column = 0; column < columns; column++) {
		Value value;

		if (!m_Pgsql->getisnull(result.get(), row, column))
			value = m_Pgsql->getvalue(result.get(), row, column);

		dict.emplace_back(m_Pgsql->fname(result.get(), column), value);
	}

	return new Dictionary(std::move(dict));
}

void IdoPgsqlConnection::ActivateObject(const DbObject::Ptr& dbobj)
{
	if (IsPaused())
		return;

	m_QueryQueue.Enqueue([this, dbobj]() { InternalActivateObject(dbobj); }, PriorityNormal);
}

void IdoPgsqlConnection::InternalActivateObject(const DbObject::Ptr& dbobj)
{
	AssertOnWorkQueue();

	if (!GetConnected())
		return;

	DbReference dbref = GetObjectID(dbobj);
	std::ostringstream qbuf;

	if (!dbref.IsValid()) {
		if (!dbobj->GetName2().IsEmpty()) {
			qbuf << "INSERT INTO " + GetTablePrefix() + "objects (instance_id, objecttype_id, name1, name2, is_active) VALUES ("
				<< static_cast<long>(m_InstanceID) << ", " << dbobj->GetType()->GetTypeID() << ", "
				<< "'" << Escape(dbobj->GetName1()) << "', '" << Escape(dbobj->GetName2()) << "', 1)";
		} else {
			qbuf << "INSERT INTO " + GetTablePrefix() + "objects (instance_id, objecttype_id, name1, is_active) VALUES ("
				<< static_cast<long>(m_InstanceID) << ", " << dbobj->GetType()->GetTypeID() << ", "
				<< "'" << Escape(dbobj->GetName1()) << "', 1)";
		}

		IncreasePendingQueries(1);
		Query(qbuf.str());
		SetObjectID(dbobj, GetSequenceValue(GetTablePrefix() + "objects", "object_id"));
	} else {
		qbuf << "UPDATE " + GetTablePrefix() + "objects SET is_active = 1 WHERE object_id = " << static_cast<long>(dbref);
		IncreasePendingQueries(1);
		Query(qbuf.str());
	}
}

void IdoPgsqlConnection::DeactivateObject(const DbObject::Ptr& dbobj)
{
	if (IsPaused())
		return;

	m_QueryQueue.Enqueue([this, dbobj]() { InternalDeactivateObject(dbobj); }, PriorityNormal);
}

void IdoPgsqlConnection::InternalDeactivateObject(const DbObject::Ptr& dbobj)
{
	AssertOnWorkQueue();

	if (!GetConnected())
		return;

	DbReference dbref = GetObjectID(dbobj);

	if (!dbref.IsValid())
		return;

	std::ostringstream qbuf;
	qbuf << "UPDATE " + GetTablePrefix() + "objects SET is_active = 0 WHERE object_id = " << static_cast<long>(dbref);
	IncreasePendingQueries(1);
	Query(qbuf.str());

	/* Note that we're _NOT_ clearing the db refs via SetReference/SetConfigUpdate/SetStatusUpdate
	 * because the object is still in the database. */

	SetObjectActive(dbobj, false);
}

bool IdoPgsqlConnection::FieldToEscapedString(const String& key, const Value& value, Value *result)
{
	if (key == "instance_id") {
		*result = static_cast<long>(m_InstanceID);
		return true;
	} else if (key == "session_token") {
		*result = GetSessionToken();
		return true;
	}

	Value rawvalue = DbValue::ExtractValue(value);

	if (rawvalue.GetType() == ValueEmpty) {
		*result = "NULL";
	} else if (rawvalue.IsObjectType<ConfigObject>()) {
		DbObject::Ptr dbobjcol = DbObject::GetOrCreateByObject(rawvalue);

		if (!dbobjcol) {
			*result = 0;
			return true;
		}

		if (!IsIDCacheValid())
			return false;

		DbReference dbrefcol;

		if (DbValue::IsObjectInsertID(value)) {
			dbrefcol = GetInsertID(dbobjcol);

			if (!dbrefcol.IsValid())
				return false;
		} else {
			dbrefcol = GetObjectID(dbobjcol);

			if (!dbrefcol.IsValid()) {
				InternalActivateObject(dbobjcol);

				dbrefcol = GetObjectID(dbobjcol);

				if (!dbrefcol.IsValid())
					return false;
			}
		}

		*result = static_cast<long>(dbrefcol);
	} else if (DbValue::IsTimestamp(value)) {
		// In addition to the limits of PostgreSQL itself (4713BC - 294276AD),
		// years not fitting in YYYY may cause problems, see e.g. https://github.com/golang/go/issues/4556.
		// RFC 3339: "All dates and times are assumed to be (...) somewhere between 0000AD and 9999AD."
		// The below limits include safety buffers to make sure the timestamps are within 0-9999 AD in all time zones:
		//
		// postgres=# \x
		// Expanded display is on.
		// postgres=# SELECT TO_TIMESTAMP(-62135510400) AT TIME ZONE 'UTC' AS utc,
		// postgres-# TO_TIMESTAMP(-62135510400) AT TIME ZONE 'Asia/Vladivostok' AS east,
		// postgres-# TO_TIMESTAMP(-62135510400) AT TIME ZONE 'America/Juneau' AS west,
		// postgres-# TO_TIMESTAMP(-62135510400) AT TIME ZONE 'America/Nuuk' AS north;
		// -[ RECORD 1 ]--------------
		// utc   | 0001-01-02 00:00:00
		// east  | 0001-01-02 08:47:31
		// west  | 0001-01-02 15:02:19
		// north | 0001-01-01 20:33:04
		//
		// postgres=# SELECT TO_TIMESTAMP(-62135510400-86400) AT TIME ZONE 'UTC' AS utc,
		// postgres-# TO_TIMESTAMP(-62135510400-86400) AT TIME ZONE 'Asia/Vladivostok' AS east,
		// postgres-# TO_TIMESTAMP(-62135510400-86400) AT TIME ZONE 'America/Juneau' AS west,
		// postgres-# TO_TIMESTAMP(-62135510400-86400) AT TIME ZONE 'America/Nuuk' AS north;
		// -[ RECORD 1 ]-----------------
		// utc   | 0001-01-01 00:00:00
		// east  | 0001-01-01 08:47:31
		// west  | 0001-01-01 15:02:19
		// north | 0001-12-31 20:33:04 BC
		//
		// postgres=# SELECT TO_TIMESTAMP(253402214400) AT TIME ZONE 'UTC' AS utc,
		// postgres-# TO_TIMESTAMP(253402214400) AT TIME ZONE 'Asia/Vladivostok' AS east,
		// postgres-# TO_TIMESTAMP(253402214400) AT TIME ZONE 'America/Juneau' AS west,
		// postgres-# TO_TIMESTAMP(253402214400) AT TIME ZONE 'America/Nuuk' AS north;
		// -[ RECORD 1 ]-------------
		// utc   | 9999-12-31 00:00:00
		// east  | 9999-12-31 10:00:00
		// west  | 9999-12-30 15:00:00
		// north | 9999-12-30 22:00:00
		//
		// postgres=#

		double ts = rawvalue;
		std::ostringstream msgbuf;
		msgbuf << "TO_TIMESTAMP(" << std::fixed << std::setprecision(0)
			<< std::fmin(std::fmax(ts, -62135510400.0), 253402214400.0) << ") AT TIME ZONE 'UTC'";
		*result = Value(msgbuf.str());
	} else if (DbValue::IsObjectInsertID(value)) {
		auto id = static_cast<long>(rawvalue);

		if (id <= 0)
			return false;

		*result = id;
		return true;
	} else {
		Value fvalue;

		if (rawvalue.IsBoolean())
			fvalue = Convert::ToLong(rawvalue);
		else
			fvalue = rawvalue;

		*result = "'" + Escape(fvalue) + "'";
	}

	return true;
}

void IdoPgsqlConnection::ExecuteQuery(const DbQuery& query)
{
	if (IsPaused() && GetPauseCalled())
		return;

	ASSERT(query.Category != DbCatInvalid);

	IncreasePendingQueries(1);
	m_QueryQueue.Enqueue([this, query]() { InternalExecuteQuery(query, -1); }, query.Priority, true);
}

void IdoPgsqlConnection::ExecuteMultipleQueries(const std::vector<DbQuery>& queries)
{
	if (IsPaused())
		return;

	if (queries.empty())
		return;

	IncreasePendingQueries(queries.size());
	m_QueryQueue.Enqueue([this, queries]() { InternalExecuteMultipleQueries(queries); }, queries[0].Priority, true);
}

bool IdoPgsqlConnection::CanExecuteQuery(const DbQuery& query)
{
	if (query.Object && !IsIDCacheValid())
		return false;

	if (query.WhereCriteria) {
		ObjectLock olock(query.WhereCriteria);
		Value value;

		for (const Dictionary::Pair& kv : query.WhereCriteria) {
			if (!FieldToEscapedString(kv.first, kv.second, &value))
				return false;
		}
	}

	if (query.Fields) {
		ObjectLock olock(query.Fields);

		for (const Dictionary::Pair& kv : query.Fields) {
			Value value;

			if (!FieldToEscapedString(kv.first, kv.second, &value))
				return false;
		}
	}

	return true;
}

void IdoPgsqlConnection::InternalExecuteMultipleQueries(const std::vector<DbQuery>& queries)
{
	AssertOnWorkQueue();

	if (IsPaused()) {
		DecreasePendingQueries(queries.size());
		return;
	}

	if (!GetConnected()) {
		DecreasePendingQueries(queries.size());
		return;
	}

	for (const DbQuery& query : queries) {
		ASSERT(query.Type == DbQueryNewTransaction || query.Category != DbCatInvalid);

		if (!CanExecuteQuery(query)) {
			m_QueryQueue.Enqueue([this, queries]() { InternalExecuteMultipleQueries(queries); }, query.Priority);
			return;
		}
	}

	for (const DbQuery& query : queries) {
		InternalExecuteQuery(query);
	}
}

void IdoPgsqlConnection::InternalExecuteQuery(const DbQuery& query, int typeOverride)
{
	AssertOnWorkQueue();

	if (IsPaused() && GetPauseCalled()) {
		DecreasePendingQueries(1);
		return;
	}

	if (!GetConnected()) {
		DecreasePendingQueries(1);
		return;
	}

	if (query.Type == DbQueryNewTransaction) {
		DecreasePendingQueries(1);
		InternalNewTransaction();
		return;
	}

	/* check whether we're allowed to execute the query first */
	if (GetCategoryFilter() != DbCatEverything && (query.Category & GetCategoryFilter()) == 0) {
		DecreasePendingQueries(1);
		return;
	}

	if (query.Object && query.Object->GetObject()->GetExtension("agent_check").ToBool()) {
		DecreasePendingQueries(1);
		return;
	}

	/* check if there are missing object/insert ids and re-enqueue the query */
	if (!CanExecuteQuery(query)) {
		m_QueryQueue.Enqueue([this, query, typeOverride]() { InternalExecuteQuery(query, typeOverride); }, query.Priority);
		return;
	}

	std::ostringstream qbuf, where;
	int type;

	if (query.WhereCriteria) {
		where << " WHERE ";

		ObjectLock olock(query.WhereCriteria);
		Value value;
		bool first = true;

		for (const Dictionary::Pair& kv : query.WhereCriteria) {
			if (!FieldToEscapedString(kv.first, kv.second, &value)) {
				m_QueryQueue.Enqueue([this, query]() { InternalExecuteQuery(query, -1); }, query.Priority);
				return;
			}

			if (!first)
				where << " AND ";

			where << kv.first << " = " << value;

			if (first)
				first = false;
		}
	}

	type = (typeOverride != -1) ? typeOverride : query.Type;

	bool upsert = false;

	if ((type & DbQueryInsert) && (type & DbQueryUpdate)) {
		bool hasid = false;

		if (query.Object) {
			if (query.ConfigUpdate)
				hasid = GetConfigUpdate(query.Object);
			else if (query.StatusUpdate)
				hasid = GetStatusUpdate(query.Object);
		}

		if (!hasid)
			upsert = true;

		type = DbQueryUpdate;
	}

	if ((type & DbQueryInsert) && (type & DbQueryDelete)) {
		std::ostringstream qdel;
		qdel << "DELETE FROM " << GetTablePrefix() << query.Table << where.str();
		IncreasePendingQueries(1);
		Query(qdel.str());

		type = DbQueryInsert;
	}

	switch (type) {
		case DbQueryInsert:
			qbuf << "INSERT INTO " << GetTablePrefix() << query.Table;
			break;
		case DbQueryUpdate:
			qbuf << "UPDATE " << GetTablePrefix() << query.Table << " SET";
			break;
		case DbQueryDelete:
			qbuf << "DELETE FROM " << GetTablePrefix() << query.Table;
			break;
		default:
			VERIFY(!"Invalid query type.");
	}

	if (type == DbQueryInsert || type == DbQueryUpdate) {
		std::ostringstream colbuf, valbuf;

		if (type == DbQueryUpdate && query.Fields->GetLength() == 0)
			return;

		ObjectLock olock(query.Fields);

		Value value;
		bool first = true;
		for (const Dictionary::Pair& kv : query.Fields) {
			if (!FieldToEscapedString(kv.first, kv.second, &value)) {
				m_QueryQueue.Enqueue([this, query]() { InternalExecuteQuery(query, -1); }, query.Priority);
				return;
			}

			if (type == DbQueryInsert) {
				if (!first) {
					colbuf << ", ";
					valbuf << ", ";
				}

				colbuf << kv.first;
				valbuf << value;
			} else {
				if (!first)
					qbuf << ", ";

				qbuf << " " << kv.first << " = " << value;
			}

			if (first)
				first = false;
		}

		if (type == DbQueryInsert)
			qbuf << " (" << colbuf.str() << ") VALUES (" << valbuf.str() << ")";
	}

	if (type != DbQueryInsert)
		qbuf << where.str();

	Query(qbuf.str());

	if (upsert && GetAffectedRows() == 0) {
		IncreasePendingQueries(1);
		InternalExecuteQuery(query, DbQueryDelete | DbQueryInsert);

		return;
	}

	if (type == DbQueryInsert && query.Object) {
		if (query.ConfigUpdate) {
			String idField = query.IdColumn;

			if (idField.IsEmpty())
				idField = query.Table.SubStr(0, query.Table.GetLength() - 1) + "_id";

			SetInsertID(query.Object, GetSequenceValue(GetTablePrefix() + query.Table, idField));

			SetConfigUpdate(query.Object, true);
		} else if (query.StatusUpdate)
			SetStatusUpdate(query.Object, true);
	}

	if (type == DbQueryInsert && query.Table == "notifications" && query.NotificationInsertID) {
		DbReference seqval = GetSequenceValue(GetTablePrefix() + query.Table, "notification_id");
		query.NotificationInsertID->SetValue(static_cast<long>(seqval));
	}
}

void IdoPgsqlConnection::CleanUpExecuteQuery(const String& table, const String& time_column, double max_age)
{
	if (IsPaused())
		return;

	IncreasePendingQueries(1);
	m_QueryQueue.Enqueue([this, table, time_column, max_age]() { InternalCleanUpExecuteQuery(table, time_column, max_age); }, PriorityLow, true);
}

void IdoPgsqlConnection::InternalCleanUpExecuteQuery(const String& table, const String& time_column, double max_age)
{
	AssertOnWorkQueue();

	if (!GetConnected()) {
		DecreasePendingQueries(1);
		return;
	}

	Query("DELETE FROM " + GetTablePrefix() + table + " WHERE instance_id = " +
		Convert::ToString(static_cast<long>(m_InstanceID)) + " AND " + time_column +
		" < TO_TIMESTAMP(" + Convert::ToString(static_cast<long>(max_age)) + ") AT TIME ZONE 'UTC'");
}

void IdoPgsqlConnection::FillIDCache(const DbType::Ptr& type)
{
	String query = "SELECT " + type->GetIDColumn() + " AS object_id, " + type->GetTable() + "_id, config_hash FROM " + GetTablePrefix() + type->GetTable() + "s";
	IncreasePendingQueries(1);
	IdoPgsqlResult result = Query(query);

	Dictionary::Ptr row;

	int index = 0;
	while ((row = FetchRow(result, index))) {
		index++;
		DbReference dbref(row->Get("object_id"));
		SetInsertID(type, dbref, DbReference(row->Get(type->GetTable() + "_id")));
		SetConfigHash(type, dbref, row->Get("config_hash"));
	}
}

int IdoPgsqlConnection::GetPendingQueryCount() const
{
	return m_QueryQueue.GetLength();
}
