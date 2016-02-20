/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "icinga/downtime.hpp"
#include "icinga/downtime.tcpp"
#include "icinga/host.hpp"
#include "remote/configobjectutility.hpp"
#include "base/configtype.hpp"
#include "base/utility.hpp"
#include "base/timer.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace icinga;

static int l_NextDowntimeID = 1;
static boost::mutex l_DowntimeMutex;
static std::map<int, String> l_LegacyDowntimesCache;
static Timer::Ptr l_DowntimesExpireTimer;

boost::signals2::signal<void (const Downtime::Ptr&)> Downtime::OnDowntimeAdded;
boost::signals2::signal<void (const Downtime::Ptr&)> Downtime::OnDowntimeRemoved;
boost::signals2::signal<void (const Downtime::Ptr&)> Downtime::OnDowntimeTriggered;

INITIALIZE_ONCE(&Downtime::StaticInitialize);

REGISTER_TYPE(Downtime);

void Downtime::StaticInitialize(void)
{
	l_DowntimesExpireTimer = new Timer();
	l_DowntimesExpireTimer->SetInterval(60);
	l_DowntimesExpireTimer->OnTimerExpired.connect(boost::bind(&Downtime::DowntimesExpireTimerHandler));
	l_DowntimesExpireTimer->Start();
}

String DowntimeNameComposer::MakeName(const String& shortName, const Object::Ptr& context) const
{
	Downtime::Ptr downtime = dynamic_pointer_cast<Downtime>(context);

	if (!downtime)
		return "";

	String name = downtime->GetHostName();

	if (!downtime->GetServiceName().IsEmpty())
		name += "!" + downtime->GetServiceName();

	name += "!" + shortName;

	return name;
}

