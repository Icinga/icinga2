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

#include "i2-icinga.h"

using namespace icinga;

int Service::m_NextDowntimeID = 1;
map<int, String> Service::m_LegacyDowntimesCache;
map<String, Service::WeakPtr> Service::m_DowntimesCache;
bool Service::m_DowntimesCacheValid;
Timer::Ptr Service::m_DowntimesExpireTimer;

int Service::GetNextDowntimeID(void)
{
	return m_NextDowntimeID;
}

Dictionary::Ptr Service::GetDowntimes(void) const
{
	return m_Downtimes;
}

String Service::AddDowntime(const String& author, const String& comment,
    double startTime, double endTime, bool fixed,
    const String& triggeredBy, double duration)
{
	Dictionary::Ptr downtime = boost::make_shared<Dictionary>();
	downtime->Set("entry_time", Utility::GetTime());
	downtime->Set("author", author);
	downtime->Set("comment", comment);
	downtime->Set("start_time", startTime);
	downtime->Set("end_time", endTime);
	downtime->Set("fixed", fixed);
	downtime->Set("duration", duration);
	downtime->Set("triggered_by", triggeredBy);
	downtime->Set("triggers", boost::make_shared<Dictionary>());
	downtime->Set("trigger_time", 0);
	downtime->Set("legacy_id", m_NextDowntimeID++);

	if (!triggeredBy.IsEmpty()) {
		Service::Ptr otherOwner = GetOwnerByDowntimeID(triggeredBy);
		Dictionary::Ptr otherDowntimes = otherOwner->Get("downtimes");
		Dictionary::Ptr otherDowntime = otherDowntimes->Get(triggeredBy);
		Dictionary::Ptr triggers = otherDowntime->Get("triggers");
		triggers->Set(triggeredBy, triggeredBy);
		otherOwner->Touch("downtimes");
	}

	Dictionary::Ptr downtimes = m_Downtimes;

	if (!downtimes)
		downtimes = boost::make_shared<Dictionary>();

	String id = Utility::NewUUID();
	downtimes->Set(id, downtime);

	m_Downtimes = downtimes;
	Touch("downtimes");

	return id;
}

void Service::RemoveDowntime(const String& id)
{
	Service::Ptr owner = GetOwnerByDowntimeID(id);

	if (!owner)
		return;

	Dictionary::Ptr downtimes = owner->m_Downtimes;

	if (!downtimes)
		return;

	downtimes->Remove(id);
	owner->Touch("downtimes");
}

void Service::TriggerDowntimes(void)
{
	Dictionary::Ptr downtimes = m_Downtimes;

	if (!downtimes)
		return;

	String id;
	BOOST_FOREACH(tie(id, tuples::ignore), downtimes) {
		TriggerDowntime(id);
	}
}

void Service::TriggerDowntime(const String& id)
{
	Service::Ptr owner = GetOwnerByDowntimeID(id);
	Dictionary::Ptr downtime = GetDowntimeByID(id);

	double now = Utility::GetTime();

	if (now < downtime->Get("start_time") ||
	    now > downtime->Get("end_time"))
		return;

	if (downtime->Get("trigger_time") == 0)
		downtime->Set("trigger_time", now);

	Dictionary::Ptr triggers = downtime->Get("triggers");
	String tid;
	BOOST_FOREACH(tie(tid, tuples::ignore), triggers) {
		TriggerDowntime(tid);
	}

	owner->Touch("downtimes");
}

String Service::GetDowntimeIDFromLegacyID(int id)
{
	ValidateDowntimesCache();

	map<int, String>::iterator it = m_LegacyDowntimesCache.find(id);

	if (it == m_LegacyDowntimesCache.end())
		return Empty;

	return it->second;
}

Service::Ptr Service::GetOwnerByDowntimeID(const String& id)
{
	ValidateDowntimesCache();

	return m_DowntimesCache[id].lock();
}

