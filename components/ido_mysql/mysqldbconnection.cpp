/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "base/logger_fwd.h"
#include "base/objectlock.h"
#include "base/convert.h"
#include "base/utility.h"
#include "ido/dbtype.h"
#include "ido/dbvalue.h"
#include "ido_mysql/mysqldbconnection.h"
#include <boost/tuple/tuple.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(MysqlDbConnection);

MysqlDbConnection::MysqlDbConnection(const Dictionary::Ptr& serializedUpdate)
	: DbConnection(serializedUpdate), m_Connected(false)
{
	RegisterAttribute("host", Attribute_Config, &m_Host);
	RegisterAttribute("port", Attribute_Config, &m_Port);
	RegisterAttribute("user", Attribute_Config, &m_User);
	RegisterAttribute("password", Attribute_Config, &m_Password);
	RegisterAttribute("database", Attribute_Config, &m_Database);

	RegisterAttribute("instance_name", Attribute_Config, &m_InstanceName);
	RegisterAttribute("instance_description", Attribute_Config, &m_InstanceDescription);

	m_TxTimer = boost::make_shared<Timer>();
	m_TxTimer->SetInterval(5);
	m_TxTimer->OnTimerExpired.connect(boost::bind(&MysqlDbConnection::TxTimerHandler, this));
	m_TxTimer->Start();

	m_ReconnectTimer = boost::make_shared<Timer>();
	m_ReconnectTimer->SetInterval(10);
	m_ReconnectTimer->OnTimerExpired.connect(boost::bind(&MysqlDbConnection::ReconnectTimerHandler, this));
	m_ReconnectTimer->Start();

	ASSERT(mysql_thread_safe());
}

void MysqlDbConnection::Stop(void)
{
	boost::mutex::scoped_lock lock(m_ConnectionMutex);

	if (!m_Connected)
		return;

	Query("COMMIT");
	mysql_close(&m_Connection);
}

void MysqlDbConnection::TxTimerHandler(void)
{
	boost::mutex::scoped_lock lock(m_ConnectionMutex);

	if (!m_Connected)
		return;

	Query("COMMIT");
	Query("BEGIN");
}

void MysqlDbConnection::ReconnectTimerHandler(void)
{
	{
		boost::mutex::scoped_lock lock(m_ConnectionMutex);

		if (m_Connected) {
			/* Check if we're really still connected */
			if (mysql_ping(&m_Connection) == 0)
				return;

			mysql_close(&m_Connection);
			m_Connected = false;
		}

		String ihost, iuser, ipasswd, idb;
		const char *host, *user , *passwd, *db;
		long port;

		ihost = m_Host;
		iuser = m_User;
		ipasswd = m_Password;
		idb = m_Database;

		host = (!ihost.IsEmpty()) ? ihost.CStr() : NULL;
		port = m_Port;
		user = (!iuser.IsEmpty()) ? iuser.CStr() : NULL;
		passwd = (!ipasswd.IsEmpty()) ? ipasswd.CStr() : NULL;
		db = (!idb.IsEmpty()) ? idb.CStr() : NULL;

		if (!mysql_init(&m_Connection))
			BOOST_THROW_EXCEPTION(std::bad_alloc());

		if (!mysql_real_connect(&m_Connection, host, user, passwd, db, port, NULL, 0))
			BOOST_THROW_EXCEPTION(std::runtime_error(mysql_error(&m_Connection)));

		m_Connected = true;

		String instanceName = "default";

		if (!m_InstanceName.IsEmpty())
			instanceName = m_InstanceName;

		Array::Ptr rows = Query("SELECT instance_id FROM icinga_instances WHERE instance_name = '" + Escape(instanceName) + "'");

		if (rows->GetLength() == 0) {
			Query("INSERT INTO icinga_instances (instance_name, instance_description) VALUES ('" + Escape(instanceName) + "', '" + m_InstanceDescription + "')");
			m_InstanceID = GetInsertID();
		} else {
			Dictionary::Ptr row = rows->Get(0);
			m_InstanceID = DbReference(row->Get("instance_id"));
		}

		std::ostringstream msgbuf;
		msgbuf << "MySQL IDO instance id: " << static_cast<long>(m_InstanceID);
		Log(LogInformation, "ido_mysql", msgbuf.str());

		Query("UPDATE icinga_objects SET is_active = 0");

		std::ostringstream q1buf;
		q1buf << "SELECT object_id, objecttype_id, name1, name2 FROM icinga_objects WHERE instance_id = " << static_cast<long>(m_InstanceID);
		rows = Query(q1buf.str());

		ObjectLock olock(rows);
		BOOST_FOREACH(const Dictionary::Ptr& row, rows) {
			DbType::Ptr dbtype = DbType::GetByID(row->Get("objecttype_id"));

			if (!dbtype)
				continue;

			DbObject::Ptr dbobj = dbtype->GetOrCreateObjectByName(row->Get("name1"), row->Get("name2"));
			SetReference(dbobj, DbReference(row->Get("object_id")));
		}

		// TODO: Use a timer, move to libido
		std::ostringstream q2buf;
		q2buf << "REPLACE INTO icinga_programstatus (instance_id, status_update_time) VALUES (" << static_cast<long>(m_InstanceID) << ", NOW())";
 		Query(q2buf.str());

		Query("BEGIN");
	}

	UpdateAllObjects();
}

