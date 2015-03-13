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

#ifndef IDOMYSQLCONNECTION_H
#define IDOMYSQLCONNECTION_H

#include "db_ido_mysql/idomysqlconnection.thpp"
#include "base/array.hpp"
#include "base/timer.hpp"
#include "base/workqueue.hpp"
#include <mysql.h>

namespace icinga
{

typedef boost::shared_ptr<MYSQL_RES> IdoMysqlResult;

typedef boost::function<void (const IdoMysqlResult&)> IdoAsyncCallback;

struct IdoAsyncQuery
{
	String Query;
	IdoAsyncCallback Callback;
};

/**
 * An IDO MySQL database connection.
 *
 * @ingroup ido
 */
class IdoMysqlConnection : public ObjectImpl<IdoMysqlConnection>
{
public:
	DECLARE_OBJECT(IdoMysqlConnection);
	DECLARE_OBJECTNAME(IdoMysqlConnection);

	IdoMysqlConnection(void);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	virtual int GetPendingQueryCount(void) const;

protected:
	virtual void Resume(void);
	virtual void Pause(void);

	virtual void ActivateObject(const DbObject::Ptr& dbobj);
	virtual void DeactivateObject(const DbObject::Ptr& dbobj);
	virtual void ExecuteQuery(const DbQuery& query);
	virtual void CleanUpExecuteQuery(const String& table, const String& time_key, double time_value);
	virtual void FillIDCache(const DbType::Ptr& type);
	virtual void NewTransaction(void);

private:
	DbReference m_InstanceID;

	WorkQueue m_QueryQueue;

	MYSQL m_Connection;
	int m_AffectedRows;
	int m_MaxPacketSize;

	std::vector<IdoAsyncQuery> m_AsyncQueries;

	Timer::Ptr m_ReconnectTimer;
	Timer::Ptr m_TxTimer;

	IdoMysqlResult Query(const String& query);
	DbReference GetLastInsertID(void);
	int GetAffectedRows(void);
	String Escape(const String& s);
	Dictionary::Ptr FetchRow(const IdoMysqlResult& result);
	void DiscardRows(const IdoMysqlResult& result);

	void AsyncQuery(const String& query, const IdoAsyncCallback& callback = IdoAsyncCallback());
	void FinishAsyncQueries(bool force = false);

	bool FieldToEscapedString(const String& key, const Value& value, Value *result);
	void InternalActivateObject(const DbObject::Ptr& dbobj);
	void InternalDeactivateObject(const DbObject::Ptr& dbobj);

	void Disconnect(void);
	void Reconnect(void);

	void AssertOnWorkQueue(void);

	void TxTimerHandler(void);
	void ReconnectTimerHandler(void);

	void InternalExecuteQuery(const DbQuery& query, DbQueryType *typeOverride = NULL);
	void FinishExecuteQuery(const DbQuery& query, int type, bool upsert);
	void InternalCleanUpExecuteQuery(const String& table, const String& time_key, double time_value);
	void InternalNewTransaction(void);

	virtual void ClearConfigTable(const String& table);

	void ExceptionHandler(boost::exception_ptr exp);
};

}

#endif /* IDOMYSQLCONNECTION_H */