Dictionary::Ptr DowntimeNameComposer::ParseName(const String& name) const
{
	std::vector<String> tokens;
	boost::algorithm::split(tokens, name, boost::is_any_of("!"));

	if (tokens.size() < 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid Downtime name."));

	Dictionary::Ptr result = new Dictionary();
	result->Set("host_name", tokens[0]);

	if (tokens.size() > 2) {
		result->Set("service_name", tokens[1]);
		result->Set("name", tokens[2]);
	} else {
		result->Set("name", tokens[1]);
	}

	return result;
}

void Downtime::OnAllConfigLoaded(void)
{
	ObjectImpl<Downtime>::OnAllConfigLoaded();

	Host::Ptr host = Host::GetByName(GetHostName());

	if (GetServiceName().IsEmpty())
		m_Checkable = host;
	else
		m_Checkable = host->GetServiceByShortName(GetServiceName());

	if (!m_Checkable)
		BOOST_THROW_EXCEPTION(ScriptError("Downtime '" + GetName() + "' references a host/service which doesn't exist.", GetDebugInfo()));
}

void Downtime::Start(bool runtimeCreated)
{
	ObjectImpl<Downtime>::Start(runtimeCreated);

	{
		boost::mutex::scoped_lock lock(l_DowntimeMutex);

		SetLegacyId(l_NextDowntimeID);
		l_LegacyDowntimesCache[l_NextDowntimeID] = GetName();
		l_NextDowntimeID++;
	}

	Checkable::Ptr checkable = GetCheckable();

	checkable->RegisterDowntime(this);

	if (runtimeCreated)
		OnDowntimeAdded(this);

	/* if this object is already in a NOT-OK state trigger
	 * this downtime now *after* it has been added (important
	 * for DB IDO, etc.)
	 */
	if (checkable->GetStateRaw() != ServiceOK) {
		Log(LogNotice, "Downtime")
		    << "Checkable '" << checkable->GetName() << "' already in a NOT-OK state."
		    << " Triggering downtime now.";
		TriggerDowntime();
	}
}

void Downtime::Stop(bool runtimeRemoved)
{
	GetCheckable()->UnregisterDowntime(this);

	if (runtimeRemoved)
		OnDowntimeRemoved(this);

	ObjectImpl<Downtime>::Stop(runtimeRemoved);
}

Checkable::Ptr Downtime::GetCheckable(void) const
{
	return static_pointer_cast<Checkable>(m_Checkable);
}

bool Downtime::IsActive(void) const
{
	double now = Utility::GetTime();

	if (now < GetStartTime() ||
		now > GetEndTime())
		return false;

	if (GetFixed())
		return true;

	double triggerTime = GetTriggerTime();

	if (triggerTime == 0)
		return false;

	return (triggerTime + GetDuration() < now);
}

bool Downtime::IsTriggered(void) const
{
	double now = Utility::GetTime();

	double triggerTime = GetTriggerTime();

	return (triggerTime > 0 && triggerTime <= now);
}

bool Downtime::IsExpired(void) const
{
	return (GetEndTime() < Utility::GetTime());
}

int Downtime::GetNextDowntimeID(void)
{
	boost::mutex::scoped_lock lock(l_DowntimeMutex);

	return l_NextDowntimeID;
}

String Downtime::AddDowntime(const Checkable::Ptr& checkable, const String& author,
    const String& comment, double startTime, double endTime, bool fixed,
    const String& triggeredBy, double duration,
    const String& scheduledDowntime, const String& scheduledBy,
    const String& id, const MessageOrigin::Ptr& origin)
{
	String fullName;

	if (id.IsEmpty())
		fullName = checkable->GetName() + "!" + Utility::NewUniqueID();
	else
		fullName = id;

	Dictionary::Ptr attrs = new Dictionary();

	attrs->Set("author", author);
	attrs->Set("comment", comment);
	attrs->Set("start_time", startTime);
	attrs->Set("end_time", endTime);
	attrs->Set("fixed", fixed);
	attrs->Set("duration", duration);
	attrs->Set("triggered_by", triggeredBy);
	attrs->Set("scheduled_by", scheduledBy);
	attrs->Set("config_owner", scheduledDowntime);

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	attrs->Set("host_name", host->GetName());
	if (service)
		attrs->Set("service_name", service->GetShortName());

	String config = ConfigObjectUtility::CreateObjectConfig(Downtime::TypeInstance, fullName, true, Array::Ptr(), attrs);

	Array::Ptr errors = new Array();

	if (!ConfigObjectUtility::CreateObject(Downtime::TypeInstance, fullName, config, errors)) {
		ObjectLock olock(errors);
		BOOST_FOREACH(const String& error, errors) {
			Log(LogCritical, "Downtime", error);
		}

		BOOST_THROW_EXCEPTION(std::runtime_error("Could not create downtime."));
	}

	if (!triggeredBy.IsEmpty()) {
		Downtime::Ptr triggerDowntime = Downtime::GetByName(triggeredBy);
		Array::Ptr triggers = triggerDowntime->GetTriggers();

		if (!triggers->Contains(triggeredBy))
			triggers->Add(triggeredBy);
	}

	Downtime::Ptr downtime = Downtime::GetByName(fullName);

	Log(LogNotice, "Downtime")
	    << "Added downtime '" << downtime->GetName()
	    << "' between '" << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S", startTime)
	    << "' and '" << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S", endTime) << "'.";


	return fullName;
}

void Downtime::RemoveDowntime(const String& id, bool cancelled, bool expired, const MessageOrigin::Ptr& origin)
{
	Downtime::Ptr downtime = Downtime::GetByName(id);

	if (!downtime)
		return;

	String config_owner = downtime->GetConfigOwner();

	if (!config_owner.IsEmpty() && !expired) {
		Log(LogWarning, "Downtime")
		    << "Cannot remove downtime '" << downtime->GetName() << "'. It is owned by scheduled downtime object '" << config_owner << "'";
		return;
	}

	downtime->SetWasCancelled(cancelled);

	Log(LogNotice, "Downtime")
	    << "Removed downtime '" << downtime->GetName() << "' from object '" << downtime->GetCheckable()->GetName() << "'.";

	Array::Ptr errors = new Array();

	if (!ConfigObjectUtility::DeleteObject(downtime, false, errors)) {
		ObjectLock olock(errors);
		BOOST_FOREACH(const String& error, errors) {
			Log(LogCritical, "Downtime", error);
		}

		BOOST_THROW_EXCEPTION(std::runtime_error("Could not remove downtime."));
	}
}

void Downtime::TriggerDowntime(void)
{
	if (IsActive() && IsTriggered()) {
		Log(LogDebug, "Downtime")
		    << "Not triggering downtime '" << GetName() << "': already triggered.";
		return;
	}

	if (IsExpired()) {
		Log(LogDebug, "Downtime")
		    << "Not triggering downtime '" << GetName() << "': expired.";
		return;
	}

	double now = Utility::GetTime();

	if (now < GetStartTime() || now > GetEndTime()) {
		Log(LogDebug, "Downtime")
		    << "Not triggering downtime '" << GetName() << "': current time is outside downtime window.";
		return;
	}

	Log(LogNotice, "Downtime")
		<< "Triggering downtime '" << GetName() << "'.";

	if (GetTriggerTime() == 0)
		SetTriggerTime(Utility::GetTime());

	Array::Ptr triggers = GetTriggers();

	{
		ObjectLock olock(triggers);
		BOOST_FOREACH(const String& triggerName, triggers) {
			Downtime::Ptr downtime = Downtime::GetByName(triggerName);

			if (!downtime)
				continue;

			downtime->TriggerDowntime();
		}
	}

	OnDowntimeTriggered(this);
}

String Downtime::GetDowntimeIDFromLegacyID(int id)
{
	boost::mutex::scoped_lock lock(l_DowntimeMutex);

	std::map<int, String>::iterator it = l_LegacyDowntimesCache.find(id);

	if (it == l_LegacyDowntimesCache.end())
		return Empty;

	return it->second;
}

void Downtime::DowntimesExpireTimerHandler(void)
{
	std::vector<Downtime::Ptr> downtimes;

	BOOST_FOREACH(const Downtime::Ptr& downtime, ConfigType::GetObjectsByType<Downtime>()) {
		downtimes.push_back(downtime);
	}

	BOOST_FOREACH(const Downtime::Ptr& downtime, downtimes) {
		if (downtime->IsExpired())
			RemoveDowntime(downtime->GetName(), false, true);
	}
}

void Downtime::ValidateStartTime(double value, const ValidationUtils& utils)
{
	ObjectImpl<Downtime>::ValidateStartTime(value, utils);

	if (value <= 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of("start_time"), "Start time must be greater than 0."));
}

void Downtime::ValidateEndTime(double value, const ValidationUtils& utils)
{
	ObjectImpl<Downtime>::ValidateEndTime(value, utils);

	if (value <= 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of("end_time"), "End time must be greater than 0."));
}
