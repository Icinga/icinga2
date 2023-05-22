/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "db_ido_mysql/idomysqlconnection-ti.hpp"
#include "mysql_shim/mysqlinterface.hpp"
#include "base/array.hpp"
#include "base/timer.hpp"
#include "base/workqueue.hpp"
#include "base/library.hpp"
#include <cstdint>

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

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	const char * GetLatestSchemaVersion() const noexcept override;
	const char * GetCompatSchemaVersion() const noexcept override;

	int GetPendingQueryCount() const override;

protected:
	void OnConfigLoaded() override;
	void Resume() override;
	void Pause() override;

	void ActivateObject(const DbObject::Ptr& dbobj) override;
	void DeactivateObject(const DbObject::Ptr& dbobj) override;
	void ExecuteQuery(const DbQuery& query) override;
	void ExecuteMultipleQueries(const std::vector<DbQuery>& queries) override;
	void CleanUpExecuteQuery(const String& table, const String& time_key, double time_value) override;
	void FillIDCache(const DbType::Ptr& type) override;
	void NewTransaction() override;
	void Disconnect() override;

private:
	DbReference m_InstanceID;

	Library m_Library;
	std::unique_ptr<MysqlInterface, MysqlInterfaceDeleter> m_Mysql;

	MYSQL m_Connection;
	int m_AffectedRows;
	unsigned int m_MaxPacketSize;

	std::vector<IdoAsyncQuery> m_AsyncQueries;
	uint_fast32_t m_UncommittedAsyncQueries = 0;

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

	void Reconnect();

	void AssertOnWorkQueue();

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