Array::Ptr MysqlDbConnection::Query(const String& query)
{
	Log(LogDebug, "ido_mysql", "Query: " + query);

	if (mysql_query(&m_Connection, query.CStr()) != 0)
	    BOOST_THROW_EXCEPTION(std::runtime_error(mysql_error(&m_Connection)));

	MYSQL_RES *result = mysql_store_result(&m_Connection);

	if (!result) {
		if (mysql_field_count(&m_Connection) > 0)
			BOOST_THROW_EXCEPTION(std::runtime_error(mysql_error(&m_Connection)));

		return Array::Ptr();
	}

	Array::Ptr rows = boost::make_shared<Array>();

	for (;;) {
		Dictionary::Ptr row = FetchRow(result);

		if (!row)
			break;

		rows->Add(row);
	}

	mysql_free_result(result);

	return rows;
}

DbReference MysqlDbConnection::GetInsertID(void)
{
	return DbReference(mysql_insert_id(&m_Connection));
}

String MysqlDbConnection::Escape(const String& s)
{
	ssize_t length = s.GetLength();
	char *to = new char[s.GetLength() * 2 + 1];

	mysql_real_escape_string(&m_Connection, to, s.CStr(), length);

	return String(to);
}

Dictionary::Ptr MysqlDbConnection::FetchRow(MYSQL_RES *result)
{
	MYSQL_ROW row;
	MYSQL_FIELD *field;
	unsigned long *lengths, i;

	row = mysql_fetch_row(result);

	if (!row)
		return Dictionary::Ptr();

	lengths = mysql_fetch_lengths(result);

	if (!lengths)
		return Dictionary::Ptr();

	Dictionary::Ptr dict = boost::make_shared<Dictionary>();

	mysql_field_seek(result, 0);
	for (field = mysql_fetch_field(result), i = 0; field; field = mysql_fetch_field(result), i++) {
		Value value;

		if (field)
			value = String(row[i], row[i] + lengths[i]);

		dict->Set(field->name, value);
	}

	return dict;
}

void MysqlDbConnection::ActivateObject(const DbObject::Ptr& dbobj)
{
	boost::mutex::scoped_lock lock(m_ConnectionMutex);

	if (!m_Connected)
		return;

	DbReference dbref = GetReference(dbobj);
	std::ostringstream qbuf;

	if (!dbref.IsValid()) {
		qbuf << "INSERT INTO icinga_objects (instance_id, objecttype_id, name1, name2, is_active) VALUES ("
		      << static_cast<long>(m_InstanceID) << ", " << dbobj->GetType()->GetTypeID() << ", "
		      << "'" << Escape(dbobj->GetName1()) << "', '" << Escape(dbobj->GetName2()) << "', 1)";
		Query(qbuf.str());
		SetReference(dbobj, GetInsertID());
	} else {
		qbuf << "UPDATE icinga_objects SET is_active = 1 WHERE object_id = " << static_cast<long>(dbref);
		Query(qbuf.str());
	}
}