Dictionary::Ptr Service::GetDowntimeByID(const String& id)
{
	Service::Ptr owner = GetOwnerByDowntimeID(id);

	if (!owner)
		return Dictionary::Ptr();

	Dictionary::Ptr downtimes = owner->m_Downtimes;

	if (downtimes) {
		Dictionary::Ptr downtime = downtimes->Get(id);
		return downtime;
	}

	return Dictionary::Ptr();
}

bool Service::IsDowntimeActive(const Dictionary::Ptr& downtime)
{
	double now = Utility::GetTime();

	if (now < downtime->Get("start_time") ||
	    now > downtime->Get("end_time"))
		return false;

	if (static_cast<bool>(downtime->Get("fixed")))
		return true;

	double triggerTime = downtime->Get("trigger_time");

	if (triggerTime == 0)
		return false;

	return (triggerTime + downtime->Get("duration") < now);
}

bool Service::IsDowntimeExpired(const Dictionary::Ptr& downtime)
{
	return (downtime->Get("end_time") < Utility::GetTime());
}

void Service::InvalidateDowntimesCache(void)
{
	m_DowntimesCacheValid = false;
	m_DowntimesCache.clear();
	m_LegacyDowntimesCache.clear();
}

void Service::AddDowntimesToCache(void)
{
	Dictionary::Ptr downtimes = m_Downtimes;

	if (!downtimes)
		return;

	String id;
	Dictionary::Ptr downtime;
	BOOST_FOREACH(tie(id, downtime), downtimes) {
		int legacy_id = downtime->Get("legacy_id");

		if (legacy_id >= m_NextDowntimeID)
			m_NextDowntimeID = legacy_id + 1;

		if (m_LegacyDowntimesCache.find(legacy_id) != m_LegacyDowntimesCache.end()) {
			/* The legacy_id is already in use by another downtime;
			 * this shouldn't usually happen - assign it a new ID. */
			legacy_id = m_NextDowntimeID++;
			downtime->Set("legacy_id", legacy_id);
			Touch("downtimes");
		}

		m_LegacyDowntimesCache[legacy_id] = id;
		m_DowntimesCache[id] = GetSelf();
	}
}

void Service::ValidateDowntimesCache(void)
{
	if (m_DowntimesCacheValid)
		return;

	m_DowntimesCache.clear();
	m_LegacyDowntimesCache.clear();

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		Service::Ptr service = dynamic_pointer_cast<Service>(object);
		service->AddDowntimesToCache();
	}

	m_DowntimesCacheValid = true;

	if (!m_DowntimesExpireTimer) {
		m_DowntimesExpireTimer = boost::make_shared<Timer>();
		m_DowntimesExpireTimer->SetInterval(300);
		m_DowntimesExpireTimer->OnTimerExpired.connect(boost::bind(&Service::DowntimesExpireTimerHandler));
		m_DowntimesExpireTimer->Start();
	}
}

void Service::RemoveExpiredDowntimes(void)
{
	Dictionary::Ptr downtimes = m_Downtimes;

	if (!downtimes)
		return;

	vector<String> expiredDowntimes;

	String id;
	Dictionary::Ptr downtime;
	BOOST_FOREACH(tie(id, downtime), downtimes) {
		if (IsDowntimeExpired(downtime))
			expiredDowntimes.push_back(id);
	}

	if (expiredDowntimes.size() > 0) {
		BOOST_FOREACH(id, expiredDowntimes) {
			downtimes->Remove(id);
		}

		Touch("downtimes");
	}
}

void Service::DowntimesExpireTimerHandler(void)
{
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		Service::Ptr service = dynamic_pointer_cast<Service>(object);
		ObjectLock slock(service);
		service->RemoveExpiredDowntimes();
	}
}

bool Service::IsInDowntime(void) const
{
	Dictionary::Ptr downtimes = GetDowntimes();

	if (!downtimes)
		return false;

	Dictionary::Ptr downtime;
	BOOST_FOREACH(tie(tuples::ignore, downtime), downtimes) {
		if (Service::IsDowntimeActive(downtime))
			return true;
	}

	return false;
}
