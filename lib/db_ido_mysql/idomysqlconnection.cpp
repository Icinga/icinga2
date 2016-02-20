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

#include "db_ido_mysql/idomysqlconnection.hpp"
#include "db_ido_mysql/idomysqlconnection.tcpp"
#include "icinga/perfdatavalue.hpp"
#include "db_ido/dbtype.hpp"
#include "db_ido/dbvalue.hpp"
#include "base/logger.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/application.hpp"
#include "base/configtype.hpp"
#include "base/exception.hpp"
#include "base/statsfunction.hpp"
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(IdoMysqlConnection);
REGISTER_STATSFUNCTION(IdoMysqlConnection, &IdoMysqlConnection::StatsFunc);

IdoMysqlConnection::IdoMysqlConnection(void)
	: m_QueryQueue(1000000)
{ }

void IdoMysqlConnection::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	Dictionary::Ptr nodes = new Dictionary();

	BOOST_FOREACH(const IdoMysqlConnection::Ptr& idomysqlconnection, ConfigType::GetObjectsByType<IdoMysqlConnection>()) {
		size_t items = idomysqlconnection->m_QueryQueue.GetLength();

		Dictionary::Ptr stats = new Dictionary();
		stats->Set("version", idomysqlconnection->GetSchemaVersion());
		stats->Set("instance_name", idomysqlconnection->GetInstanceName());
		stats->Set("connected", idomysqlconnection->GetConnected());
		stats->Set("query_queue_items", items);

		nodes->Set(idomysqlconnection->GetName(), stats);

		perfdata->Add(new PerfdataValue("idomysqlconnection_" + idomysqlconnection->GetName() + "_queries_rate", idomysqlconnection->GetQueryCount(60) / 60.0));
		perfdata->Add(new PerfdataValue("idomysqlconnection_" + idomysqlconnection->GetName() + "_queries_1min", idomysqlconnection->GetQueryCount(60)));
		perfdata->Add(new PerfdataValue("idomysqlconnection_" + idomysqlconnection->GetName() + "_queries_5mins", idomysqlconnection->GetQueryCount(5 * 60)));
		perfdata->Add(new PerfdataValue("idomysqlconnection_" + idomysqlconnection->GetName() + "_queries_15mins", idomysqlconnection->GetQueryCount(15 * 60)));
		perfdata->Add(new PerfdataValue("idomysqlconnection_" + idomysqlconnection->GetName() + "_query_queue_items", items));
	}

	status->Set("idomysqlconnection", nodes);
}

void IdoMysqlConnection::Resume(void)
{
	DbConnection::Resume();

	SetConnected(false);

	m_QueryQueue.SetExceptionCallback(boost::bind(&IdoMysqlConnection::ExceptionHandler, this, _1));

	m_TxTimer = new Timer();
	m_TxTimer->SetInterval(1);
	m_TxTimer->OnTimerExpired.connect(boost::bind(&IdoMysqlConnection::TxTimerHandler, this));
	m_TxTimer->Start();

	m_ReconnectTimer = new Timer();
	m_ReconnectTimer->SetInterval(10);
	m_ReconnectTimer->OnTimerExpired.connect(boost::bind(&IdoMysqlConnection::ReconnectTimerHandler, this));
	m_ReconnectTimer->Start();
	m_ReconnectTimer->Reschedule(0);

	ASSERT(mysql_thread_safe());
}

void IdoMysqlConnection::Pause(void)
{
	m_ReconnectTimer.reset();

	DbConnection::Pause();

	m_QueryQueue.Enqueue(boost::bind(&IdoMysqlConnection::Disconnect, this), PriorityHigh);
	m_QueryQueue.Join();
}

void IdoMysqlConnection::ExceptionHandler(boost::exception_ptr exp)
{
	Log(LogCritical, "IdoMysqlConnection", "Exception during database operation: Verify that your database is operational!");

	Log(LogDebug, "IdoMysqlConnection")
	    << "Exception during database operation: " << DiagnosticInformation(exp);

	if (GetConnected()) {
		mysql_close(&m_Connection);

		SetConnected(false);
	}
}

