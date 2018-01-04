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

#ifndef IDOPGSQLCONNECTION_H
#define IDOPGSQLCONNECTION_H

#include "db_ido_pgsql/idopgsqlconnection.thpp"
#include "pgsql_shim/pgsqlinterface.hpp"
#include "base/array.hpp"
#include "base/timer.hpp"
#include "base/workqueue.hpp"
#include "base/library.hpp"

namespace icinga
{

typedef std::shared_ptr<PGresult> IdoPgsqlResult;

/**
 * An IDO pgSQL database connection.
 *
 * @ingroup ido
 */
class IdoPgsqlConnection final : public ObjectImpl<IdoPgsqlConnection>
{
public:
	DECLARE_OBJECT(IdoPgsqlConnection);
	DECLARE_OBJECTNAME(IdoPgsqlConnection);

	IdoPgsqlConnection();

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

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

private:
	DbReference m_InstanceID;

	WorkQueue m_QueryQueue;

	Library m_Library;
	std::unique_ptr<PgsqlInterface, PgsqlInterfaceDeleter> m_Pgsql;

	PGconn *m_Connection;
	int m_AffectedRows;

	Timer::Ptr m_ReconnectTimer;
	Timer::Ptr m_TxTimer;

	IdoPgsqlResult Query(const String& query);
	DbReference GetSequenceValue(const String& table, const String& column);
	int GetAffectedRows();
	String Escape(const String& s);
	Dictionary::Ptr FetchRow(const IdoPgsqlResult& result, int row);

	bool FieldToEscapedString(const String& key, const Value& value, Value *result);
	void InternalActivateObject(const DbObject::Ptr& dbobj);
	void InternalDeactivateObject(const DbObject::Ptr& dbobj);

	void Disconnect();
	void InternalNewTransaction();
	void Reconnect();

	void AssertOnWorkQueue();

	void TxTimerHandler();
	void ReconnectTimerHandler();

	void StatsLoggerTimerHandler();

	bool CanExecuteQuery(const DbQuery& query);

	void InternalExecuteQuery(const DbQuery& query, int typeOverride = -1);
	void InternalExecuteMultipleQueries(const std::vector<DbQuery>& queries);
	void InternalCleanUpExecuteQuery(const String& table, const String& time_key, double time_value);

	void ClearTableBySession(const String& table);
	void ClearTablesBySession();

	void ExceptionHandler(boost::exception_ptr exp);

	void FinishConnect(double startTime);
};

}

#endif /* IDOPGSQLCONNECTION_H */
