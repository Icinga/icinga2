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

#ifndef IDOMYSQLCONNECTION_H
#define IDOMYSQLCONNECTION_H

#include "base/array.h"
#include "base/dynamictype.h"
#include "base/timer.h"
#include "db_ido/dbconnection.h"
#include <mysql/mysql.h>

namespace icinga
{

/**
 * An IDO MySQL database connection.
 *
 * @ingroup ido
 */
class IdoMysqlConnection : public DbConnection
{
public:
	DECLARE_PTR_TYPEDEFS(IdoMysqlConnection);

	//virtual void UpdateObject(const DbObject::Ptr& dbobj, DbUpdateType kind);

protected:
	virtual void Start(void);
	virtual void Stop(void);

	virtual void InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const;
	virtual void InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes);

	virtual void ActivateObject(const DbObject::Ptr& dbobj);
	virtual void DeactivateObject(const DbObject::Ptr& dbobj);
	virtual void ExecuteQuery(const DbQuery& query);
        virtual void CleanUpExecuteQuery(const String& table, const String& time_key, double time_value);

private:
	String m_Host;
	Value m_Port;
	String m_User;
	String m_Password;
	String m_Database;
	String m_InstanceName;
	String m_InstanceDescription;

	DbReference m_InstanceID;
        DbReference m_LastNotificationID;

	boost::mutex m_ConnectionMutex;
	bool m_Connected;
	MYSQL m_Connection;

        String m_RequiredSchemaVersion;

	Timer::Ptr m_ReconnectTimer;
	Timer::Ptr m_TxTimer;

	Array::Ptr Query(const String& query);
	DbReference GetLastInsertID(void);
	String Escape(const String& s);
	Dictionary::Ptr FetchRow(MYSQL_RES *result);

	bool FieldToEscapedString(const String& key, const Value& value, Value *result);
	void InternalActivateObject(const DbObject::Ptr& dbobj);

	void TxTimerHandler(void);
	void ReconnectTimerHandler(void);

	void ClearConfigTables(void);
	void ClearConfigTable(const String& table);
};

}

#endif /* IDOMYSQLCONNECTION_H */