void IdoMysqlConnection::AssertOnWorkQueue(void)
{
	ASSERT(m_QueryQueue.IsWorkerThread());
}

void IdoMysqlConnection::Disconnect(void)
{
	AssertOnWorkQueue();

	if (!GetConnected())
		return;

	Query("COMMIT");
	mysql_close(&m_Connection);

	SetConnected(false);
}

void IdoMysqlConnection::TxTimerHandler(void)
{
	NewTransaction();
}

void IdoMysqlConnection::NewTransaction(void)
{
	m_QueryQueue.Enqueue(boost::bind(&IdoMysqlConnection::InternalNewTransaction, this), PriorityHigh);
	m_QueryQueue.Enqueue(boost::bind(&IdoMysqlConnection::FinishAsyncQueries, this), PriorityHigh);
}

void IdoMysqlConnection::InternalNewTransaction(void)
{
	AssertOnWorkQueue();

	if (!GetConnected())
		return;

	AsyncQuery("COMMIT");
	AsyncQuery("BEGIN");
}

void IdoMysqlConnection::ReconnectTimerHandler(void)
{
	m_QueryQueue.Enqueue(boost::bind(&IdoMysqlConnection::Reconnect, this), PriorityLow);
}

void IdoMysqlConnection::Reconnect(void)
{
	AssertOnWorkQueue();

	if (!IsActive())
		return;

	CONTEXT("Reconnecting to MySQL IDO database '" + GetName() + "'");

	double startTime = Utility::GetTime();
	m_SessionToken = static_cast<int>(Utility::GetTime());

	SetShouldConnect(true);

	bool reconnect = false;

	if (GetConnected()) {
		/* Check if we're really still connected */
		if (mysql_ping(&m_Connection) == 0)
			return;

		mysql_close(&m_Connection);
		SetConnected(false);
		reconnect = true;
	}

	ClearIDCache();

	String ihost, isocket_path, iuser, ipasswd, idb;
	const char *host, *socket_path, *user , *passwd, *db;
	long port;

	ihost = GetHost();
	isocket_path = GetSocketPath();
	iuser = GetUser();
	ipasswd = GetPassword();
	idb = GetDatabase();

	host = (!ihost.IsEmpty()) ? ihost.CStr() : NULL;
	port = GetPort();
	socket_path = (!isocket_path.IsEmpty()) ? isocket_path.CStr() : NULL;
	user = (!iuser.IsEmpty()) ? iuser.CStr() : NULL;
	passwd = (!ipasswd.IsEmpty()) ? ipasswd.CStr() : NULL;
	db = (!idb.IsEmpty()) ? idb.CStr() : NULL;

	/* connection */
	if (!mysql_init(&m_Connection)) {
		Log(LogCritical, "IdoMysqlConnection")
		    << "mysql_init() failed: \"" << mysql_error(&m_Connection) << "\"";

		BOOST_THROW_EXCEPTION(std::bad_alloc());
	}

	if (!mysql_real_connect(&m_Connection, host, user, passwd, db, port, socket_path, CLIENT_FOUND_ROWS | CLIENT_MULTI_STATEMENTS)) {
		Log(LogCritical, "IdoMysqlConnection")
		    << "Connection to database '" << db << "' with user '" << user << "' on '" << host << ":" << port
		    << "' failed: \"" << mysql_error(&m_Connection) << "\"";

		BOOST_THROW_EXCEPTION(std::runtime_error(mysql_error(&m_Connection)));
	}

	SetConnected(true);

	IdoMysqlResult result = Query("SELECT @@global.max_allowed_packet AS max_allowed_packet");

	Dictionary::Ptr row = FetchRow(result);

	if (row)
		m_MaxPacketSize = row->Get("max_allowed_packet");
	else
		m_MaxPacketSize = 64 * 1024;

	DiscardRows(result);

	String dbVersionName = "idoutils";
	result = Query("SELECT version FROM " + GetTablePrefix() + "dbversion WHERE name='" + Escape(dbVersionName) + "'");

	row = FetchRow(result);

	if (!row) {
		mysql_close(&m_Connection);
		SetConnected(false);

		Log(LogCritical, "IdoMysqlConnection", "Schema does not provide any valid version! Verify your schema installation.");

		Application::Exit(EXIT_FAILURE);
	}

	DiscardRows(result);

	String version = row->Get("version");

	SetSchemaVersion(version);

	if (Utility::CompareVersion(IDO_COMPAT_SCHEMA_VERSION, version) < 0) {
		mysql_close(&m_Connection);
		SetConnected(false);

		Log(LogCritical, "IdoMysqlConnection")
		    << "Schema version '" << version << "' does not match the required version '"
		    << IDO_COMPAT_SCHEMA_VERSION << "' (or newer)! Please check the upgrade documentation.";

		Application::Exit(EXIT_FAILURE);
	}

	String instanceName = GetInstanceName();

	result = Query("SELECT instance_id FROM " + GetTablePrefix() + "instances WHERE instance_name = '" + Escape(instanceName) + "'");
	row = FetchRow(result);

	if (!row) {
		Query("INSERT INTO " + GetTablePrefix() + "instances (instance_name, instance_description) VALUES ('" + Escape(instanceName) + "', '" + Escape(GetInstanceDescription()) + "')");
		m_InstanceID = GetLastInsertID();
	} else {
		m_InstanceID = DbReference(row->Get("instance_id"));
	}

	DiscardRows(result);

	Endpoint::Ptr my_endpoint = Endpoint::GetLocalEndpoint();

	/* we have an endpoint in a cluster setup, so decide if we can proceed here */
	if (my_endpoint && GetHAMode() == HARunOnce) {
		/* get the current endpoint writing to programstatus table */
		result = Query("SELECT UNIX_TIMESTAMP(status_update_time) AS status_update_time, endpoint_name FROM " +
		    GetTablePrefix() + "programstatus WHERE instance_id = " + Convert::ToString(m_InstanceID));
		row = FetchRow(result);
		DiscardRows(result);

		String endpoint_name;

		if (row)
			endpoint_name = row->Get("endpoint_name");
		else
			Log(LogNotice, "IdoMysqlConnection", "Empty program status table");

		/* if we did not write into the database earlier, another instance is active */
		if (endpoint_name != my_endpoint->GetName()) {
			double status_update_time;

			if (row)
				status_update_time = row->Get("status_update_time");
			else
				status_update_time = 0;

			double status_update_age = Utility::GetTime() - status_update_time;

			Log(LogNotice, "IdoMysqlConnection")
			    << "Last update by '" << endpoint_name << "' was " << status_update_age << "s ago.";

			if (status_update_age < GetFailoverTimeout()) {
				mysql_close(&m_Connection);
				SetConnected(false);
				SetShouldConnect(false);

				return;
			}

			/* activate the IDO only, if we're authoritative in this zone */
			if (IsPaused()) {
				Log(LogNotice, "IdoMysqlConnection")
				    << "Local endpoint '" << my_endpoint->GetName() << "' is not authoritative, bailing out.";

				mysql_close(&m_Connection);
				SetConnected(false);

				return;
			}
		}

		Log(LogNotice, "IdoMysqlConnection", "Enabling IDO connection.");
	}

	Log(LogInformation, "IdoMysqlConnection")
	    << "MySQL IDO instance id: " << static_cast<long>(m_InstanceID) << " (schema version: '" + version + "')";

	/* set session time zone to utc */
	Query("SET SESSION TIME_ZONE='+00:00'");

	/* record connection */
	Query("INSERT INTO " + GetTablePrefix() + "conninfo " +
	    "(instance_id, connect_time, last_checkin_time, agent_name, agent_version, connect_type, data_start_time) VALUES ("
	    + Convert::ToString(static_cast<long>(m_InstanceID)) + ", NOW(), NOW(), 'icinga2 db_ido_mysql', '" + Escape(Application::GetAppVersion())
	    + "', '" + (reconnect ? "RECONNECT" : "INITIAL") + "', NOW())");

	/* clear config tables for the initial config dump */
	PrepareDatabase();

	std::ostringstream q1buf;
	q1buf << "SELECT object_id, objecttype_id, name1, name2, is_active FROM " + GetTablePrefix() + "objects WHERE instance_id = " << static_cast<long>(m_InstanceID);
	result = Query(q1buf.str());

	std::vector<DbObject::Ptr> activeDbObjs;

	while ((row = FetchRow(result))) {
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

	Query("BEGIN");

	BOOST_FOREACH(const DbObject::Ptr& dbobj, activeDbObjs) {
		if (dbobj->GetObject() == NULL) {
			Log(LogNotice, "IdoMysqlConnection")
			    << "Deactivate deleted object name1: '" << dbobj->GetName1()
			    << "' name2: '" << dbobj->GetName2() + "'.";
			DeactivateObject(dbobj);
		} else {
			dbobj->SendConfigUpdate();
			dbobj->SendStatusUpdate();
		}
	}

	UpdateAllObjects();

	/* delete all customvariables without current session token */
	ClearCustomVarTable("customvariables");
	ClearCustomVarTable("customvariablestatus");

	m_QueryQueue.Enqueue(boost::bind(&IdoMysqlConnection::FinishConnect, this, startTime), PriorityLow);
}

void IdoMysqlConnection::FinishConnect(double startTime)
{
	AssertOnWorkQueue();

	if (!GetConnected())
		return;

	FinishAsyncQueries();

	Log(LogInformation, "IdoMysqlConnection")
	    << "Finished reconnecting to MySQL IDO database in " << std::setw(2) << Utility::GetTime() - startTime << " second(s).";

	Query("COMMIT");
	Query("BEGIN");
}

void IdoMysqlConnection::ClearCustomVarTable(const String& table)
{
	Query("DELETE FROM " + GetTablePrefix() + table + " WHERE session_token <> " + Convert::ToString(m_SessionToken));
}

void IdoMysqlConnection::ClearConfigTable(const String& table)
{
	Query("DELETE FROM " + GetTablePrefix() + table + " WHERE instance_id = " + Convert::ToString(static_cast<long>(m_InstanceID)));
}

void IdoMysqlConnection::AsyncQuery(const String& query, const boost::function<void (const IdoMysqlResult&)>& callback)
{
	AssertOnWorkQueue();

	IdoAsyncQuery aq;
	aq.Query = query;
	aq.Callback = callback;
	m_AsyncQueries.push_back(aq);
}

void IdoMysqlConnection::FinishAsyncQueries(void)
{
	std::vector<IdoAsyncQuery> queries;
	m_AsyncQueries.swap(queries);

	std::vector<IdoAsyncQuery>::size_type offset = 0;

	while (offset < queries.size()) {
		std::ostringstream querybuf;

		std::vector<IdoAsyncQuery>::size_type count = 0;
		size_t num_bytes = 0;

		for (std::vector<IdoAsyncQuery>::size_type i = offset; i < queries.size(); i++) {
			const IdoAsyncQuery& aq = queries[i];

			size_t size_query = aq.Query.GetLength() + 1;

			if (num_bytes + size_query > m_MaxPacketSize - 512)
				break;

			if (count > 0)
				querybuf << ";";

			IncreaseQueryCount();
			count++;

			Log(LogDebug, "IdoMysqlConnection")
			    << "Query: " << aq.Query;

			querybuf << aq.Query;
			num_bytes += size_query;
		}

		String query = querybuf.str();

		if (mysql_query(&m_Connection, query.CStr()) != 0) {
			std::ostringstream msgbuf;
			String message = mysql_error(&m_Connection);
			msgbuf << "Error \"" << message << "\" when executing query \"" << query << "\"";
			Log(LogCritical, "IdoMysqlConnection", msgbuf.str());

			BOOST_THROW_EXCEPTION(
			    database_error()
				<< errinfo_message(mysql_error(&m_Connection))
				<< errinfo_database_query(query)
			);
		}

		for (std::vector<IdoAsyncQuery>::size_type i = offset; i < offset + count; i++) {
			const IdoAsyncQuery& aq = queries[i];

			MYSQL_RES *result = mysql_store_result(&m_Connection);

			m_AffectedRows = mysql_affected_rows(&m_Connection);

			IdoMysqlResult iresult;

			if (!result) {
				if (mysql_field_count(&m_Connection) > 0) {
					std::ostringstream msgbuf;
					String message = mysql_error(&m_Connection);
					msgbuf << "Error \"" << message << "\" when executing query \"" << aq.Query << "\"";
					Log(LogCritical, "IdoMysqlConnection", msgbuf.str());

					BOOST_THROW_EXCEPTION(
					    database_error()
						<< errinfo_message(mysql_error(&m_Connection))
						<< errinfo_database_query(query)
					);
				}
			} else
				iresult = IdoMysqlResult(result, std::ptr_fun(mysql_free_result));

			if (aq.Callback)
				aq.Callback(iresult);

			if (mysql_next_result(&m_Connection) > 0) {
				std::ostringstream msgbuf;
				String message = mysql_error(&m_Connection);
				msgbuf << "Error \"" << message << "\" when executing query \"" << query << "\"";
				Log(LogCritical, "IdoMysqlConnection", msgbuf.str());

				BOOST_THROW_EXCEPTION(
				    database_error()
					<< errinfo_message(mysql_error(&m_Connection))
					<< errinfo_database_query(query)
				);
			}
		}

		offset += count;
	}
}

IdoMysqlResult IdoMysqlConnection::Query(const String& query)
{
	AssertOnWorkQueue();

	/* finish all async queries to maintain the right order for queries */
	FinishAsyncQueries();

	Log(LogDebug, "IdoMysqlConnection")
	    << "Query: " << query;

	IncreaseQueryCount();

	if (mysql_query(&m_Connection, query.CStr()) != 0) {
		std::ostringstream msgbuf;
		String message = mysql_error(&m_Connection);
		msgbuf << "Error \"" << message << "\" when executing query \"" << query << "\"";
		Log(LogCritical, "IdoMysqlConnection", msgbuf.str());

		BOOST_THROW_EXCEPTION(
		    database_error()
			<< errinfo_message(mysql_error(&m_Connection))
			<< errinfo_database_query(query)
		);
	}

	MYSQL_RES *result = mysql_store_result(&m_Connection);

	m_AffectedRows = mysql_affected_rows(&m_Connection);

	if (!result) {
		if (mysql_field_count(&m_Connection) > 0) {
			std::ostringstream msgbuf;
			String message = mysql_error(&m_Connection);
			msgbuf << "Error \"" << message << "\" when executing query \"" << query << "\"";
			Log(LogCritical, "IdoMysqlConnection", msgbuf.str());

			BOOST_THROW_EXCEPTION(
			    database_error()
				<< errinfo_message(mysql_error(&m_Connection))
				<< errinfo_database_query(query)
			);
		}

		return IdoMysqlResult();
	}

	return IdoMysqlResult(result, std::ptr_fun(mysql_free_result));
}

DbReference IdoMysqlConnection::GetLastInsertID(void)
{
	AssertOnWorkQueue();

	return DbReference(mysql_insert_id(&m_Connection));
}

int IdoMysqlConnection::GetAffectedRows(void)
{
	AssertOnWorkQueue();

	return m_AffectedRows;
}

String IdoMysqlConnection::Escape(const String& s)
{
	AssertOnWorkQueue();

	String utf8s = Utility::ValidateUTF8(s);

	size_t length = utf8s.GetLength();
	char *to = new char[utf8s.GetLength() * 2 + 1];

	mysql_real_escape_string(&m_Connection, to, utf8s.CStr(), length);

	String result = String(to);

	delete [] to;

	return result;
}

Dictionary::Ptr IdoMysqlConnection::FetchRow(const IdoMysqlResult& result)
{
	AssertOnWorkQueue();

	MYSQL_ROW row;
	MYSQL_FIELD *field;
	unsigned long *lengths, i;

	row = mysql_fetch_row(result.get());

	if (!row)
		return Dictionary::Ptr();

	lengths = mysql_fetch_lengths(result.get());

	if (!lengths)
		return Dictionary::Ptr();

	Dictionary::Ptr dict = new Dictionary();

	mysql_field_seek(result.get(), 0);
	for (field = mysql_fetch_field(result.get()), i = 0; field; field = mysql_fetch_field(result.get()), i++)
		dict->Set(field->name, String(row[i], row[i] + lengths[i]));

	return dict;
}

void IdoMysqlConnection::DiscardRows(const IdoMysqlResult& result)
{
	Dictionary::Ptr row;

	while ((row = FetchRow(result)))
		; /* empty loop body */
}

void IdoMysqlConnection::ActivateObject(const DbObject::Ptr& dbobj)
{
	m_QueryQueue.Enqueue(boost::bind(&IdoMysqlConnection::InternalActivateObject, this, dbobj), PriorityLow);
}

void IdoMysqlConnection::InternalActivateObject(const DbObject::Ptr& dbobj)
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

		Query(qbuf.str());
		SetObjectID(dbobj, GetLastInsertID());
	} else {
		qbuf << "UPDATE " + GetTablePrefix() + "objects SET is_active = 1 WHERE object_id = " << static_cast<long>(dbref);
		AsyncQuery(qbuf.str());
	}
}

