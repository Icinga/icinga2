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

#ifndef IDOMYSQLCONNECTION_H
#define IDOMYSQLCONNECTION_H

#include "db_ido_mysql/idomysqlconnection.thpp"
#include "mysql_shim/mysqlinterface.hpp"
#include "base/array.hpp"
#include "base/timer.hpp"
#include "base/workqueue.hpp"
#include "base/library.hpp"

namespace icinga
{

typedef std::shared_ptr<MYSQL_RES> IdoMysqlResult;

typedef std::function<void (const IdoMysqlResult&)> IdoAsyncCallback;

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
class IdoMysqlConnection final : public ObjectImpl<IdoMysqlConnection>
{
public:
	DECLARE_OBJECT(IdoMysqlConnection);
	DECLARE_OBJECTNAME(IdoMysqlConnection);

	IdoMysqlConnection();

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	virtual int GetPendingQueryCount() const override;

protected:
	virtual void OnConfigLoaded() override;
	virtual void Resume() override;
	virtual void Pause() override;

	virtual void ActivateObject(const DbObject::Ptr& dbobj) override;
	virtual void DeactivateObject(const DbObject::Ptr& dbobj) override;
	virtual void ExecuteQuery(const DbQuery& query) override;
	virtual void ExecuteMultipleQueries(const std::vector<DbQuery>& queries) override;
	virtual void CleanUpExecuteQuery(const String& table, const String& time_key, double time_value) override;
	virtual void FillIDCache(const DbType::Ptr& type) override;
	virtual void NewTransaction() override;

private:
	DbReference m_InstanceID;

	WorkQueue m_QueryQueue;

	Library m_Library;
	std::unique_ptr<MysqlInterface, MysqlInterfaceDeleter> m_Mysql;

	MYSQL m_Connection;
	int m_AffectedRows;
	unsigned int m_MaxPacketSize;

	std::vector<IdoAsyncQuery> m_AsyncQueries;

	Timer::Ptr m_ReconnectTimer;
	Timer::Ptr m_TxTimer;

	IdoMysqlResult Query(const String& query);
	DbReference GetLastInsertID();
	int GetAffectedRows();
	String Escape(const String& s);
	Dictionary::Ptr FetchRow(const IdoMysqlResult& result);
	void DiscardRows(const IdoMysqlResult& result);

	void AsyncQuery(const String& query, const IdoAsyncCallback& callback = IdoAsyncCallback());
	void FinishAsyncQueries();

	bool FieldToEscapedString(const String& key, const Value& value, Value *result);
	void InternalActivateObject(const DbObject::Ptr& dbobj);
	void InternalDeactivateObject(const DbObject::Ptr& dbobj);

	void Disconnect();
	void Reconnect();

	void AssertOnWorkQueue();

	void TxTimerHandler();
	void ReconnectTimerHandler();

	bool CanExecuteQuery(const DbQuery& query);

	void InternalExecuteQuery(const DbQuery& query, int typeOverride = -1);
	void InternalExecuteMultipleQueries(const std::vector<DbQuery>& queries);

	void FinishExecuteQuery(const DbQuery& query, int type, bool upsert);
	void InternalCleanUpExecuteQuery(const String& table, const String& time_key, double time_value);
	void InternalNewTransaction();

	void ClearTableBySession(const String& table);
	void ClearTablesBySession();

	void ExceptionHandler(boost::exception_ptr exp);

	void FinishConnect(double startTime);
};

}

#endif /* IDOMYSQLCONNECTION_H */
