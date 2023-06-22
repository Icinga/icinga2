/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/downtime.hpp"
#include "icinga/downtime-ti.cpp"
#include "icinga/host.hpp"
#include "icinga/scheduleddowntime.hpp"
#include "remote/configobjectutility.hpp"
#include "base/configtype.hpp"
#include "base/utility.hpp"
#include "base/timer.hpp"
#include <boost/thread/once.hpp>
#include <cmath>
#include <utility>

using namespace icinga;

static int l_NextDowntimeID = 1;
static std::mutex l_DowntimeMutex;
static std::map<int, String> l_LegacyDowntimesCache;
static Timer::Ptr l_DowntimesOrphanedTimer;
static Timer::Ptr l_DowntimesStartTimer;

boost::signals2::signal<void (const Downtime::Ptr&)> Downtime::OnDowntimeAdded;
boost::signals2::signal<void (const Downtime::Ptr&)> Downtime::OnDowntimeRemoved;
boost::signals2::signal<void (const Downtime::Ptr&)> Downtime::OnDowntimeStarted;
boost::signals2::signal<void (const Downtime::Ptr&)> Downtime::OnDowntimeTriggered;
boost::signals2::signal<void (const Downtime::Ptr&, const String&, double, const MessageOrigin::Ptr&)> Downtime::OnRemovalInfoChanged;

REGISTER_TYPE(Downtime);

INITIALIZE_ONCE(&Downtime::StaticInitialize);

void Downtime::StaticInitialize()
{
	ScriptGlobal::Set("Icinga.DowntimeNoChildren", "DowntimeNoChildren");
	ScriptGlobal::Set("Icinga.DowntimeTriggeredChildren", "DowntimeTriggeredChildren");
	ScriptGlobal::Set("Icinga.DowntimeNonTriggeredChildren", "DowntimeNonTriggeredChildren");
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
	std::vector<String> tokens = name.Split("!");

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

	if (GetServiceName().IsEmpty())
		m_Checkable = Host::GetByName(GetHostName());
	else
		m_Checkable = Service::GetByNamePair(GetHostName(), GetServiceName());

	if (!m_Checkable)
		BOOST_THROW_EXCEPTION(ScriptError("Downtime '" + GetName() + "' references a host/service which doesn't exist.", GetDebugInfo()));
}

void Downtime::Start(bool runtimeCreated)
{
	ObjectImpl<Downtime>::Start(runtimeCreated);

	static boost::once_flag once = BOOST_ONCE_INIT;

	boost::call_once(once, [this]() {
		l_DowntimesStartTimer = Timer::Create();
		l_DowntimesStartTimer->SetInterval(5);
		l_DowntimesStartTimer->OnTimerExpired.connect([](const Timer * const&){ DowntimesStartTimerHandler(); });
		l_DowntimesStartTimer->Start();

		l_DowntimesOrphanedTimer = Timer::Create();
		l_DowntimesOrphanedTimer->SetInterval(60);
		l_DowntimesOrphanedTimer->OnTimerExpired.connect([](const Timer * const&) { DowntimesOrphanedTimerHandler(); });
		l_DowntimesOrphanedTimer->Start();
	});

	{
		std::unique_lock<std::mutex> lock(l_DowntimeMutex);

		SetLegacyId(l_NextDowntimeID);
		l_LegacyDowntimesCache[l_NextDowntimeID] = GetName();
		l_NextDowntimeID++;
	}

	Checkable::Ptr checkable = GetCheckable();

	checkable->RegisterDowntime(this);

	Downtime::Ptr parent = GetByName(GetParent());

	if (parent)
		parent->RegisterChild(this);

	if (runtimeCreated)
		OnDowntimeAdded(this);

	/* if this object is already in a NOT-OK state trigger
	 * this downtime now *after* it has been added (important
	 * for DB IDO, etc.)
	 */
	if (!GetFixed() && !checkable->IsStateOK(checkable->GetStateRaw())) {
		Log(LogNotice, "Downtime")
			<< "Checkable '" << checkable->GetName() << "' already in a NOT-OK state."
			<< " Triggering downtime now.";

		TriggerDowntime(std::fmax(std::fmax(GetStartTime(), GetEntryTime()), checkable->GetLastStateChange()));
	}

	if (GetFixed() && CanBeTriggered()) {
		/* Send notifications. */
		OnDowntimeStarted(this);

		/* Trigger fixed downtime immediately. */
		TriggerDowntime(std::fmax(GetStartTime(), GetEntryTime()));
	}
}

void Downtime::Stop(bool runtimeRemoved)
{
	GetCheckable()->UnregisterDowntime(this);

	Downtime::Ptr parent = GetByName(GetParent());

	if (parent)
		parent->UnregisterChild(this);

	if (runtimeRemoved)
		OnDowntimeRemoved(this);

	ObjectImpl<Downtime>::Stop(runtimeRemoved);
}