void MysqlDbConnection::DeactivateObject(const DbObject::Ptr& dbobj)
{
	boost::mutex::scoped_lock lock(m_ConnectionMutex);

	if (!m_Connected)
		return;

	DbReference dbref = GetReference(dbobj);

	if (!dbref.IsValid())
		return;

	std::ostringstream qbuf;
	qbuf << "UPDATE icinga_objects SET is_active = 0 WHERE object_id = " << static_cast<long>(dbref);
	Query(qbuf.str());

	SetReference(dbobj, DbReference());
}

bool MysqlDbConnection::FieldToEscapedString(const String& key, const Value& value, Value *result)
{
	if (key == "instance_id") {
		*result = static_cast<long>(m_InstanceID);
		return true;
	}

	if (value.IsObjectType<DynamicObject>()) {
		DbObject::Ptr dbobjcol = DbObject::GetOrCreateByObject(value);

		if (!dbobjcol) {
			*result = 0;
			return true;
		}

		DbReference dbrefcol = GetReference(dbobjcol);

		if (!dbrefcol.IsValid()) {
			ActivateObject(dbobjcol);

			dbrefcol = GetReference(dbobjcol);

			if (!dbrefcol.IsValid())
				return false;
		}

		*result = static_cast<long>(dbrefcol);
	} else if (DbValue::IsTimestamp(value)) {
		double ts = DbValue::ExtractValue(value);
		std::ostringstream msgbuf;
		msgbuf << "FROM_UNIXTIME(" << ts << ")";
		*result = Value(msgbuf.str());
	} else if (DbValue::IsTimestampNow(value)) {
		*result = "NOW()";
	} else {
		*result = "'" + Escape(DbValue::ExtractValue(value)) + "'";
	}

	return true;
}

void MysqlDbConnection::ExecuteQuery(const DbQuery& query)
{
	boost::mutex::scoped_lock lock(m_ConnectionMutex);

	if (!m_Connected)
		return;

	std::ostringstream qbuf;

	switch (query.Type) {
		case DbQueryInsert:
			qbuf << "INSERT INTO " << query.Table;
			break;
		case DbQueryUpdate:
			qbuf << "UPDATE " << query.Table << "SET";
			break;
		case DbQueryDelete:
			qbuf << "DELETE FROM " << query.Table;
			break;
		default:
			ASSERT(!"Invalid query type.");
	}

	if (query.Type == DbQueryInsert || query.Type == DbQueryUpdate) {
		String cols;
		String values;

		ObjectLock olock(query.Fields);

		String key;
		Value value;
		bool first = true;
		BOOST_FOREACH(boost::tie(key, value), query.Fields) {
			if (!FieldToEscapedString(key, value, &value))
				return;

			if (query.Type == DbQueryInsert) {
				if (!first) {
					cols += ", ";
					values += ", ";
				}

				cols += key;
				values += Convert::ToString(value);
			} else {
				if (!first)
					qbuf << ", ";

				qbuf << " " << key << " = " << value;
			}

			if (first)
				first = false;
		}

		if (query.Type == DbQueryInsert)
			qbuf << " (" << cols << ") VALUES (" << values << ")";
	}

	if (query.WhereCriteria) {
		qbuf << " WHERE ";

		ObjectLock olock(query.WhereCriteria);

		String key;
		Value value;
		bool first = true;
		BOOST_FOREACH(boost::tie(key, value), query.WhereCriteria) {
			if (!FieldToEscapedString(Empty, value, &value))
				return;

			if (!first)
				qbuf << " AND ";

			qbuf << key << " = '" << Escape(value) << "'";

			if (first)
				first = false;
		}
	}

	Query(qbuf.str());
}
