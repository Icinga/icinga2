/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
#include "icinga/perfdatavalue.hpp"
#include "base/logger.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/application.hpp"
#include "base/configtype.hpp"
#include "base/exception.hpp"
#include "base/context.hpp"
#include "base/statsfunction.hpp"
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(IdoPgsqlConnection);

REGISTER_STATSFUNCTION(IdoPgsqlConnection, &IdoPgsqlConnection::StatsFunc);

IdoPgsqlConnection::IdoPgsqlConnection(void)
	: m_QueryQueue(500000)
{ }

void IdoPgsqlConnection::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	Dictionary::Ptr nodes = new Dictionary();

	BOOST_FOREACH(const IdoPgsqlConnection::Ptr& idopgsqlconnection, ConfigType::GetObjectsByType<IdoPgsqlConnection>()) {
		size_t items = idopgsqlconnection->m_QueryQueue.GetLength();

		Dictionary::Ptr stats = new Dictionary();
		stats->Set("version", idopgsqlconnection->GetSchemaVersion());
		stats->Set("connected", idopgsqlconnection->GetConnected());
		stats->Set("instance_name", idopgsqlconnection->GetInstanceName());
		stats->Set("query_queue_items", items);

		nodes->Set(idopgsqlconnection->GetName(), stats);

		perfdata->Add(new PerfdataValue("idopgsqlconnection_" + idopgsqlconnection->GetName() + "_query_queue_items", items));
	}

	status->Set("idopgsqlconnection", nodes);
}

void IdoPgsqlConnection::Resume(void)
{
	DbConnection::Resume();

	SetConnected(false);

	m_QueryQueue.SetExceptionCallback(boost::bind(&IdoPgsqlConnection::ExceptionHandler, this, _1));

	m_TxTimer = new Timer();
	m_TxTimer->SetInterval(1);
	m_TxTimer->OnTimerExpired.connect(boost::bind(&IdoPgsqlConnection::TxTimerHandler, this));
	m_TxTimer->Start();

	m_ReconnectTimer = new Timer();
	m_ReconnectTimer->SetInterval(10);
	m_ReconnectTimer->OnTimerExpired.connect(boost::bind(&IdoPgsqlConnection::ReconnectTimerHandler, this));
	m_ReconnectTimer->Start();
	m_ReconnectTimer->Reschedule(0);

	ASSERT(PQisthreadsafe());
}

void IdoPgsqlConnection::Pause(void)
{
	m_ReconnectTimer.reset();

	DbConnection::Pause();

	m_QueryQueue.Enqueue(boost::bind(&IdoPgsqlConnection::Disconnect, this));
	m_QueryQueue.Join();
}

void IdoPgsqlConnection::ExceptionHandler(boost::exception_ptr exp)
{
	Log(LogWarning, "IdoPgsqlConnection", "Exception during database operation: Verify that your database is operational!");

	Log(LogDebug, "IdoPgsqlConnection")
	    << "Exception during database operation: " << DiagnosticInformation(exp);

	if (GetConnected()) {
		PQfinish(m_Connection);
		SetConnected(false);
	}
}

void IdoPgsqlConnection::AssertOnWorkQueue(void)
{
	ASSERT(m_QueryQueue.IsWorkerThread());
}

void IdoPgsqlConnection::Disconnect(void)
{
	AssertOnWorkQueue();

	if (!GetConnected())
		return;

	Query("COMMIT");

	PQfinish(m_Connection);
	SetConnected(false);
}

void IdoPgsqlConnection::TxTimerHandler(void)
{
	NewTransaction();
}

void IdoPgsqlConnection::NewTransaction(void)
{
	m_QueryQueue.Enqueue(boost::bind(&IdoPgsqlConnection::InternalNewTransaction, this), true);
}

void IdoPgsqlConnection::InternalNewTransaction(void)
{
	AssertOnWorkQueue();

	if (!GetConnected())
		return;

	Query("COMMIT");
	Query("BEGIN");
}

void IdoPgsqlConnection::ReconnectTimerHandler(void)
{
	m_QueryQueue.Enqueue(boost::bind(&IdoPgsqlConnection::Reconnect, this));
}

