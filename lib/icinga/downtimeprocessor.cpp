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

int DowntimeProcessor::m_NextDowntimeID = 1;
map<int, DynamicObject::WeakPtr> DowntimeProcessor::m_DowntimeCache;
bool DowntimeProcessor::m_DowntimeCacheValid;

int DowntimeProcessor::GetNextDowntimeID(void)
{
	return m_NextDowntimeID;
}

int DowntimeProcessor::AddDowntime(const DynamicObject::Ptr& owner,
    const String& author, const String& comment,
    double startTime, double endTime,
    bool fixed, int triggeredBy, double duration)
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
	downtime->Set("trigger_time", 0);

	Dictionary::Ptr downtimes = owner->Get("downtimes");

	if (!downtimes)
		downtimes = boost::make_shared<Dictionary>();

	int id = m_NextDowntimeID;
	m_NextDowntimeID++;

	downtimes->Set(Convert::ToString(id), downtime);
	owner->Set("downtimes", downtimes);

	return id;
}

void DowntimeProcessor::RemoveDowntime(int id)
{
	DynamicObject::Ptr owner = GetOwnerByDowntimeID(id);

	Dictionary::Ptr downtimes = owner->Get("downtimes");

	if (downtimes) {
		downtimes->Remove(Convert::ToString(id));
		owner->Touch("downtimes");
	}
}

DynamicObject::Ptr DowntimeProcessor::GetOwnerByDowntimeID(int id)
{
	ValidateDowntimeCache();

	return m_DowntimeCache[id].lock();
}

Dictionary::Ptr DowntimeProcessor::GetDowntimeByID(int id)
{
	DynamicObject::Ptr owner = GetOwnerByDowntimeID(id);

	if (!owner)
		throw_exception(invalid_argument("Downtime ID does not exist."));

	Dictionary::Ptr downtimes = owner->Get("downtimes");

	if (downtimes) {
		Dictionary::Ptr downtime = downtimes->Get(Convert::ToString(id));
		return downtime;
	}

	return Dictionary::Ptr();
}

bool DowntimeProcessor::IsDowntimeActive(const Dictionary::Ptr& downtime)
{
	double now = Utility::GetTime();

	if (now < static_cast<double>(downtime->Get("start_time")) ||
	    now > static_cast<double>(downtime->Get("end_time")))
		return false;

	if (static_cast<bool>(downtime->Get("fixed")))
		return true;

	double triggerTime = static_cast<double>(downtime->Get("trigger_time"));

	if (triggerTime == 0)
		return false;

	return (triggerTime + static_cast<double>(downtime->Get("duration")) < now);
}

void DowntimeProcessor::InvalidateDowntimeCache(void)
{
	m_DowntimeCacheValid = false;
	m_DowntimeCache.clear();
}

void DowntimeProcessor::AddDowntimesToCache(const DynamicObject::Ptr& owner)
{
	Dictionary::Ptr downtimes = owner->Get("downtimes");

	if (!downtimes)
		return;

	String sid;
	BOOST_FOREACH(tie(sid, tuples::ignore), downtimes) {
		int id = Convert::ToLong(sid);

		if (id > m_NextDowntimeID)
			m_NextDowntimeID = id;

		m_DowntimeCache[id] = owner;
	}
}

void DowntimeProcessor::ValidateDowntimeCache(void)
{
	if (m_DowntimeCacheValid)
		return;

	m_DowntimeCache.clear();

	DynamicObject::Ptr object;
	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Host")->GetObjects()) {
		AddDowntimesToCache(object);
	}

	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Service")->GetObjects()) {
		AddDowntimesToCache(object);
	}

	m_DowntimeCacheValid = true;
}

