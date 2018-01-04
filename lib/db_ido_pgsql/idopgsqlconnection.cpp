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

#include "db_ido_pgsql/idopgsqlconnection.hpp"
#include "db_ido_pgsql/idopgsqlconnection.tcpp"
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
#include <boost/tuple/tuple.hpp>
#include <utility>

using namespace icinga;

REGISTER_TYPE(IdoPgsqlConnection);

REGISTER_STATSFUNCTION(IdoPgsqlConnection, &IdoPgsqlConnection::StatsFunc);

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
	Dictionary::Ptr nodes = new Dictionary();

	for (const IdoPgsqlConnection::Ptr& idopgsqlconnection : ConfigType::GetObjectsByType<IdoPgsqlConnection>()) {
		size_t queryQueueItems = idopgsqlconnection->m_QueryQueue.GetLength();
		double queryQueueItemRate = idopgsqlconnection->m_QueryQueue.GetTaskCount(60) / 60.0;

		Dictionary::Ptr stats = new Dictionary();
		stats->Set("version", idopgsqlconnection->GetSchemaVersion());
		stats->Set("instance_name", idopgsqlconnection->GetInstanceName());
		stats->Set("connected", idopgsqlconnection->GetConnected());
		stats->Set("query_queue_items", queryQueueItems);
		stats->Set("query_queue_item_rate", queryQueueItemRate);

		nodes->Set(idopgsqlconnection->GetName(), stats);

		perfdata->Add(new PerfdataValue("idopgsqlconnection_" + idopgsqlconnection->GetName() + "_queries_rate", idopgsqlconnection->GetQueryCount(60) / 60.0));
		perfdata->Add(new PerfdataValue("idopgsqlconnection_" + idopgsqlconnection->GetName() + "_queries_1min", idopgsqlconnection->GetQueryCount(60)));
		perfdata->Add(new PerfdataValue("idopgsqlconnection_" + idopgsqlconnection->GetName() + "_queries_5mins", idopgsqlconnection->GetQueryCount(5 * 60)));
		perfdata->Add(new PerfdataValue("idopgsqlconnection_" + idopgsqlconnection->GetName() + "_queries_15mins", idopgsqlconnection->GetQueryCount(15 * 60)));
		perfdata->Add(new PerfdataValue("idopgsqlconnection_" + idopgsqlconnection->GetName() + "_query_queue_items", queryQueueItems));
		perfdata->Add(new PerfdataValue("idopgsqlconnection_" + idopgsqlconnection->GetName() + "_query_queue_item_rate", queryQueueItemRate));
	}

	status->Set("idopgsqlconnection", nodes);
}

void IdoPgsqlConnection::Resume()
{
	DbConnection::Resume();

	Log(LogInformation, "IdoPgsqlConnection")
		<< "'" << GetName() << "' resumed.";

	SetConnected(false);

	m_QueryQueue.SetExceptionCallback(std::bind(&IdoPgsqlConnection::ExceptionHandler, this, _1));

	m_TxTimer = new Timer();
	m_TxTimer->SetInterval(1);
	m_TxTimer->OnTimerExpired.connect(std::bind(&IdoPgsqlConnection::TxTimerHandler, this));
	m_TxTimer->Start();

	m_ReconnectTimer = new Timer();
	m_ReconnectTimer->SetInterval(10);
	m_ReconnectTimer->OnTimerExpired.connect(std::bind(&IdoPgsqlConnection::ReconnectTimerHandler, this));
	m_ReconnectTimer->Start();
	m_ReconnectTimer->Reschedule(0);

	ASSERT(m_Pgsql->isthreadsafe());
}

void IdoPgsqlConnection::Pause()
{
	Log(LogInformation, "IdoPgsqlConnection")
		<< "'" << GetName() << "' paused.";

	m_ReconnectTimer.reset();

	DbConnection::Pause();

	m_QueryQueue.Enqueue(std::bind(&IdoPgsqlConnection::Disconnect, this), PriorityHigh);
	m_QueryQueue.Join();
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

	Query("COMMIT");

	m_Pgsql->finish(m_Connection);
	SetConnected(false);
}

void IdoPgsqlConnection::TxTimerHandler()
{
	NewTransaction();
}

void IdoPgsqlConnection::NewTransaction()
{
	m_QueryQueue.Enqueue(std::bind(&IdoPgsqlConnection::InternalNewTransaction, this), PriorityHigh, true);
}

