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

#include "icinga/service.hpp"
#include "base/dynamictype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/timer.hpp"
#include "base/utility.hpp"
#include "base/convert.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

static int l_NextDowntimeID = 1;
static boost::mutex l_DowntimeMutex;
static std::map<int, String> l_LegacyDowntimesCache;
static std::map<String, Checkable::Ptr> l_DowntimesCache;
static Timer::Ptr l_DowntimesExpireTimer;

boost::signals2::signal<void (const Checkable::Ptr&, const Downtime::Ptr&, const MessageOrigin&)> Checkable::OnDowntimeAdded;
boost::signals2::signal<void (const Checkable::Ptr&, const Downtime::Ptr&, const MessageOrigin&)> Checkable::OnDowntimeRemoved;
boost::signals2::signal<void (const Checkable::Ptr&, const Downtime::Ptr&)> Checkable::OnDowntimeTriggered;

INITIALIZE_ONCE(&Checkable::StartDowntimesExpiredTimer);

int Checkable::GetNextDowntimeID(void)
{
	boost::mutex::scoped_lock lock(l_DowntimeMutex);

	return l_NextDowntimeID;
}

String Checkable::AddDowntime(const String& author, const String& comment,
    double startTime, double endTime, bool fixed,
    const String& triggeredBy, double duration, const String& scheduledBy,
    const String& id, const MessageOrigin& origin)
{
	String uid;

	if (id.IsEmpty())
		uid = Utility::NewUniqueID();
	else
		uid = id;

	Downtime::Ptr downtime = new Downtime();
	downtime->SetId(uid);
	downtime->SetEntryTime(Utility::GetTime());
	downtime->SetAuthor(author);
	downtime->SetComment(comment);
	downtime->SetStartTime(startTime);
	downtime->SetEndTime(endTime);
	downtime->SetFixed(fixed);
	downtime->SetDuration(duration);
	downtime->SetTriggeredBy(triggeredBy);
	downtime->SetScheduledBy(scheduledBy);

	if (!triggeredBy.IsEmpty()) {
		Downtime::Ptr triggerDowntime = GetDowntimeByID(triggeredBy);
		
		if (triggerDowntime)
			downtime->SetTriggeredByLegacyId(triggerDowntime->GetLegacyId());
	}

	int legacy_id;

	{
		boost::mutex::scoped_lock lock(l_DowntimeMutex);
		legacy_id = l_NextDowntimeID++;
	}

	downtime->SetLegacyId(legacy_id);

	if (!triggeredBy.IsEmpty()) {
		Checkable::Ptr otherOwner = GetOwnerByDowntimeID(triggeredBy);
		Dictionary::Ptr otherDowntimes = otherOwner->GetDowntimes();
		Downtime::Ptr otherDowntime = otherDowntimes->Get(triggeredBy);
		Dictionary::Ptr triggers = otherDowntime->GetTriggers();

		triggers->Set(triggeredBy, triggeredBy);
	}

	GetDowntimes()->Set(uid, downtime);

	{
		boost::mutex::scoped_lock lock(l_DowntimeMutex);
		l_LegacyDowntimesCache[legacy_id] = uid;
		l_DowntimesCache[uid] = this;
	}

	Log(LogNotice, "Checkable")
	    << "Added downtime with ID '" << downtime->GetLegacyId()
	    << "' between '" << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S", startTime)
	    << "' and '" << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S", endTime) << "'.";

	OnDowntimeAdded(this, downtime, origin);

	return uid;
}

void Checkable::RemoveDowntime(const String& id, bool cancelled, const MessageOrigin& origin)
{
	Checkable::Ptr owner = GetOwnerByDowntimeID(id);

	if (!owner)
		return;

	Dictionary::Ptr downtimes = owner->GetDowntimes();

	Downtime::Ptr downtime = downtimes->Get(id);

	if (!downtime)
		return;

	int legacy_id = downtime->GetLegacyId();

	String config_owner = downtime->GetConfigOwner();

	if (!config_owner.IsEmpty()) {
		Log(LogWarning, "Checkable")
		    << "Cannot remove downtime with ID '" << legacy_id << "'. It is owned by scheduled downtime object '" << config_owner << "'";
		return;
	}

	downtimes->Remove(id);

	{
		boost::mutex::scoped_lock lock(l_DowntimeMutex);
		l_LegacyDowntimesCache.erase(legacy_id);
		l_DowntimesCache.erase(id);
	}

	downtime->SetWasCancelled(cancelled);

	Log(LogNotice, "Checkable")
	    << "Removed downtime with ID '" << downtime->GetLegacyId() << "' from service '" << owner->GetName() << "'.";

	OnDowntimeRemoved(owner, downtime, origin);
}

void Checkable::TriggerDowntimes(void)
{
	Dictionary::Ptr downtimes = GetDowntimes();

	std::vector<String> ids;

	{
		ObjectLock olock(downtimes);

		BOOST_FOREACH(const Dictionary::Pair& kv, downtimes) {
			ids.push_back(kv.first);
		}
	}

	BOOST_FOREACH(const String& id, ids) {
		TriggerDowntime(id);
	}
}