void IdoMysqlConnection::DeactivateObject(const DbObject::Ptr& dbobj)
{
	m_QueryQueue.Enqueue(boost::bind(&IdoMysqlConnection::InternalDeactivateObject, this, dbobj), PriorityLow);
}

void IdoMysqlConnection::InternalDeactivateObject(const DbObject::Ptr& dbobj)
{
	AssertOnWorkQueue();

	if (!GetConnected())
		return;

	DbReference dbref = GetObjectID(dbobj);

	if (!dbref.IsValid())
		return;

	std::ostringstream qbuf;
	qbuf << "UPDATE " + GetTablePrefix() + "objects SET is_active = 0 WHERE object_id = " << static_cast<long>(dbref);
	AsyncQuery(qbuf.str());

	/* Note that we're _NOT_ clearing the db refs via SetReference/SetConfigUpdate/SetStatusUpdate
	 * because the object is still in the database. */
}

bool IdoMysqlConnection::FieldToEscapedString(const String& key, const Value& value, Value *result)
{
	if (key == "instance_id") {
		*result = static_cast<long>(m_InstanceID);
		return true;
	} else if (key == "session_token") {
		*result = m_SessionToken;
		return true;
	} else if (key == "notification_id") {
		DbReference ref = GetNotificationInsertID(value);

		if (!ref.IsValid())
			return false;

		*result = static_cast<long>(ref);
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
		msgbuf << "FROM_UNIXTIME(" << ts << ")";
		*result = Value(msgbuf.str());
	} else if (DbValue::IsTimestampNow(value)) {
		*result = "NOW()";
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

void IdoMysqlConnection::ExecuteQuery(const DbQuery& query)
{
	ASSERT(query.Category != DbCatInvalid);

	m_QueryQueue.Enqueue(boost::bind(&IdoMysqlConnection::InternalExecuteQuery, this, query, (DbQueryType *)NULL), query.Priority, true);
}

void IdoMysqlConnection::ExecuteMultipleQueries(const std::vector<DbQuery>& queries)
{
	ASSERT(!queries.empty());

	m_QueryQueue.Enqueue(boost::bind(&IdoMysqlConnection::InternalExecuteMultipleQueries, this, queries), queries[0].Priority, true);
}

bool IdoMysqlConnection::CanExecuteQuery(const DbQuery& query)
{
	if (query.WhereCriteria) {
		ObjectLock olock(query.WhereCriteria);
		Value value;

		BOOST_FOREACH(const Dictionary::Pair& kv, query.WhereCriteria) {
			if (!FieldToEscapedString(kv.first, kv.second, &value))
				return false;
		}
	}

	if (query.Fields) {
		ObjectLock olock(query.Fields);

		BOOST_FOREACH(const Dictionary::Pair& kv, query.Fields) {
			Value value;

			if (kv.second.IsEmpty() && !kv.second.IsString())
				continue;

			if (!FieldToEscapedString(kv.first, kv.second, &value))
				return false;
		}
	}

	return true;
}

void IdoMysqlConnection::InternalExecuteMultipleQueries(const std::vector<DbQuery>& queries)
{
	AssertOnWorkQueue();

	if (!GetConnected())
		return;

	BOOST_FOREACH(const DbQuery& query, queries) {
		ASSERT(query.Category != DbCatInvalid);

		if (!CanExecuteQuery(query)) {
			m_QueryQueue.Enqueue(boost::bind(&IdoMysqlConnection::InternalExecuteMultipleQueries, this, queries), query.Priority);
			return;
		}
	}

	BOOST_FOREACH(const DbQuery& query, queries) {
		InternalExecuteQuery(query, NULL);
	}
}

void IdoMysqlConnection::InternalExecuteQuery(const DbQuery& query, DbQueryType *typeOverride)
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
			if (!FieldToEscapedString(kv.first, kv.second, &value)) {
				m_QueryQueue.Enqueue(boost::bind(&IdoMysqlConnection::InternalExecuteQuery, this, query, (DbQueryType *)NULL), query.Priority);
				return;
			}

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

		bool first = true;
		BOOST_FOREACH(const Dictionary::Pair& kv, query.Fields) {
			Value value;

			if (kv.second.IsEmpty() && !kv.second.IsString())
				continue;

			if (!FieldToEscapedString(kv.first, kv.second, &value)) {
				m_QueryQueue.Enqueue(boost::bind(&IdoMysqlConnection::InternalExecuteQuery, this, query, (DbQueryType *)NULL), query.Priority);
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

	AsyncQuery(qbuf.str(), boost::bind(&IdoMysqlConnection::FinishExecuteQuery, this, query, type, upsert));
}

void IdoMysqlConnection::FinishExecuteQuery(const DbQuery& query, int type, bool upsert)
{
	if (upsert && GetAffectedRows() == 0) {
		DbQueryType to = DbQueryInsert;
		InternalExecuteQuery(query, &to);

		return;
	}

	if (type == DbQueryInsert && query.Object) {
		if (query.ConfigUpdate) {
			SetInsertID(query.Object, GetLastInsertID());
			SetConfigUpdate(query.Object, true);
		} else if (query.StatusUpdate)
			SetStatusUpdate(query.Object, true);
	}

	if (type == DbQueryInsert && query.Table == "notifications" && query.NotificationObject) { // FIXME remove hardcoded table name
		SetNotificationInsertID(query.NotificationObject, GetLastInsertID());
		Log(LogDebug, "IdoMysqlConnection")
		    << "saving contactnotification notification_id=" << static_cast<long>(GetLastInsertID());
	}
}

void IdoMysqlConnection::CleanUpExecuteQuery(const String& table, const String& time_column, double max_age)
{
	m_QueryQueue.Enqueue(boost::bind(&IdoMysqlConnection::InternalCleanUpExecuteQuery, this, table, time_column, max_age), PriorityLow, true);
}

void IdoMysqlConnection::InternalCleanUpExecuteQuery(const String& table, const String& time_column, double max_age)
{
	AssertOnWorkQueue();

	if (!GetConnected())
		return;

	AsyncQuery("DELETE FROM " + GetTablePrefix() + table + " WHERE instance_id = " +
	    Convert::ToString(static_cast<long>(m_InstanceID)) + " AND " + time_column +
	    " < FROM_UNIXTIME(" + Convert::ToString(static_cast<long>(max_age)) + ")");
}

void IdoMysqlConnection::FillIDCache(const DbType::Ptr& type)
{
	String query = "SELECT " + type->GetIDColumn() + " AS object_id, " + type->GetTable() + "_id FROM " + GetTablePrefix() + type->GetTable() + "s";
	IdoMysqlResult result = Query(query);

	Dictionary::Ptr row;

	while ((row = FetchRow(result))) {
		SetInsertID(type, DbReference(row->Get("object_id")), DbReference(row->Get(type->GetTable() + "_id")));
	}
}

int IdoMysqlConnection::GetPendingQueryCount(void) const
{
	return m_QueryQueue.GetLength();
}