void Downtime::Pause()
{
	if (m_CleanupTimer) {
		m_CleanupTimer->Stop();
	}

	ObjectImpl<Downtime>::Pause();
}

void Downtime::Resume()
{
	ObjectImpl<Downtime>::Resume();
	SetupCleanupTimer();
}

Checkable::Ptr Downtime::GetCheckable() const
{
	return static_pointer_cast<Checkable>(m_Checkable);
}

bool Downtime::IsInEffect() const
{
	double now = Utility::GetTime();

	if (GetFixed()) {
		/* fixed downtimes are in effect during the entire [start..end) interval */
		return (now >= GetStartTime() && now < GetEndTime());
	}

	double triggerTime = GetTriggerTime();

	if (triggerTime == 0)
		/* flexible downtime has not been triggered yet */
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
	if (!ScheduledDowntime::AllConfigIsLoaded()) {
		return true;
	}

	String configOwner = GetConfigOwner();
	return configOwner.IsEmpty() || Zone::GetByName(GetAuthoritativeZone()) != Zone::GetLocalZone() || GetObject<ScheduledDowntime>(configOwner);
}

int Downtime::GetNextDowntimeID()
{
	std::unique_lock<std::mutex> lock(l_DowntimeMutex);

	return l_NextDowntimeID;
}

