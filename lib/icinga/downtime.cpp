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

#include "icinga/downtime.hpp"
#include "icinga/downtime.tcpp"
#include "icinga/host.hpp"
#include "icinga/scheduleddowntime.hpp"
#include "remote/configobjectutility.hpp"
#include "base/configtype.hpp"
#include "base/utility.hpp"
#include "base/timer.hpp"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/thread/once.hpp>

using namespace icinga;

static int l_NextDowntimeID = 1;
static boost::mutex l_DowntimeMutex;
static std::map<int, String> l_LegacyDowntimesCache;
static Timer::Ptr l_DowntimesExpireTimer;
static Timer::Ptr l_DowntimesStartTimer;

boost::signals2::signal<void (const Downtime::Ptr&)> Downtime::OnDowntimeAdded;
boost::signals2::signal<void (const Downtime::Ptr&)> Downtime::OnDowntimeRemoved;
boost::signals2::signal<void (const Downtime::Ptr&)> Downtime::OnDowntimeStarted;
boost::signals2::signal<void (const Downtime::Ptr&)> Downtime::OnDowntimeTriggered;

REGISTER_TYPE(Downtime);

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

void Downtime::OnAllConfigLoaded()
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

	static boost::once_flag once = BOOST_ONCE_INIT;

	boost::call_once(once, [this]() {
		l_DowntimesStartTimer = new Timer();
		l_DowntimesStartTimer->SetInterval(5);
		l_DowntimesStartTimer->OnTimerExpired.connect(std::bind(&Downtime::DowntimesStartTimerHandler));
		l_DowntimesStartTimer->Start();

		l_DowntimesExpireTimer = new Timer();
		l_DowntimesExpireTimer->SetInterval(60);
		l_DowntimesExpireTimer->OnTimerExpired.connect(std::bind(&Downtime::DowntimesExpireTimerHandler));
		l_DowntimesExpireTimer->Start();
	});

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
	if (!checkable->IsStateOK(checkable->GetStateRaw())) {
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

Checkable::Ptr Downtime::GetCheckable() const
{
	return static_pointer_cast<Checkable>(m_Checkable);
}

bool Downtime::IsInEffect() const
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

	return (now < triggerTime + GetDuration());
}

bool Downtime::IsTriggered() const
{
	double now = Utility::GetTime();

	double triggerTime = GetTriggerTime();

	return (triggerTime > 0 && triggerTime <= now);
}

bool Downtime::IsExpired() const
{
	double now = Utility::GetTime();

	if (GetFixed())
		return (GetEndTime() < now);
	else {
		/* triggered flexible downtime not in effect anymore */
		if (IsTriggered() && !IsInEffect())
			return true;
		/* flexible downtime never triggered */
		else if (!IsTriggered() && (GetEndTime() < now))
			return true;
		else
			return false;
	}
}

bool Downtime::HasValidConfigOwner() const
{
	String configOwner = GetConfigOwner();
	return configOwner.IsEmpty() || GetObject<ScheduledDowntime>(configOwner);
}

int Downtime::GetNextDowntimeID()
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
	attrs->Set("entry_time", Utility::GetTime());

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	attrs->Set("host_name", host->GetName());
	if (service)
		attrs->Set("service_name", service->GetShortName());

	String zone = checkable->GetZoneName();

	if (!zone.IsEmpty())
		attrs->Set("zone", zone);

	String config = ConfigObjectUtility::CreateObjectConfig(Downtime::TypeInstance, fullName, true, nullptr, attrs);

	Array::Ptr errors = new Array();

	if (!ConfigObjectUtility::CreateObject(Downtime::TypeInstance, fullName, config, errors)) {
		ObjectLock olock(errors);
		for (const String& error : errors) {
			Log(LogCritical, "Downtime", error);
		}

		BOOST_THROW_EXCEPTION(std::runtime_error("Could not create downtime."));
	}

	if (!triggeredBy.IsEmpty()) {
		Downtime::Ptr parentDowntime = Downtime::GetByName(triggeredBy);
		Array::Ptr triggers = parentDowntime->GetTriggers();

		ObjectLock olock(triggers);
		if (!triggers->Contains(fullName))
			triggers->Add(fullName);
	}

	Downtime::Ptr downtime = Downtime::GetByName(fullName);

	if (!downtime)
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not create downtime object."));

	Log(LogNotice, "Downtime")
		<< "Added downtime '" << downtime->GetName()
		<< "' between '" << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S", startTime)
		<< "' and '" << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S", endTime) << "'.";

	return fullName;
}

void Downtime::RemoveDowntime(const String& id, bool cancelled, bool expired, const MessageOrigin::Ptr& origin)
{
	Downtime::Ptr downtime = Downtime::GetByName(id);

	if (!downtime || downtime->GetPackage() != "_api")
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
		for (const String& error : errors) {
			Log(LogCritical, "Downtime", error);
		}

		BOOST_THROW_EXCEPTION(std::runtime_error("Could not remove downtime."));
	}
}

bool Downtime::CanBeTriggered()
{
	if (IsInEffect() && IsTriggered())
		return false;

	if (IsExpired())
		return false;

	double now = Utility::GetTime();

	if (now < GetStartTime() || now > GetEndTime())
		return false;

	return true;
}

void Downtime::TriggerDowntime()
{
	if (!CanBeTriggered())
		return;

	Log(LogNotice, "Downtime")
		<< "Triggering downtime '" << GetName() << "'.";

	if (GetTriggerTime() == 0)
		SetTriggerTime(Utility::GetTime());

	Array::Ptr triggers = GetTriggers();

	{
		ObjectLock olock(triggers);
		for (const String& triggerName : triggers) {
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

	auto it = l_LegacyDowntimesCache.find(id);

	if (it == l_LegacyDowntimesCache.end())
		return Empty;

	return it->second;
}

void Downtime::DowntimesStartTimerHandler()
{
	/* Start fixed downtimes. Flexible downtimes will be triggered on-demand. */
	for (const Downtime::Ptr& downtime : ConfigType::GetObjectsByType<Downtime>()) {
		if (downtime->IsActive() &&
			downtime->CanBeTriggered() &&
			downtime->GetFixed()) {
			/* Send notifications. */
			OnDowntimeStarted(downtime);

			/* Trigger fixed downtime immediately. */
			downtime->TriggerDowntime();
		}
	}
}

void Downtime::DowntimesExpireTimerHandler()
{
	std::vector<Downtime::Ptr> downtimes;

	for (const Downtime::Ptr& downtime : ConfigType::GetObjectsByType<Downtime>()) {
		downtimes.push_back(downtime);
	}

	for (const Downtime::Ptr& downtime : downtimes) {
		/* Only remove downtimes which are activated after daemon start. */
		if (downtime->IsActive() && (downtime->IsExpired() || !downtime->HasValidConfigOwner()))
			RemoveDowntime(downtime->GetName(), false, true);
	}
}

void Downtime::ValidateStartTime(const Timestamp& value, const ValidationUtils& utils)
{
	ObjectImpl<Downtime>::ValidateStartTime(value, utils);

	if (value <= 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "start_time" }, "Start time must be greater than 0."));
}

void Downtime::ValidateEndTime(const Timestamp& value, const ValidationUtils& utils)
{
	ObjectImpl<Downtime>::ValidateEndTime(value, utils);

	if (value <= 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "end_time" }, "End time must be greater than 0."));
}
