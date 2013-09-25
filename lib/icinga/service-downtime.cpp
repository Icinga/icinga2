/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "icinga/service.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/timer.h"
#include "base/utility.h"
#include <boost/tuple/tuple.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

static int l_NextDowntimeID = 1;
static boost::mutex l_DowntimeMutex;
static std::map<int, String> l_LegacyDowntimesCache;
static std::map<String, Service::WeakPtr> l_DowntimesCache;
static Timer::Ptr l_DowntimesExpireTimer;

boost::signals2::signal<void (const Service::Ptr&, const Dictionary::Ptr&, const String&)> Service::OnDowntimeAdded;
boost::signals2::signal<void (const Service::Ptr&, const Dictionary::Ptr&, const String&)> Service::OnDowntimeRemoved;
boost::signals2::signal<void (const Service::Ptr&, const Dictionary::Ptr&)> Service::OnDowntimeTriggered;

int Service::GetNextDowntimeID(void)
{
	boost::mutex::scoped_lock lock(l_DowntimeMutex);

	return l_NextDowntimeID;
}

Dictionary::Ptr Service::GetDowntimes(void) const
{
	return m_Downtimes;
}

String Service::AddDowntime(const String& comment_id,
    double startTime, double endTime, bool fixed,
    const String& triggeredBy, double duration, const String& id, const String& authority)
{
	String uid;

	if (id.IsEmpty())
		uid = Utility::NewUniqueID();
	else
		uid = id;

	Dictionary::Ptr downtime = boost::make_shared<Dictionary>();
	downtime->Set("id", uid);
	downtime->Set("entry_time", Utility::GetTime());
	downtime->Set("comment_id", comment_id);
	downtime->Set("start_time", startTime);
	downtime->Set("end_time", endTime);
	downtime->Set("fixed", fixed);
	downtime->Set("duration", duration);
	downtime->Set("triggered_by", triggeredBy);
	downtime->Set("triggers", boost::make_shared<Dictionary>());
	downtime->Set("trigger_time", 0);

	int legacy_id;

	{
		boost::mutex::scoped_lock lock(l_DowntimeMutex);
		legacy_id = l_NextDowntimeID++;
	}

	downtime->Set("legacy_id", legacy_id);

	if (!triggeredBy.IsEmpty()) {
		Service::Ptr otherOwner = GetOwnerByDowntimeID(triggeredBy);
		Dictionary::Ptr otherDowntimes = otherOwner->m_Downtimes;
		Dictionary::Ptr otherDowntime = otherDowntimes->Get(triggeredBy);
		Dictionary::Ptr triggers = otherDowntime->Get("triggers");

		{
			ObjectLock olock(otherOwner);
			triggers->Set(triggeredBy, triggeredBy);
		}
	}

	Dictionary::Ptr downtimes;

	{
		ObjectLock olock(this);

		downtimes = m_Downtimes;

		if (!downtimes)
			downtimes = boost::make_shared<Dictionary>();

		m_Downtimes = downtimes;
	}

	downtimes->Set(uid, downtime);

	{
		boost::mutex::scoped_lock lock(l_DowntimeMutex);
		l_LegacyDowntimesCache[legacy_id] = uid;
		l_DowntimesCache[uid] = GetSelf();
	}

	Log(LogWarning, "icinga", "added downtime with ID '" + downtime->Get("legacy_id") + "'.");

	Utility::QueueAsyncCallback(boost::bind(boost::ref(OnDowntimeAdded), GetSelf(), downtime, authority));

	return uid;
}

void Service::RemoveDowntime(const String& id, bool cancelled, const String& authority)
{
	Service::Ptr owner = GetOwnerByDowntimeID(id);

	if (!owner)
		return;

	Dictionary::Ptr downtimes = owner->GetDowntimes();

	if (!downtimes)
		return;

	Dictionary::Ptr downtime = downtimes->Get(id);

	if (!downtime)
		return;

	String comment_id = downtime->Get("comment_id");

	int legacy_id = downtime->Get("legacy_id");

	downtimes->Remove(id);

	{
		boost::mutex::scoped_lock lock(l_DowntimeMutex);
		l_LegacyDowntimesCache.erase(legacy_id);
		l_DowntimesCache.erase(id);
	}

	RemoveComment(comment_id);

	downtime->Set("was_cancelled", cancelled);

	Log(LogWarning, "icinga", "removed downtime with ID '" + downtime->Get("legacy_id") + "' from service '" + owner->GetName() + "'.");

	Utility::QueueAsyncCallback(boost::bind(boost::ref(OnDowntimeRemoved), owner, downtime, authority));
}

void Service::TriggerDowntimes(void)
{
	Dictionary::Ptr downtimes = GetDowntimes();

	if (!downtimes)
		return;

	std::vector<String> ids;

	{
		ObjectLock olock(downtimes);

		String id;
		BOOST_FOREACH(boost::tie(id, boost::tuples::ignore), downtimes) {
			ids.push_back(id);
		}
	}

	BOOST_FOREACH(const String& id, ids) {
		TriggerDowntime(id);
	}
}