void IdoPgsqlConnection::InternalNewTransaction()
{
	AssertOnWorkQueue();

	if (!GetConnected())
		return;

	Query("COMMIT");
	Query("BEGIN");
}

void IdoPgsqlConnection::ReconnectTimerHandler()
{
	m_QueryQueue.Enqueue(std::bind(&IdoPgsqlConnection::Reconnect, this), PriorityLow);
}

void IdoPgsqlConnection::Reconnect()
{
	AssertOnWorkQueue();

	CONTEXT("Reconnecting to PostgreSQL IDO database '" + GetName() + "'");

	double startTime = Utility::GetTime();

	SetShouldConnect(true);

	bool reconnect = false;

	if (GetConnected()) {
		/* Check if we're really still connected */
		try {
			Query("SELECT 1");
			return;
		} catch (const std::exception&) {
			m_Pgsql->finish(m_Connection);
			SetConnected(false);
			reconnect = true;
		}
	}

	ClearIDCache();

	String ihost, iport, iuser, ipasswd, idb;
	const char *host, *port, *user , *passwd, *db;

	ihost = GetHost();
	iport = GetPort();
	iuser = GetUser();
	ipasswd = GetPassword();
	idb = GetDatabase();

	host = (!ihost.IsEmpty()) ? ihost.CStr() : nullptr;
	port = (!iport.IsEmpty()) ? iport.CStr() : nullptr;
	user = (!iuser.IsEmpty()) ? iuser.CStr() : nullptr;
	passwd = (!ipasswd.IsEmpty()) ? ipasswd.CStr() : nullptr;
	db = (!idb.IsEmpty()) ? idb.CStr() : nullptr;

	m_Connection = m_Pgsql->setdbLogin(host, port, nullptr, nullptr, db, user, passwd);

	if (!m_Connection)
		return;

	if (m_Pgsql->status(m_Connection) != CONNECTION_OK) {
		String message = m_Pgsql->errorMessage(m_Connection);
		m_Pgsql->finish(m_Connection);
		SetConnected(false);

		Log(LogCritical, "IdoPgsqlConnection")
			<< "Connection to database '" << db << "' with user '" << user << "' on '" << host << ":" << port
			<< "' failed: \"" << message << "\"";

		BOOST_THROW_EXCEPTION(std::runtime_error(message));
	}

	SetConnected(true);

	IdoPgsqlResult result;

	/* explicitely require legacy mode for string escaping in PostgreSQL >= 9.1
	 * changing standard_conforming_strings to on by default
	 */
	if (m_Pgsql->serverVersion(m_Connection) >= 90100)
		result = Query("SET standard_conforming_strings TO off");

	String dbVersionName = "idoutils";
	result = Query("SELECT version FROM " + GetTablePrefix() + "dbversion WHERE name=E'" + Escape(dbVersionName) + "'");

	Dictionary::Ptr row = FetchRow(result, 0);

	if (!row) {
		m_Pgsql->finish(m_Connection);
		SetConnected(false);

		Log(LogCritical, "IdoPgsqlConnection", "Schema does not provide any valid version! Verify your schema installation.");

		BOOST_THROW_EXCEPTION(std::runtime_error("Invalid schema."));
	}

	String version = row->Get("version");

	SetSchemaVersion(version);

	if (Utility::CompareVersion(IDO_COMPAT_SCHEMA_VERSION, version) < 0) {
		m_Pgsql->finish(m_Connection);
		SetConnected(false);

		Log(LogCritical, "IdoPgsqlConnection")
			<< "Schema version '" << version << "' does not match the required version '"
			<< IDO_COMPAT_SCHEMA_VERSION << "' (or newer)! Please check the upgrade documentation at "
			<< "https://docs.icinga.com/icinga2/latest/doc/module/icinga2/chapter/upgrading-icinga-2#upgrading-postgresql-db";

		BOOST_THROW_EXCEPTION(std::runtime_error("Schema version mismatch."));
	}

	String instanceName = GetInstanceName();

	result = Query("SELECT instance_id FROM " + GetTablePrefix() + "instances WHERE instance_name = E'" + Escape(instanceName) + "'");
	row = FetchRow(result, 0);

	if (!row) {
		Query("INSERT INTO " + GetTablePrefix() + "instances (instance_name, instance_description) VALUES (E'" + Escape(instanceName) + "', E'" + Escape(GetInstanceDescription()) + "')");
		m_InstanceID = GetSequenceValue(GetTablePrefix() + "instances", "instance_id");
	} else {
		m_InstanceID = DbReference(row->Get("instance_id"));
	}

	Endpoint::Ptr my_endpoint = Endpoint::GetLocalEndpoint();

	/* we have an endpoint in a cluster setup, so decide if we can proceed here */
	if (my_endpoint && GetHAMode() == HARunOnce) {
		/* get the current endpoint writing to programstatus table */
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

			double status_update_age = Utility::GetTime() - status_update_time;

			Log(LogNotice, "IdoPgsqlConnection")
				<< "Last update by '" << endpoint_name << "' was " << status_update_age << "s ago.";

			if (status_update_age < GetFailoverTimeout()) {
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
		}

		Log(LogNotice, "IdoPgsqlConnection", "Enabling IDO connection.");
	}

	Log(LogInformation, "IdoPgsqlConnection")
		<< "pgSQL IDO instance id: " << static_cast<long>(m_InstanceID) << " (schema version: '" + version + "')";

	Query("BEGIN");

	/* update programstatus table */
	UpdateProgramStatus();

	/* record connection */
	Query("INSERT INTO " + GetTablePrefix() + "conninfo " +
		"(instance_id, connect_time, last_checkin_time, agent_name, agent_version, connect_type, data_start_time) VALUES ("
		+ Convert::ToString(static_cast<long>(m_InstanceID)) + ", NOW(), NOW(), E'icinga2 db_ido_pgsql', E'" + Escape(Application::GetAppVersion())
		+ "', E'" + (reconnect ? "RECONNECT" : "INITIAL") + "', NOW())");

	/* clear config tables for the initial config dump */
	PrepareDatabase();

	std::ostringstream q1buf;
	q1buf << "SELECT object_id, objecttype_id, name1, name2, is_active FROM " + GetTablePrefix() + "objects WHERE instance_id = " << static_cast<long>(m_InstanceID);
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

	m_QueryQueue.Enqueue(std::bind(&IdoPgsqlConnection::ClearTablesBySession, this), PriorityLow);

	m_QueryQueue.Enqueue(std::bind(&IdoPgsqlConnection::FinishConnect, this, startTime), PriorityLow);
}