Downtime::Ptr Downtime::AddDowntime(const Checkable::Ptr& checkable, const String& author,
	const String& comment, double startTime, double endTime, bool fixed,
	const String& triggeredBy, double duration,
	const String& scheduledDowntime, const String& scheduledBy, const String& parent,
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
	attrs->Set("parent", parent);
	attrs->Set("config_owner", scheduledDowntime);
	attrs->Set("entry_time", Utility::GetTime());

	if (!scheduledDowntime.IsEmpty()) {
		auto localZone (Zone::GetLocalZone());

		if (localZone) {
			attrs->Set("authoritative_zone", localZone->GetName());
		}

		auto sd (ScheduledDowntime::GetByName(scheduledDowntime));

		if (sd) {
			attrs->Set("config_owner_hash", sd->HashDowntimeOptions());
		}
	}

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	attrs->Set("host_name", host->GetName());
	if (service)
		attrs->Set("service_name", service->GetShortName());

	String zone;

	if (!scheduledDowntime.IsEmpty()) {
		auto sdt (ScheduledDowntime::GetByName(scheduledDowntime));

		if (sdt) {
			auto sdtZone (sdt->GetZone());

			if (sdtZone) {
				zone = sdtZone->GetName();
			}
		}
	}

	if (zone.IsEmpty()) {
		zone = checkable->GetZoneName();
	}

	if (!zone.IsEmpty())
		attrs->Set("zone", zone);

	String config = ConfigObjectUtility::CreateObjectConfig(Downtime::TypeInstance, fullName, true, nullptr, attrs);

	Array::Ptr errors = new Array();

	if (!ConfigObjectUtility::CreateObject(Downtime::TypeInstance, fullName, config, errors, nullptr)) {
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

	Log(LogInformation, "Downtime")
		<< "Added downtime '" << downtime->GetName()
		<< "' between '" << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S", startTime)
		<< "' and '" << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S", endTime) << "', author: '"
		<< author << "', " << (fixed ? "fixed" : "flexible with " + Convert::ToString(duration) + "s duration");

	return downtime;
}

void Downtime::RemoveDowntime(const String& id, bool includeChildren, bool cancelled, bool expired,
	const String& removedBy, const MessageOrigin::Ptr& origin)
{
	Downtime::Ptr downtime = Downtime::GetByName(id);

	if (!downtime || downtime->GetPackage() != "_api")
		return;

	String config_owner = downtime->GetConfigOwner();

	if (!config_owner.IsEmpty() && !expired) {
		BOOST_THROW_EXCEPTION(invalid_downtime_removal_error("Cannot remove downtime '" + downtime->GetName() +
			"'. It is owned by scheduled downtime object '" + config_owner + "'"));
	}

	if (includeChildren) {
		for (const Downtime::Ptr& child : downtime->GetChildren()) {
			Downtime::RemoveDowntime(child->GetName(), true, true);
		}
	}

	if (cancelled) {
		downtime->SetRemovalInfo(removedBy, Utility::GetTime());
	}

	Array::Ptr errors = new Array();

	if (!ConfigObjectUtility::DeleteObject(downtime, false, errors, nullptr)) {
		ObjectLock olock(errors);
		for (const String& error : errors) {
			Log(LogCritical, "Downtime", error);
		}

		BOOST_THROW_EXCEPTION(std::runtime_error("Could not remove downtime."));
	}

	String reason;

	if (expired) {
		reason = "expired at " + Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", downtime->GetEndTime());
	} else if (cancelled) {
		reason = "cancelled by user";
	} else {
		reason = "<unknown>";
	}

	Log msg (LogInformation, "Downtime");

	msg << "Removed downtime '" << downtime->GetName() << "' from checkable";

	{
		auto checkable (downtime->GetCheckable());

		if (checkable) {
			msg << " '" << checkable->GetName() << "'";
		}
	}

	msg << " (Reason: " << reason << ").";
}

void Downtime::RegisterChild(const Downtime::Ptr& downtime)
{
	std::unique_lock<std::mutex> lock(m_ChildrenMutex);
	m_Children.insert(downtime);
}

void Downtime::UnregisterChild(const Downtime::Ptr& downtime)
{
	std::unique_lock<std::mutex> lock(m_ChildrenMutex);
	m_Children.erase(downtime);
}

std::set<Downtime::Ptr> Downtime::GetChildren() const
{
	std::unique_lock<std::mutex> lock(m_ChildrenMutex);
	return m_Children;
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

void Downtime::SetupCleanupTimer()
{
	if (!m_CleanupTimer) {
		m_CleanupTimer = Timer::Create();

		auto name (GetName());

		m_CleanupTimer->OnTimerExpired.connect([name=std::move(name)](const Timer * const&) {
			auto downtime (Downtime::GetByName(name));

			if (downtime && downtime->IsExpired()) {
				RemoveDowntime(name, false, false, true);
			}
		});
	}

	auto triggerTime (GetTriggerTime());

	m_CleanupTimer->Reschedule((GetFixed() || triggerTime <= 0 ? GetEndTime() : triggerTime + GetDuration()) + 0.1);
	m_CleanupTimer->Start();
}

void Downtime::TriggerDowntime(double triggerTime)
{
	if (!CanBeTriggered())
		return;

	Checkable::Ptr checkable = GetCheckable();

	Log(LogInformation, "Downtime")
		<< "Triggering downtime '" << GetName() << "' for checkable '" << checkable->GetName() << "'.";

	if (GetTriggerTime() == 0) {
		SetTriggerTime(triggerTime);
	}

	{
		ObjectLock olock (this);
		SetupCleanupTimer();
	}

	Array::Ptr triggers = GetTriggers();

	{
		ObjectLock olock(triggers);
		for (const String& triggerName : triggers) {
			Downtime::Ptr downtime = Downtime::GetByName(triggerName);

			if (!downtime)
				continue;

			downtime->TriggerDowntime(triggerTime);
		}
	}

	OnDowntimeTriggered(this);
}

void Downtime::SetRemovalInfo(const String& removedBy, double removeTime, const MessageOrigin::Ptr& origin) {
	{
		ObjectLock olock(this);

		SetRemovedBy(removedBy, false, origin);
		SetRemoveTime(removeTime, false, origin);
	}

	OnRemovalInfoChanged(this, removedBy, removeTime, origin);
}

String Downtime::GetDowntimeIDFromLegacyID(int id)
{
	std::unique_lock<std::mutex> lock(l_DowntimeMutex);

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
			downtime->TriggerDowntime(std::fmax(downtime->GetStartTime(), downtime->GetEntryTime()));
		}
	}
}

void Downtime::DowntimesOrphanedTimerHandler()
{
	for (const Downtime::Ptr& downtime : ConfigType::GetObjectsByType<Downtime>()) {
		/* Only remove downtimes which are activated after daemon start. */
		if (downtime->IsActive() && !downtime->HasValidConfigOwner())
			RemoveDowntime(downtime->GetName(), false, false, true);
	}
}

void Downtime::ValidateStartTime(const Lazy<Timestamp>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<Downtime>::ValidateStartTime(lvalue, utils);

	if (lvalue() <= 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "start_time" }, "Start time must be greater than 0."));
}

void Downtime::ValidateEndTime(const Lazy<Timestamp>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<Downtime>::ValidateEndTime(lvalue, utils);

	if (lvalue() <= 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "end_time" }, "End time must be greater than 0."));
}

DowntimeChildOptions Downtime::ChildOptionsFromValue(const Value& options)
{
	if (options == "DowntimeNoChildren")
		return DowntimeNoChildren;
	else if (options == "DowntimeTriggeredChildren")
		return DowntimeTriggeredChildren;
	else if (options == "DowntimeNonTriggeredChildren")
		return DowntimeNonTriggeredChildren;
	else if (options.IsNumber()) {
		int number = options;
		if (number >= 0 && number <= 2)
			return static_cast<DowntimeChildOptions>(number);
	}

	BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid child option specified"));
}