void Service::TriggerDowntime(const String& id)
{
	Service::Ptr owner = GetOwnerByDowntimeID(id);
	Dictionary::Ptr downtime = GetDowntimeByID(id);

	if (!downtime)
		return;

	if (IsDowntimeActive(downtime) && IsDowntimeTriggered(downtime)) {
		Log(LogDebug, "icinga", "Not triggering downtime with ID '" + downtime->Get("legacy_id") + "': already triggered.");
		return;
	}

	if (IsDowntimeExpired(downtime)) {
		Log(LogDebug, "icinga", "Not triggering downtime with ID '" + downtime->Get("legacy_id") + "': expired.");
		return;
	}

	double now = Utility::GetTime();

	if (now < downtime->Get("start_time") ||
	    now > downtime->Get("end_time"))
		return;

	Log(LogDebug, "icinga", "Triggering downtime with ID '" + downtime->Get("legacy_id") + "'.");

	if (downtime->Get("trigger_time") == 0)
		downtime->Set("trigger_time", now);

	Dictionary::Ptr triggers = downtime->Get("triggers");
	ObjectLock olock(triggers);
	String tid;
	BOOST_FOREACH(boost::tie(tid, boost::tuples::ignore), triggers) {
		TriggerDowntime(tid);
	}

	Utility::QueueAsyncCallback(boost::bind(boost::ref(OnDowntimeTriggered), owner, downtime));
}

String Service::GetDowntimeIDFromLegacyID(int id)
{
	boost::mutex::scoped_lock lock(l_DowntimeMutex);

	std::map<int, String>::iterator it = l_LegacyDowntimesCache.find(id);

	if (it == l_LegacyDowntimesCache.end())
		return Empty;

	return it->second;
}

Service::Ptr Service::GetOwnerByDowntimeID(const String& id)
{
	boost::mutex::scoped_lock lock(l_DowntimeMutex);
	return l_DowntimesCache[id].lock();
}

Dictionary::Ptr Service::GetDowntimeByID(const String& id)
{
	Service::Ptr owner = GetOwnerByDowntimeID(id);

	if (!owner)
		return Dictionary::Ptr();

	Dictionary::Ptr downtimes = owner->GetDowntimes();

	if (downtimes)
		return downtimes->Get(id);

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

bool Service::IsDowntimeTriggered(const Dictionary::Ptr& downtime)
{
	double now = Utility::GetTime();

	double triggerTime = downtime->Get("trigger_time");

	return (triggerTime > 0 && triggerTime <= now);
}

bool Service::IsDowntimeExpired(const Dictionary::Ptr& downtime)
{
	return (downtime->Get("end_time") < Utility::GetTime());
}

void Service::StartDowntimesExpiredTimer(void)
{
        if (!l_DowntimesExpireTimer) {
		l_DowntimesExpireTimer = boost::make_shared<Timer>();
		l_DowntimesExpireTimer->SetInterval(60);
		l_DowntimesExpireTimer->OnTimerExpired.connect(boost::bind(&Service::DowntimesExpireTimerHandler));
		l_DowntimesExpireTimer->Start();
        }
}

void Service::AddDowntimesToCache(void)
{
	Log(LogDebug, "icinga", "Updating Service downtimes cache.");

	Dictionary::Ptr downtimes = GetDowntimes();

	if (!downtimes)
		return;

	boost::mutex::scoped_lock lock(l_DowntimeMutex);

	ObjectLock olock(downtimes);

	String id;
	Dictionary::Ptr downtime;
	BOOST_FOREACH(boost::tie(id, downtime), downtimes) {
		int legacy_id = downtime->Get("legacy_id");

		if (legacy_id >= l_NextDowntimeID)
			l_NextDowntimeID = legacy_id + 1;

		l_LegacyDowntimesCache[legacy_id] = id;
		l_DowntimesCache[id] = GetSelf();
	}
}

void Service::RemoveExpiredDowntimes(void)
{
	Dictionary::Ptr downtimes = GetDowntimes();

	if (!downtimes)
		return;

	std::vector<String> expiredDowntimes;

	{
		ObjectLock olock(downtimes);

		String id;
		Dictionary::Ptr downtime;
		BOOST_FOREACH(boost::tie(id, downtime), downtimes) {
			if (IsDowntimeExpired(downtime))
				expiredDowntimes.push_back(id);
		}
	}

	BOOST_FOREACH(const String& id, expiredDowntimes) {
		RemoveDowntime(id, false);
	}
}

void Service::DowntimesExpireTimerHandler(void)
{
	BOOST_FOREACH(const Service::Ptr& service, DynamicType::GetObjects<Service>()) {
		service->RemoveExpiredDowntimes();
	}
}

bool Service::IsInDowntime(void) const
{
	Dictionary::Ptr downtimes = GetDowntimes();

	if (!downtimes)
		return false;

	ObjectLock olock(downtimes);

	Dictionary::Ptr downtime;
	BOOST_FOREACH(boost::tie(boost::tuples::ignore, downtime), downtimes) {
		if (Service::IsDowntimeActive(downtime))
			return true;
	}

	return false;
}

int Service::GetDowntimeDepth(void) const
{
	int downtime_depth = 0;
	Dictionary::Ptr downtimes = GetDowntimes();

	if (!downtimes)
		return 0;

	ObjectLock olock(downtimes);

	Dictionary::Ptr downtime;
	BOOST_FOREACH(boost::tie(boost::tuples::ignore, downtime), downtimes) {
		if (Service::IsDowntimeActive(downtime))
			downtime_depth++;
	}

	return downtime_depth;
}