void IdoPgsqlConnection::FinishConnect(double startTime)
{
	AssertOnWorkQueue();

	if (!GetConnected())
		return;

	Log(LogInformation, "IdoPgsqlConnection")
		<< "Finished reconnecting to PostgreSQL IDO database in " << std::setw(2) << Utility::GetTime() - startTime << " second(s).";

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
	Query("DELETE FROM " + GetTablePrefix() + table + " WHERE instance_id = " +
		Convert::ToString(static_cast<long>(m_InstanceID)) + " AND session_token <> " +
		Convert::ToString(GetSessionToken()));
}

IdoPgsqlResult IdoPgsqlConnection::Query(const String& query)
{
	AssertOnWorkQueue();

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

	return IdoPgsqlResult(result, std::bind(&PgsqlInterface::clear, std::cref(m_Pgsql), _1));
}

DbReference IdoPgsqlConnection::GetSequenceValue(const String& table, const String& column)
{
	AssertOnWorkQueue();

	IdoPgsqlResult result = Query("SELECT CURRVAL(pg_get_serial_sequence(E'" + Escape(table) + "', E'" + Escape(column) + "')) AS id");

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

	Dictionary::Ptr dict = new Dictionary();

	for (int column = 0; column < columns; column++) {
		Value value;

		if (!m_Pgsql->getisnull(result.get(), row, column))
			value = m_Pgsql->getvalue(result.get(), row, column);

		dict->Set(m_Pgsql->fname(result.get(), column), value);
	}

	return dict;
}

void IdoPgsqlConnection::ActivateObject(const DbObject::Ptr& dbobj)
{
	m_QueryQueue.Enqueue(std::bind(&IdoPgsqlConnection::InternalActivateObject, this, dbobj), PriorityLow);
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
				<< "E'" << Escape(dbobj->GetName1()) << "', E'" << Escape(dbobj->GetName2()) << "', 1)";
		} else {
			qbuf << "INSERT INTO " + GetTablePrefix() + "objects (instance_id, objecttype_id, name1, is_active) VALUES ("
				<< static_cast<long>(m_InstanceID) << ", " << dbobj->GetType()->GetTypeID() << ", "
				<< "E'" << Escape(dbobj->GetName1()) << "', 1)";
		}

		Query(qbuf.str());
		SetObjectID(dbobj, GetSequenceValue(GetTablePrefix() + "objects", "object_id"));
	} else {
		qbuf << "UPDATE " + GetTablePrefix() + "objects SET is_active = 1 WHERE object_id = " << static_cast<long>(dbref);
		Query(qbuf.str());
	}
}