void Checkable::TriggerDowntime(const String& id)
{
	Checkable::Ptr owner = GetOwnerByDowntimeID(id);
	Downtime::Ptr downtime = GetDowntimeByID(id);

	if (!downtime)
		return;

	if (downtime->IsActive() && downtime->IsTriggered()) {
		Log(LogDebug, "Checkable")
		    << "Not triggering downtime with ID '" << downtime->GetLegacyId() << "': already triggered.";
		return;
	}

	if (downtime->IsExpired()) {
		Log(LogDebug, "Checkable")
		    << "Not triggering downtime with ID '" << downtime->GetLegacyId() << "': expired.";
		return;
	}

	Log(LogNotice, "Checkable")
		<< "Triggering downtime with ID '" << downtime->GetLegacyId() << "'.";

	if (downtime->GetTriggerTime() == 0)
		downtime->SetTriggerTime(Utility::GetTime());

	Dictionary::Ptr triggers = downtime->GetTriggers();
	ObjectLock olock(triggers);
	BOOST_FOREACH(const Dictionary::Pair& kv, triggers) {
		TriggerDowntime(kv.first);
	}

	OnDowntimeTriggered(owner, downtime);
}

String Checkable::GetDowntimeIDFromLegacyID(int id)
{
	boost::mutex::scoped_lock lock(l_DowntimeMutex);

	std::map<int, String>::iterator it = l_LegacyDowntimesCache.find(id);

	if (it == l_LegacyDowntimesCache.end())
		return Empty;

	return it->second;
}

Checkable::Ptr Checkable::GetOwnerByDowntimeID(const String& id)
{
	boost::mutex::scoped_lock lock(l_DowntimeMutex);
	return l_DowntimesCache[id];
}

Downtime::Ptr Checkable::GetDowntimeByID(const String& id)
{
	Checkable::Ptr owner = GetOwnerByDowntimeID(id);

	if (!owner)
		return Downtime::Ptr();

	Dictionary::Ptr downtimes = owner->GetDowntimes();

	if (downtimes)
		return downtimes->Get(id);

	return Downtime::Ptr();
}

void Checkable::StartDowntimesExpiredTimer(void)
{
	l_DowntimesExpireTimer = new Timer();
	l_DowntimesExpireTimer->SetInterval(60);
	l_DowntimesExpireTimer->OnTimerExpired.connect(boost::bind(&Checkable::DowntimesExpireTimerHandler));
	l_DowntimesExpireTimer->Start();
}

void Checkable::AddDowntimesToCache(void)
{
#ifdef I2_DEBUG
	Log(LogDebug, "Checkable", "Updating Checkable downtimes cache.");
#endif /* I2_DEBUG */

	Dictionary::Ptr downtimes = GetDowntimes();

	boost::mutex::scoped_lock lock(l_DowntimeMutex);

	ObjectLock olock(downtimes);

	BOOST_FOREACH(const Dictionary::Pair& kv, downtimes) {
		Downtime::Ptr downtime = kv.second;

		int legacy_id = downtime->GetLegacyId();

		if (legacy_id >= l_NextDowntimeID)
			l_NextDowntimeID = legacy_id + 1;

		l_LegacyDowntimesCache[legacy_id] = kv.first;
		l_DowntimesCache[kv.first] = this;
	}
}

void Checkable::RemoveExpiredDowntimes(void)
{
	Dictionary::Ptr downtimes = GetDowntimes();

	std::vector<String> expiredDowntimes;

	{
		ObjectLock olock(downtimes);

		BOOST_FOREACH(const Dictionary::Pair& kv, downtimes) {
			Downtime::Ptr downtime = kv.second;

			if (downtime->IsExpired())
				expiredDowntimes.push_back(kv.first);
		}
	}

	BOOST_FOREACH(const String& id, expiredDowntimes) {
		/* override config owner to clear expired downtimes once */
		Downtime::Ptr downtime = GetDowntimeByID(id);
		downtime->SetConfigOwner(Empty);

		RemoveDowntime(id, false);
	}
}

void Checkable::DowntimesExpireTimerHandler(void)
{
	BOOST_FOREACH(const Host::Ptr& host, DynamicType::GetObjectsByType<Host>()) {
		host->RemoveExpiredDowntimes();
	}

	BOOST_FOREACH(const Service::Ptr& service, DynamicType::GetObjectsByType<Service>()) {
		service->RemoveExpiredDowntimes();
	}
}

bool Checkable::IsInDowntime(void) const
{
	Dictionary::Ptr downtimes = GetDowntimes();

	ObjectLock olock(downtimes);

	BOOST_FOREACH(const Dictionary::Pair& kv, downtimes) {
		Downtime::Ptr downtime = kv.second;

		if (downtime->IsActive())
			return true;
	}

	return false;
}

int Checkable::GetDowntimeDepth(void) const
{
	int downtime_depth = 0;
	Dictionary::Ptr downtimes = GetDowntimes();

	ObjectLock olock(downtimes);

	BOOST_FOREACH(const Dictionary::Pair& kv, downtimes) {
		Downtime::Ptr downtime = kv.second;

		if (downtime->IsActive())
			downtime_depth++;
	}

	return downtime_depth;
}

