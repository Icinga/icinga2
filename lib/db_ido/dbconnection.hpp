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

#ifndef DBCONNECTION_H
#define DBCONNECTION_H

#include "db_ido/i2-db_ido.hpp"
#include "db_ido/dbconnection.thpp"
#include "db_ido/dbobject.hpp"
#include "db_ido/dbquery.hpp"
#include "base/timer.hpp"
#include <boost/thread/once.hpp>

namespace icinga
{

/**
 * A database connection.
 *
 * @ingroup db_ido
 */
class I2_DB_IDO_API DbConnection : public ObjectImpl<DbConnection>
{
public:
	DECLARE_OBJECT(DbConnection);

	static void InitializeDbTimer(void);

	void SetObjectID(const DbObject::Ptr& dbobj, const DbReference& dbref);
	DbReference GetObjectID(const DbObject::Ptr& dbobj) const;

	void SetInsertID(const DbObject::Ptr& dbobj, const DbReference& dbref);
	void SetInsertID(const DbType::Ptr& type, const DbReference& objid, const DbReference& dbref);
	DbReference GetInsertID(const DbObject::Ptr& dbobj) const;
	DbReference GetInsertID(const DbType::Ptr& type, const DbReference& objid) const;

	void SetNotificationInsertID(const CustomVarObject::Ptr& obj, const DbReference& dbref);
	DbReference GetNotificationInsertID(const CustomVarObject::Ptr& obj) const;

	void SetObjectActive(const DbObject::Ptr& dbobj, bool active);
	bool GetObjectActive(const DbObject::Ptr& dbobj) const;

	void ClearIDCache(void);

	void SetConfigUpdate(const DbObject::Ptr& dbobj, bool hasupdate);
	bool GetConfigUpdate(const DbObject::Ptr& dbobj) const;

	void SetStatusUpdate(const DbObject::Ptr& dbobj, bool hasupdate);
	bool GetStatusUpdate(const DbObject::Ptr& dbobj) const;

	static void ValidateFailoverTimeout(const String& location, const DbConnection::Ptr& object);

protected:
	virtual void OnConfigLoaded(void);
	virtual void Start(void);
	virtual void Resume(void);
	virtual void Pause(void);

	virtual void ExecuteQuery(const DbQuery& query) = 0;
	virtual void ActivateObject(const DbObject::Ptr& dbobj) = 0;
	virtual void DeactivateObject(const DbObject::Ptr& dbobj) = 0;

	virtual void CleanUpExecuteQuery(const String& table, const String& time_column, double max_age);
	virtual void FillIDCache(const DbType::Ptr& type) = 0;

	void UpdateAllObjects(void);

	void PrepareDatabase(void);

private:
	std::map<DbObject::Ptr, DbReference> m_ObjectIDs;
	std::map<std::pair<DbType::Ptr, DbReference>, DbReference> m_InsertIDs;
	std::map<CustomVarObject::Ptr, DbReference> m_NotificationInsertIDs;
	std::set<DbObject::Ptr> m_ActiveObjects;
	std::set<DbObject::Ptr> m_ConfigUpdates;
	std::set<DbObject::Ptr> m_StatusUpdates;
	Timer::Ptr m_CleanUpTimer;

	void CleanUpHandler(void);

	virtual void ClearConfigTable(const String& table) = 0;

	static Timer::Ptr m_ProgramStatusTimer;
	static boost::once_flag m_OnceFlag;

	static void InsertRuntimeVariable(const String& key, const Value& value);
	static void ProgramStatusHandler(void);
};

struct database_error : virtual std::exception, virtual boost::exception { };

struct errinfo_database_query_;
typedef boost::error_info<struct errinfo_database_query_, std::string> errinfo_database_query;

}

#endif /* DBCONNECTION_H */
