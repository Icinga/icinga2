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
#include "base/convert.h"
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

static int l_NextDowntimeID = 1;
static boost::mutex l_DowntimeMutex;
static std::map<int, String> l_LegacyDowntimesCache;
static std::map<String, Service::WeakPtr> l_DowntimesCache;
static Timer::Ptr l_DowntimesExpireTimer;

boost::signals2::signal<void (const Service::Ptr&, const Downtime::Ptr&, const String&)> Service::OnDowntimeAdded;
boost::signals2::signal<void (const Service::Ptr&, const Downtime::Ptr&, const String&)> Service::OnDowntimeRemoved;
boost::signals2::signal<void (const Service::Ptr&, const Downtime::Ptr&)> Service::OnDowntimeTriggered;

int Service::GetNextDowntimeID(void)
{
	boost::mutex::scoped_lock lock(l_DowntimeMutex);

	return l_NextDowntimeID;
}

String Service::AddDowntime(const String& author, const String& comment,
    double startTime, double endTime, bool fixed,
    const String& triggeredBy, double duration, const String& id, const String& authority)
{
	String uid;

	if (id.IsEmpty())
		uid = Utility::NewUniqueID();
	else
		uid = id;

	Downtime::Ptr downtime = make_shared<Downtime>();
	downtime->SetId(uid);
	downtime->SetEntryTime(Utility::GetTime());
	downtime->SetAuthor(author);
	downtime->SetComment(comment);
	downtime->SetStartTime(startTime);
	downtime->SetEndTime(endTime);
	downtime->SetFixed(fixed);
	downtime->SetDuration(duration);
	downtime->SetTriggeredBy(triggeredBy);

	int legacy_id;

	{
		boost::mutex::scoped_lock lock(l_DowntimeMutex);
		legacy_id = l_NextDowntimeID++;
	}

	downtime->SetLegacyId(legacy_id);

	if (!triggeredBy.IsEmpty()) {
		Service::Ptr otherOwner = GetOwnerByDowntimeID(triggeredBy);
		Dictionary::Ptr otherDowntimes = otherOwner->GetDowntimes();
		Downtime::Ptr otherDowntime = otherDowntimes->Get(triggeredBy);
		Dictionary::Ptr triggers = otherDowntime->GetTriggers();

		{
			ObjectLock olock(otherOwner);
			triggers->Set(triggeredBy, triggeredBy);
		}
	}

	GetDowntimes()->Set(uid, downtime);

	{
		boost::mutex::scoped_lock lock(l_DowntimeMutex);
		l_LegacyDowntimesCache[legacy_id] = uid;
		l_DowntimesCache[uid] = GetSelf();
	}

	Log(LogWarning, "icinga", "added downtime with ID '" + Convert::ToString(downtime->GetLegacyId()) + "'.");

	Utility::QueueAsyncCallback(boost::bind(boost::ref(OnDowntimeAdded), GetSelf(), downtime, authority));

	return uid;
}

void Service::RemoveDowntime(const String& id, bool cancelled, const String& authority)
{
	Service::Ptr owner = GetOwnerByDowntimeID(id);

	if (!owner)
		return;

	Dictionary::Ptr downtimes = owner->GetDowntimes();

	Downtime::Ptr downtime = downtimes->Get(id);

	if (!downtime)
		return;

	int legacy_id = downtime->GetLegacyId();

	downtimes->Remove(id);

	{
		boost::mutex::scoped_lock lock(l_DowntimeMutex);
		l_LegacyDowntimesCache.erase(legacy_id);
		l_DowntimesCache.erase(id);
	}

	downtime->SetWasCancelled(cancelled);

	Log(LogWarning, "icinga", "removed downtime with ID '" + Convert::ToString(downtime->GetLegacyId()) + "' from service '" + owner->GetName() + "'.");

	Utility::QueueAsyncCallback(boost::bind(boost::ref(OnDowntimeRemoved), owner, downtime, authority));
}

void Service::TriggerDowntimes(void)
{
	Dictionary::Ptr downtimes = GetDowntimes();

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
	Downtime::Ptr downtime = GetDowntimeByID(id);

	if (!downtime)
		return;

	if (downtime->IsActive() && downtime->IsTriggered()) {
		Log(LogDebug, "icinga", "Not triggering downtime with ID '" + Convert::ToString(downtime->GetLegacyId()) + "': already triggered.");
		return;
	}

	if (downtime->IsExpired()) {
		Log(LogDebug, "icinga", "Not triggering downtime with ID '" + Convert::ToString(downtime->GetLegacyId()) + "': expired.");
		return;
	}

	Log(LogDebug, "icinga", "Triggering downtime with ID '" + Convert::ToString(downtime->GetLegacyId()) + "'.");

	if (downtime->GetTriggerTime() == 0)
		downtime->SetTriggerTime(Utility::GetTime());

	Dictionary::Ptr triggers = downtime->GetTriggers();
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

Downtime::Ptr Service::GetDowntimeByID(const String& id)
{
	Service::Ptr owner = GetOwnerByDowntimeID(id);

	if (!owner)
		return Downtime::Ptr();

	Dictionary::Ptr downtimes = owner->GetDowntimes();

	if (downtimes)
		return downtimes->Get(id);

	return Downtime::Ptr();
}

void Service::StartDowntimesExpiredTimer(void)
{
        if (!l_DowntimesExpireTimer) {
		l_DowntimesExpireTimer = make_shared<Timer>();
		l_DowntimesExpireTimer->SetInterval(60);
		l_DowntimesExpireTimer->OnTimerExpired.connect(boost::bind(&Service::DowntimesExpireTimerHandler));
		l_DowntimesExpireTimer->Start();
        }
}

void Service::AddDowntimesToCache(void)
{
	Log(LogDebug, "icinga", "Updating Service downtimes cache.");

	Dictionary::Ptr downtimes = GetDowntimes();

	boost::mutex::scoped_lock lock(l_DowntimeMutex);

	ObjectLock olock(downtimes);

	String id;
	Downtime::Ptr downtime;
	BOOST_FOREACH(boost::tie(id, downtime), downtimes) {
		int legacy_id = downtime->GetLegacyId();

		if (legacy_id >= l_NextDowntimeID)
			l_NextDowntimeID = legacy_id + 1;

		l_LegacyDowntimesCache[legacy_id] = id;
		l_DowntimesCache[id] = GetSelf();
	}
}

void Service::RemoveExpiredDowntimes(void)
{
	Dictionary::Ptr downtimes = GetDowntimes();

	std::vector<String> expiredDowntimes;

	{
		ObjectLock olock(downtimes);

		String id;
		Downtime::Ptr downtime;
		BOOST_FOREACH(boost::tie(id, downtime), downtimes) {
			if (downtime->IsExpired())
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

	ObjectLock olock(downtimes);

	Downtime::Ptr downtime;
	BOOST_FOREACH(boost::tie(boost::tuples::ignore, downtime), downtimes) {
		if (downtime->IsActive())
			return true;
	}

	return false;
}

int Service::GetDowntimeDepth(void) const
{
	int downtime_depth = 0;
	Dictionary::Ptr downtimes = GetDowntimes();

	ObjectLock olock(downtimes);

	Downtime::Ptr downtime;
	BOOST_FOREACH(boost::tie(boost::tuples::ignore, downtime), downtimes) {
		if (downtime->IsActive())
			downtime_depth++;
	}

	return downtime_depth;
}
