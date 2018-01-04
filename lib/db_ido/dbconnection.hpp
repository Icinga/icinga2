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

#ifndef DBCONNECTION_H
#define DBCONNECTION_H

#include "db_ido/i2-db_ido.hpp"
#include "db_ido/dbconnection.thpp"
#include "db_ido/dbobject.hpp"
#include "db_ido/dbquery.hpp"
#include "base/timer.hpp"
#include "base/ringbuffer.hpp"
#include <boost/thread/once.hpp>
#include <boost/thread/mutex.hpp>

#define IDO_CURRENT_SCHEMA_VERSION "1.14.3"
#define IDO_COMPAT_SCHEMA_VERSION "1.14.3"

namespace icinga
{

/**
 * A database connection.
 *
 * @ingroup db_ido
 */
class DbConnection : public ObjectImpl<DbConnection>
{
public:
	DECLARE_OBJECT(DbConnection);

	DbConnection(void);

	static void InitializeDbTimer(void);

	void SetConfigHash(const DbObject::Ptr& dbobj, const String& hash);
	void SetConfigHash(const DbType::Ptr& type, const DbReference& objid, const String& hash);
	String GetConfigHash(const DbObject::Ptr& dbobj) const;
	String GetConfigHash(const DbType::Ptr& type, const DbReference& objid) const;

	void SetObjectID(const DbObject::Ptr& dbobj, const DbReference& dbref);
	DbReference GetObjectID(const DbObject::Ptr& dbobj) const;

	void SetInsertID(const DbObject::Ptr& dbobj, const DbReference& dbref);
	void SetInsertID(const DbType::Ptr& type, const DbReference& objid, const DbReference& dbref);
	DbReference GetInsertID(const DbObject::Ptr& dbobj) const;
	DbReference GetInsertID(const DbType::Ptr& type, const DbReference& objid) const;

	void SetObjectActive(const DbObject::Ptr& dbobj, bool active);
	bool GetObjectActive(const DbObject::Ptr& dbobj) const;

	void ClearIDCache(void);

	void SetConfigUpdate(const DbObject::Ptr& dbobj, bool hasupdate);
	bool GetConfigUpdate(const DbObject::Ptr& dbobj) const;

	void SetStatusUpdate(const DbObject::Ptr& dbobj, bool hasupdate);
	bool GetStatusUpdate(const DbObject::Ptr& dbobj) const;

	int GetQueryCount(RingBuffer::SizeType span);
	virtual int GetPendingQueryCount(void) const = 0;

	virtual void ValidateFailoverTimeout(double value, const ValidationUtils& utils) override final;
	virtual void ValidateCategories(const Array::Ptr& value, const ValidationUtils& utils) override final;

protected:
	virtual void OnConfigLoaded(void) override;
	virtual void Start(bool runtimeCreated) override;
	virtual void Stop(bool runtimeRemoved) override;
	virtual void Resume(void) override;
	virtual void Pause(void) override;

	virtual void ExecuteQuery(const DbQuery& query) = 0;
	virtual void ExecuteMultipleQueries(const std::vector<DbQuery>&) = 0;
	virtual void ActivateObject(const DbObject::Ptr& dbobj) = 0;
	virtual void DeactivateObject(const DbObject::Ptr& dbobj) = 0;

	virtual void CleanUpExecuteQuery(const String& table, const String& time_column, double max_age);
	virtual void FillIDCache(const DbType::Ptr& type) = 0;
	virtual void NewTransaction(void) = 0;

	void UpdateObject(const ConfigObject::Ptr& object);
	void UpdateAllObjects(void);

	void PrepareDatabase(void);

	void IncreaseQueryCount(void);

	bool IsIDCacheValid(void) const;
	void SetIDCacheValid(bool valid);

	void EnableActiveChangedHandler(void);

	static void UpdateProgramStatus(void);

	static int GetSessionToken(void);

private:
	bool m_IDCacheValid;
	std::map<std::pair<DbType::Ptr, DbReference>, String> m_ConfigHashes;
	std::map<DbObject::Ptr, DbReference> m_ObjectIDs;
	std::map<std::pair<DbType::Ptr, DbReference>, DbReference> m_InsertIDs;
	std::set<DbObject::Ptr> m_ActiveObjects;
	std::set<DbObject::Ptr> m_ConfigUpdates;
	std::set<DbObject::Ptr> m_StatusUpdates;
	Timer::Ptr m_CleanUpTimer;

	void CleanUpHandler(void);

	static Timer::Ptr m_ProgramStatusTimer;
	static boost::once_flag m_OnceFlag;

	static void InsertRuntimeVariable(const String& key, const Value& value);

	mutable boost::mutex m_StatsMutex;
	RingBuffer m_QueryStats;
	bool m_ActiveChangedHandler;
};

struct database_error : virtual std::exception, virtual boost::exception { };

struct errinfo_database_query_;
typedef boost::error_info<struct errinfo_database_query_, std::string> errinfo_database_query;

}

#endif /* DBCONNECTION_H */
