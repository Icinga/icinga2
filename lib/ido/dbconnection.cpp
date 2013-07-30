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

#include "ido/dbconnection.h"
#include "ido/dbvalue.h"
#include "icinga/icingaapplication.h"
#include "base/dynamictype.h"
#include "base/utility.h"
#include <boost/foreach.hpp>

using namespace icinga;

Timer::Ptr DbConnection::m_ProgramStatusTimer;

DbConnection::DbConnection(const Dictionary::Ptr& serializedUpdate)
	: DynamicObject(serializedUpdate)
{ }

void DbConnection::Start(void)
{
	DbObject::OnRegistered.connect(boost::bind(&DbConnection::ActivateObject, this, _1));
	DbObject::OnUnregistered.connect(boost::bind(&DbConnection::DeactivateObject, this, _1));
	DbObject::OnQuery.connect(boost::bind(&DbConnection::ExecuteQuery, this, _1));
}

void DbConnection::StaticInitialize(void)
{
	m_ProgramStatusTimer = boost::make_shared<Timer>();
	m_ProgramStatusTimer->SetInterval(10);
	m_ProgramStatusTimer->OnTimerExpired.connect(boost::bind(&DbConnection::ProgramStatusHandler));
	m_ProgramStatusTimer->Start();
}

void DbConnection::ProgramStatusHandler(void)
{
	DbQuery query1;
	query1.Table = "icinga_programstatus";
	query1.Type = DbQueryDelete;
	query1.WhereCriteria = boost::make_shared<Dictionary>();
	query1.WhereCriteria->Set("instance_id", 0);  /* DbConnection class fills in real ID */
	DbObject::OnQuery(query1);

	DbQuery query2;
	query2.Table = "icinga_programstatus";
	query2.Type = DbQueryInsert;

	query2.Fields = boost::make_shared<Dictionary>();
	query2.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */
	query2.Fields->Set("status_update_time", DbValue::FromTimestamp(Utility::GetTime()));
	query2.Fields->Set("program_start_time", DbValue::FromTimestamp(IcingaApplication::GetInstance()->GetStartTime()));
	query2.Fields->Set("is_currently_running", 1);
	query2.Fields->Set("process_id", Utility::GetPid());
	query2.Fields->Set("daemon_mode", 1);
	query2.Fields->Set("last_command_check", DbValue::FromTimestamp(Utility::GetTime()));
	query2.Fields->Set("notifications_enabled", 1);
	query2.Fields->Set("active_service_checks_enabled", 1);
	query2.Fields->Set("passive_service_checks_enabled", 1);
	query2.Fields->Set("event_handlers_enabled", 1);
	query2.Fields->Set("flap_detection_enabled", 1);
	query2.Fields->Set("failure_prediction_enabled", 1);
	query2.Fields->Set("process_performance_data", 1);
	DbObject::OnQuery(query2);
}

void DbConnection::SetReference(const DbObject::Ptr& dbobj, const DbReference& dbref)
{
	if (dbref.IsValid())
		m_References[dbobj] = dbref;
	else
		m_References.erase(dbobj);
}

DbReference DbConnection::GetReference(const DbObject::Ptr& dbobj) const
{
	std::map<DbObject::Ptr, DbReference>::const_iterator it;

	it = m_References.find(dbobj);

	if (it == m_References.end())
		return DbReference();

	return it->second;
}

void DbConnection::ExecuteQuery(const DbQuery&)
{
	/* Default handler does nothing. */
}

void DbConnection::UpdateAllObjects(void)
{
	DynamicType::Ptr type;
	BOOST_FOREACH(const DynamicType::Ptr& dt, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, dt->GetObjects()) {
			DbObject::Ptr dbobj = DbObject::GetOrCreateByObject(object);

			if (dbobj) {
				ActivateObject(dbobj);
				dbobj->SendConfigUpdate();
				dbobj->SendStatusUpdate();
			}
		}
	}
}
