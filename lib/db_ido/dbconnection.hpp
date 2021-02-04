/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef DBCONNECTION_H
#define DBCONNECTION_H

#include "db_ido/i2-db_ido.hpp"
#include "db_ido/dbconnection-ti.hpp"
#include "db_ido/dbobject.hpp"
#include "db_ido/dbquery.hpp"
#include "base/timer.hpp"
#include "base/ringbuffer.hpp"
#include <boost/thread/once.hpp>
#include <mutex>

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

	static void InitializeDbTimer();

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

	void ClearIDCache();

	void SetConfigUpdate(const DbObject::Ptr& dbobj, bool hasupdate);
	bool GetConfigUpdate(const DbObject::Ptr& dbobj) const;

	void SetStatusUpdate(const DbObject::Ptr& dbobj, bool hasupdate);
	bool GetStatusUpdate(const DbObject::Ptr& dbobj) const;

	int GetQueryCount(RingBuffer::SizeType span);
	virtual int GetPendingQueryCount() const = 0;

	void ValidateFailoverTimeout(const Lazy<double>& lvalue, const ValidationUtils& utils) final;
	void ValidateCategories(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils) final;

protected:
	void OnConfigLoaded() override;
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;
	void Resume() override;
	void Pause() override;

	virtual void ExecuteQuery(const DbQuery& query) = 0;
	virtual void ExecuteMultipleQueries(const std::vector<DbQuery>&) = 0;
	virtual void ActivateObject(const DbObject::Ptr& dbobj) = 0;
	virtual void DeactivateObject(const DbObject::Ptr& dbobj) = 0;

	virtual void CleanUpExecuteQuery(const String& table, const String& time_column, double max_age);
	virtual void FillIDCache(const DbType::Ptr& type) = 0;
	virtual void NewTransaction() = 0;

	void UpdateObject(const ConfigObject::Ptr& object);
	void UpdateAllObjects();

	void PrepareDatabase();

	void IncreaseQueryCount();

	bool IsIDCacheValid() const;
	void SetIDCacheValid(bool valid);

	void EnableActiveChangedHandler();

	static void UpdateProgramStatus();

	static int GetSessionToken();

	void IncreasePendingQueries(int count);
	void DecreasePendingQueries(int count);

	WorkQueue m_QueryQueue{10000000, 1, LogNotice};

private:
	bool m_IDCacheValid{false};
	std::map<std::pair<DbType::Ptr, DbReference>, String> m_ConfigHashes;
	std::map<DbObject::Ptr, DbReference> m_ObjectIDs;
	std::map<std::pair<DbType::Ptr, DbReference>, DbReference> m_InsertIDs;
	std::set<DbObject::Ptr> m_ActiveObjects;
	std::set<DbObject::Ptr> m_ConfigUpdates;
	std::set<DbObject::Ptr> m_StatusUpdates;
	Timer::Ptr m_CleanUpTimer;
	Timer::Ptr m_LogStatsTimer;

	double m_LogStatsTimeout;

	void CleanUpHandler();
	void LogStatsHandler();

	static Timer::Ptr m_ProgramStatusTimer;
	static boost::once_flag m_OnceFlag;

	static void InsertRuntimeVariable(const String& key, const Value& value);

	mutable std::mutex m_StatsMutex;
	RingBuffer m_QueryStats{15 * 60};
	bool m_ActiveChangedHandler{false};

	RingBuffer m_InputQueries{10};
	RingBuffer m_OutputQueries{10};
	Atomic<uint_fast64_t> m_PendingQueries{0};
};

struct database_error : virtual std::exception, virtual boost::exception { };

struct errinfo_database_query_;
typedef boost::error_info<struct errinfo_database_query_, std::string> errinfo_database_query;

}

#endif /* DBCONNECTION_H */
