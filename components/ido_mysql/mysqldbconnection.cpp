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
#include "ido/dbtype.h"
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

		std::ostringstream qbuf;
		qbuf << "SELECT object_id, objecttype_id, name1, name2 FROM icinga_objects WHERE instance_id = " << static_cast<long>(m_InstanceID);
		rows = Query(qbuf.str());

		ObjectLock olock(rows);
		BOOST_FOREACH(const Dictionary::Ptr& row, rows) {
			DbType::Ptr dbtype = DbType::GetByID(row->Get("objecttype_id"));

			if (!dbtype)
				continue;

			DbObject::Ptr dbobj = dbtype->GetOrCreateObjectByName(row->Get("name1"), row->Get("name2"));
			SetReference(dbobj, DbReference(row->Get("object_id")));
		}

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

		if (field) {
			value = String(row[i], row[i] + lengths[i]);
		}

		dict->Set(field->name, value);
	}

	return dict;
}

void MysqlDbConnection::UpdateObject(const DbObject::Ptr& dbobj, DbUpdateType kind) {
	boost::mutex::scoped_lock lock(m_ConnectionMutex);

	/* Check if we can handle updates right now */
	if (!m_Connected)
		return;

	DbReference dbref = GetReference(dbobj);

	if (kind == DbObjectRemoved) {
		if (!dbref.IsValid())
			return;

		std::ostringstream qbuf;
		qbuf << "DELETE FROM icinga_" << dbobj->GetType()->GetTable() << "s WHERE id = " << static_cast<long>(dbref);
		Log(LogWarning, "ido_mysql", "Query: " + qbuf.str());
	}

	if (kind == DbObjectCreated) {
		std::ostringstream q1buf;

		if (!dbref.IsValid()) {
			q1buf << "INSERT INTO icinga_objects (instance_id, objecttype_id, name1, name2, is_active) VALUES ("
			      << static_cast<long>(m_InstanceID) << ", " << dbobj->GetType()->GetTypeID() << ", "
			      << "'" << Escape(dbobj->GetName1()) << "', '" << Escape(dbobj->GetName2()) << "', 1)";
			Query(q1buf.str());
			dbref = GetInsertID();
		} else if (kind == DbObjectCreated) {
			q1buf << "UPDATE icinga_objects SET is_active = 1 WHERE object_id = " << static_cast<long>(dbref);
			Query(q1buf.str());
		}

		Dictionary::Ptr fields = dbobj->GetFields();

		if (!fields)
			return;

		String cols;
		String values;

		ObjectLock olock(fields);

		String key;
		Value value;
		BOOST_FOREACH(boost::tie(key, value), fields) {
			if (value.IsObjectType<DynamicObject>()) {
				DbObject::Ptr dbobjcol = DbObject::GetOrCreateByObject(value);

				if (!dbobjcol)
					return;

				DbReference dbrefcol = GetReference(dbobjcol);

				if (!dbrefcol.IsValid()) {
					UpdateObject(dbobjcol, DbObjectCreated);

					dbrefcol = GetReference(dbobjcol);

					if (!dbrefcol.IsValid())
						return;
				}

				value = static_cast<long>(dbrefcol);
			}

			cols += ", " + key;
			values += ", '" + Convert::ToString(value) + "'";
		}

		std::ostringstream q2buf;
		q2buf << "DELETE FROM icinga_" << dbobj->GetType()->GetTable() << "s WHERE " << dbobj->GetType()->GetTable() << "_object_id = " << static_cast<long>(dbref);
		Query(q2buf.str());

		std::ostringstream q3buf;
		q3buf << "INSERT INTO icinga_" << dbobj->GetType()->GetTable()
		      << "s (instance_id, " << dbobj->GetType()->GetTable() << "_object_id" << cols << ") VALUES ("
		      << static_cast<long>(m_InstanceID) << ", " << static_cast<long>(dbref) << values << ")";
		Query(q3buf.str());
	}

	// TODO: Object and status updates
}