void IdoPgsqlConnection::DeactivateObject(const DbObject::Ptr& dbobj)
{
	m_QueryQueue.Enqueue(std::bind(&IdoPgsqlConnection::InternalDeactivateObject, this, dbobj), PriorityLow);
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
	Query(qbuf.str());

	/* Note that we're _NOT_ clearing the db refs via SetReference/SetConfigUpdate/SetStatusUpdate
	 * because the object is still in the database. */
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

	if (rawvalue.IsObjectType<ConfigObject>()) {
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
		long ts = rawvalue;
		std::ostringstream msgbuf;
		msgbuf << "TO_TIMESTAMP(" << ts << ") AT TIME ZONE 'UTC'";
		*result = Value(msgbuf.str());
	} else if (DbValue::IsTimestampNow(value)) {
		*result = "NOW()";
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

		*result = "E'" + Escape(fvalue) + "'";
	}

	return true;
}

void IdoPgsqlConnection::ExecuteQuery(const DbQuery& query)
{
	ASSERT(query.Category != DbCatInvalid);

	m_QueryQueue.Enqueue(std::bind(&IdoPgsqlConnection::InternalExecuteQuery, this, query, -1), query.Priority, true);
}

void IdoPgsqlConnection::ExecuteMultipleQueries(const std::vector<DbQuery>& queries)
{
	if (queries.empty())
		return;

	m_QueryQueue.Enqueue(std::bind(&IdoPgsqlConnection::InternalExecuteMultipleQueries, this, queries), queries[0].Priority, true);
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

			if (kv.second.IsEmpty() && !kv.second.IsString())
				continue;

			if (!FieldToEscapedString(kv.first, kv.second, &value))
				return false;
		}
	}

	return true;
}

void IdoPgsqlConnection::InternalExecuteMultipleQueries(const std::vector<DbQuery>& queries)
{
	AssertOnWorkQueue();

	if (!GetConnected())
		return;

	for (const DbQuery& query : queries) {
		ASSERT(query.Type == DbQueryNewTransaction || query.Category != DbCatInvalid);

		if (!CanExecuteQuery(query)) {
			m_QueryQueue.Enqueue(std::bind(&IdoPgsqlConnection::InternalExecuteMultipleQueries, this, queries), query.Priority);
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

	if (!GetConnected())
		return;

	if (query.Type == DbQueryNewTransaction) {
		InternalNewTransaction();
		return;
	}

	/* check whether we're allowed to execute the query first */
	if (GetCategoryFilter() != DbCatEverything && (query.Category & GetCategoryFilter()) == 0)
		return;

	if (query.Object && query.Object->GetObject()->GetExtension("agent_check").ToBool())
		return;

	/* check if there are missing object/insert ids and re-enqueue the query */
	if (!CanExecuteQuery(query)) {
		m_QueryQueue.Enqueue(std::bind(&IdoPgsqlConnection::InternalExecuteQuery, this, query, typeOverride), query.Priority);
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
				m_QueryQueue.Enqueue(std::bind(&IdoPgsqlConnection::InternalExecuteQuery, this, query, -1), query.Priority);
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
			if (kv.second.IsEmpty() && !kv.second.IsString())
				continue;

			if (!FieldToEscapedString(kv.first, kv.second, &value)) {
				m_QueryQueue.Enqueue(std::bind(&IdoPgsqlConnection::InternalExecuteQuery, this, query, -1), query.Priority);
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
	m_QueryQueue.Enqueue(std::bind(&IdoPgsqlConnection::InternalCleanUpExecuteQuery, this, table, time_column, max_age), PriorityLow, true);
}

void IdoPgsqlConnection::InternalCleanUpExecuteQuery(const String& table, const String& time_column, double max_age)
{
	AssertOnWorkQueue();

	if (!GetConnected())
		return;

	Query("DELETE FROM " + GetTablePrefix() + table + " WHERE instance_id = " +
		Convert::ToString(static_cast<long>(m_InstanceID)) + " AND " + time_column +
		" < TO_TIMESTAMP(" + Convert::ToString(static_cast<long>(max_age)) + ")");
}

void IdoPgsqlConnection::FillIDCache(const DbType::Ptr& type)
{
	String query = "SELECT " + type->GetIDColumn() + " AS object_id, " + type->GetTable() + "_id, config_hash FROM " + GetTablePrefix() + type->GetTable() + "s";
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