void IdoPgsqlConnection::Reconnect(void)
{
	AssertOnWorkQueue();

	CONTEXT("Reconnecting to PostgreSQL IDO database '" + GetName() + "'");

	m_SessionToken = Utility::NewUniqueID();

	SetShouldConnect(true);

	std::vector<DbObject::Ptr> active_dbobjs;

	{
		bool reconnect = false;

		if (GetConnected()) {
			/* Check if we're really still connected */
			try {
				Query("SELECT 1");
				return;
			} catch (const std::exception&) {
				PQfinish(m_Connection);
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

		host = (!ihost.IsEmpty()) ? ihost.CStr() : NULL;
		port = (!iport.IsEmpty()) ? iport.CStr() : NULL;
		user = (!iuser.IsEmpty()) ? iuser.CStr() : NULL;
		passwd = (!ipasswd.IsEmpty()) ? ipasswd.CStr() : NULL;
		db = (!idb.IsEmpty()) ? idb.CStr() : NULL;

		m_Connection = PQsetdbLogin(host, port, NULL, NULL, db, user, passwd);

		if (!m_Connection)
			return;

		if (PQstatus(m_Connection) != CONNECTION_OK) {
			String message = PQerrorMessage(m_Connection);
			PQfinish(m_Connection);
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
		if (PQserverVersion(m_Connection) >= 90100)
			result = Query("SET standard_conforming_strings TO off");

		String dbVersionName = "idoutils";
		result = Query("SELECT version FROM " + GetTablePrefix() + "dbversion WHERE name=E'" + Escape(dbVersionName) + "'");

		Dictionary::Ptr row = FetchRow(result, 0);

		if (!row) {
			PQfinish(m_Connection);
			SetConnected(false);

			Log(LogCritical, "IdoPgsqlConnection", "Schema does not provide any valid version! Verify your schema installation.");

			Application::Exit(EXIT_FAILURE);
		}

		String version = row->Get("version");

		SetSchemaVersion(version);

		if (Utility::CompareVersion(IDO_COMPAT_SCHEMA_VERSION, version) < 0) {
			PQfinish(m_Connection);
			SetConnected(false);

			Log(LogCritical, "IdoPgsqlConnection")
			    << "Schema version '" << version << "' does not match the required version '"
			    << IDO_COMPAT_SCHEMA_VERSION << "' (or newer)! Please check the upgrade documentation.";

			Application::Exit(EXIT_FAILURE);
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
					PQfinish(m_Connection);
					SetConnected(false);
					SetShouldConnect(false);

					return;
				}

				/* activate the IDO only, if we're authoritative in this zone */
				if (IsPaused()) {
					Log(LogNotice, "IdoPgsqlConnection")
					    << "Local endpoint '" << my_endpoint->GetName() << "' is not authoritative, bailing out.";

					PQfinish(m_Connection);
					SetConnected(false);

					return;
				}
			}

			Log(LogNotice, "IdoPgsqlConnection", "Enabling IDO connection.");
		}

		Log(LogInformation, "IdoPgsqlConnection")
		    << "pgSQL IDO instance id: " << static_cast<long>(m_InstanceID) << " (schema version: '" + version + "')";

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

		int index = 0;
		while ((row = FetchRow(result, index))) {
			index++;

			DbType::Ptr dbtype = DbType::GetByID(row->Get("objecttype_id"));

			if (!dbtype)
				continue;

			DbObject::Ptr dbobj = dbtype->GetOrCreateObjectByName(row->Get("name1"), row->Get("name2"));
			SetObjectID(dbobj, DbReference(row->Get("object_id")));
			SetObjectActive(dbobj, row->Get("is_active"));

			if (GetObjectActive(dbobj))
				active_dbobjs.push_back(dbobj);
		}

		Query("BEGIN");
	}

	UpdateAllObjects();

	/* deactivate all deleted configuration objects */
	BOOST_FOREACH(const DbObject::Ptr& dbobj, active_dbobjs) {
		if (dbobj->GetObject() == NULL) {
			Log(LogNotice, "IdoPgsqlConnection")
			    << "Deactivate deleted object name1: '" << dbobj->GetName1()
			    << "' name2: '" << dbobj->GetName2() + "'.";
			DeactivateObject(dbobj);
		}
	}

	/* delete all customvariables without current session token */
	ClearCustomVarTable("customvariables");
	ClearCustomVarTable("customvariablestatus");

	Query("COMMIT");
	Query("BEGIN");
}

void IdoPgsqlConnection::ClearCustomVarTable(const String& table)
{
	Query("DELETE FROM " + GetTablePrefix() + table + " WHERE session_token <> '" + Escape(m_SessionToken) + "'");
}

void IdoPgsqlConnection::ClearConfigTable(const String& table)
{
	Query("DELETE FROM " + GetTablePrefix() + table + " WHERE instance_id = " + Convert::ToString(static_cast<long>(m_InstanceID)));
}

IdoPgsqlResult IdoPgsqlConnection::Query(const String& query)
{
	AssertOnWorkQueue();

	Log(LogDebug, "IdoPgsqlConnection")
	    << "Query: " << query;

	IncreaseQueryCount();

	PGresult *result = PQexec(m_Connection, query.CStr());

	if (!result) {
		String message = PQerrorMessage(m_Connection);
		Log(LogCritical, "IdoPgsqlConnection")
		    << "Error \"" << message << "\" when executing query \"" << query << "\"";

		BOOST_THROW_EXCEPTION(
		    database_error()
			<< errinfo_message(message)
			<< errinfo_database_query(query)
		);
	}

	char *rowCount = PQcmdTuples(result);
	m_AffectedRows = atoi(rowCount);

	if (PQresultStatus(result) == PGRES_COMMAND_OK) {
		PQclear(result);
		return IdoPgsqlResult();
	}

	if (PQresultStatus(result) != PGRES_TUPLES_OK) {
		String message = PQresultErrorMessage(result);
		PQclear(result);

		Log(LogCritical, "IdoPgsqlConnection")
		    << "Error \"" << message << "\" when executing query \"" << query << "\"";

		BOOST_THROW_EXCEPTION(
		    database_error()
			<< errinfo_message(message)
			<< errinfo_database_query(query)
		);
	}

	return IdoPgsqlResult(result, std::ptr_fun(PQclear));
}

DbReference IdoPgsqlConnection::GetSequenceValue(const String& table, const String& column)
{
	AssertOnWorkQueue();

	IdoPgsqlResult result = Query("SELECT CURRVAL(pg_get_serial_sequence(E'" + Escape(table) + "', E'" + Escape(column) + "')) AS id");

	Dictionary::Ptr row = FetchRow(result, 0);

	ASSERT(row);

	Log(LogDebug, "IdoPgsqlConnection")
	    << "Sequence Value: " << row->Get("id");

	return DbReference(Convert::ToLong(row->Get("id")));
}

int IdoPgsqlConnection::GetAffectedRows(void)
{
	AssertOnWorkQueue();

	return m_AffectedRows;
}

String IdoPgsqlConnection::Escape(const String& s)
{
	AssertOnWorkQueue();

	size_t length = s.GetLength();
	char *to = new char[s.GetLength() * 2 + 1];

	PQescapeStringConn(m_Connection, to, s.CStr(), length, NULL);

	String result = String(to);

	delete [] to;

	return result;
}

Dictionary::Ptr IdoPgsqlConnection::FetchRow(const IdoPgsqlResult& result, int row)
{
	AssertOnWorkQueue();

	if (row >= PQntuples(result.get()))
		return Dictionary::Ptr();

	int columns = PQnfields(result.get());

	Dictionary::Ptr dict = new Dictionary();

	for (int column = 0; column < columns; column++) {
		Value value;

		if (!PQgetisnull(result.get(), row, column))
			value = PQgetvalue(result.get(), row, column);

		dict->Set(PQfname(result.get(), column), value);
	}

	return dict;
}

void IdoPgsqlConnection::ActivateObject(const DbObject::Ptr& dbobj)
{
	m_QueryQueue.Enqueue(boost::bind(&IdoPgsqlConnection::InternalActivateObject, this, dbobj));
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
	m_QueryQueue.Enqueue(boost::bind(&IdoPgsqlConnection::InternalDeactivateObject, this, dbobj));
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
	}
	if (key == "notification_id") {
		*result = static_cast<long>(GetNotificationInsertID(value));
		return true;
	}

	Value rawvalue = DbValue::ExtractValue(value);

	if (rawvalue.IsObjectType<ConfigObject>()) {
		DbObject::Ptr dbobjcol = DbObject::GetOrCreateByObject(rawvalue);

		if (!dbobjcol) {
			*result = 0;
			return true;
		}

		DbReference dbrefcol;

		if (DbValue::IsObjectInsertID(value)) {
			dbrefcol = GetInsertID(dbobjcol);

			ASSERT(dbrefcol.IsValid());
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
		msgbuf << "TO_TIMESTAMP(" << ts << ")";
		*result = Value(msgbuf.str());
	} else if (DbValue::IsTimestampNow(value)) {
		*result = "NOW()";
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

	m_QueryQueue.Enqueue(boost::bind(&IdoPgsqlConnection::InternalExecuteQuery, this, query, (DbQueryType *)NULL), true);
}

void IdoPgsqlConnection::InternalExecuteQuery(const DbQuery& query, DbQueryType *typeOverride)
{
	AssertOnWorkQueue();

	if ((query.Category & GetCategories()) == 0)
		return;

	if (!GetConnected())
		return;

	if (query.Object && query.Object->GetObject()->GetExtension("agent_check").ToBool())
		return;

	std::ostringstream qbuf, where;
	int type;

	if (query.WhereCriteria) {
		where << " WHERE ";

		ObjectLock olock(query.WhereCriteria);
		Value value;
		bool first = true;

		BOOST_FOREACH(const Dictionary::Pair& kv, query.WhereCriteria) {
			if (!FieldToEscapedString(kv.first, kv.second, &value))
				return;

			if (!first)
				where << " AND ";

			where << kv.first << " = " << value;

			if (first)
				first = false;
		}
	}

	type = typeOverride ? *typeOverride : query.Type;

	bool upsert = false;

	if ((type & DbQueryInsert) && (type & DbQueryUpdate)) {
		bool hasid = false;

		ASSERT(query.Object);

		if (query.ConfigUpdate)
			hasid = GetConfigUpdate(query.Object);
		else if (query.StatusUpdate)
			hasid = GetStatusUpdate(query.Object);

		if (!hasid)
			upsert = true;

		type = DbQueryUpdate;
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
		BOOST_FOREACH(const Dictionary::Pair& kv, query.Fields) {
			if (kv.second.IsEmpty() && !kv.second.IsString())
				continue;

			if (!FieldToEscapedString(kv.first, kv.second, &value))
				return;

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
		DbQueryType to = DbQueryInsert;
		InternalExecuteQuery(query, &to);

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

	if (type == DbQueryInsert && query.Table == "notifications" && query.NotificationObject) { // FIXME remove hardcoded table name
		String idField = "notification_id";
		DbReference seqval = GetSequenceValue(GetTablePrefix() + query.Table, idField);
		SetNotificationInsertID(query.NotificationObject, seqval);
		Log(LogDebug, "IdoPgsqlConnection")
		    << "saving contactnotification notification_id=" << Convert::ToString(seqval);
	}
}

void IdoPgsqlConnection::CleanUpExecuteQuery(const String& table, const String& time_column, double max_age)
{
	m_QueryQueue.Enqueue(boost::bind(&IdoPgsqlConnection::InternalCleanUpExecuteQuery, this, table, time_column, max_age), true);
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
	String query = "SELECT " + type->GetIDColumn() + " AS object_id, " + type->GetTable() + "_id FROM " + GetTablePrefix() + type->GetTable() + "s";
	IdoPgsqlResult result = Query(query);

	Dictionary::Ptr row;

	int index = 0;
	while ((row = FetchRow(result, index))) {
		index++;
		SetInsertID(type, DbReference(row->Get("object_id")), DbReference(row->Get(type->GetTable() + "_id")));
	}
}

int IdoPgsqlConnection::GetPendingQueryCount(void) const
{
	return m_QueryQueue.GetLength();
}
