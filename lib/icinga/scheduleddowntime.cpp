/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/scheduleddowntime.hpp"
#include "icinga/scheduleddowntime-ti.cpp"
#include "icinga/legacytimeperiod.hpp"
#include "icinga/downtime.hpp"
#include "icinga/service.hpp"
#include "base/timer.hpp"
#include "base/configtype.hpp"
#include "base/utility.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include <boost/thread/once.hpp>

using namespace icinga;

REGISTER_TYPE(ScheduledDowntime);

static Timer::Ptr l_Timer;

String ScheduledDowntimeNameComposer::MakeName(const String& shortName, const Object::Ptr& context) const
{
	ScheduledDowntime::Ptr downtime = dynamic_pointer_cast<ScheduledDowntime>(context);

	if (!downtime)
		return "";

	String name = downtime->GetHostName();

	if (!downtime->GetServiceName().IsEmpty())
		name += "!" + downtime->GetServiceName();

	name += "!" + shortName;

	return name;
}

Dictionary::Ptr ScheduledDowntimeNameComposer::ParseName(const String& name) const
{
	std::vector<String> tokens = name.Split("!");

	if (tokens.size() < 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid ScheduledDowntime name."));

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

void ScheduledDowntime::OnAllConfigLoaded()
{
	ObjectImpl<ScheduledDowntime>::OnAllConfigLoaded();

	if (!GetCheckable())
		BOOST_THROW_EXCEPTION(ScriptError("ScheduledDowntime '" + GetName() + "' references a host/service which doesn't exist.", GetDebugInfo()));

	m_AllConfigLoaded.store(true);
}

void ScheduledDowntime::Start(bool runtimeCreated)
{
	ObjectImpl<ScheduledDowntime>::Start(runtimeCreated);

	static boost::once_flag once = BOOST_ONCE_INIT;

	boost::call_once(once, [this]() {
		l_Timer = new Timer();
		l_Timer->SetInterval(60);
		l_Timer->OnTimerExpired.connect([](const Timer * const&) { TimerProc(); });
		l_Timer->Start();
	});

	if (!IsPaused())
		Utility::QueueAsyncCallback([this]() { CreateNextDowntime(); });
}

void ScheduledDowntime::TimerProc()
{
	for (const ScheduledDowntime::Ptr& sd : ConfigType::GetObjectsByType<ScheduledDowntime>()) {
		if (sd->IsActive() && !sd->IsPaused())
			sd->CreateNextDowntime();
	}
}

Checkable::Ptr ScheduledDowntime::GetCheckable() const
{
	Host::Ptr host = Host::GetByName(GetHostName());

	if (GetServiceName().IsEmpty())
		return host;
	else
		return host->GetServiceByShortName(GetServiceName());
}

std::pair<double, double> ScheduledDowntime::FindRunningSegment(double minEnd)
{
	time_t refts = Utility::GetTime();
	tm reference = Utility::LocalTime(refts);

	Log(LogDebug, "ScheduledDowntime")
	    << "Finding running scheduled downtime segment for time " << refts
	    << " (minEnd " << (minEnd > 0 ? Utility::FormatDateTime("%c", minEnd) : "-") << ")";

	Dictionary::Ptr ranges = GetRanges();

	if (!ranges)
		return std::make_pair(0, 0);

	Array::Ptr segments = new Array();

	Dictionary::Ptr bestSegment;
	double bestBegin = 0.0, bestEnd = 0.0;
	double now = Utility::GetTime();

	ObjectLock olock(ranges);

	/* Find the longest lasting (and longer than minEnd, if given) segment that's already running */  
	for (const Dictionary::Pair& kv : ranges) {
		Log(LogDebug, "ScheduledDowntime")
		    << "Evaluating (running?) segment: " << kv.first << ": " << kv.second;

		Dictionary::Ptr segment = LegacyTimePeriod::FindRunningSegment(kv.first, kv.second, &reference);

		if (!segment)
			continue;

		double begin = segment->Get("begin");
		double end = segment->Get("end");

		Log(LogDebug, "ScheduledDowntime")
		    << "Considering (running?) segment: " << Utility::FormatDateTime("%c", begin) << " -> " << Utility::FormatDateTime("%c", end);

		if (begin >= now || end < now) {
			Log(LogDebug, "ScheduledDowntime") << "not running.";
			continue;
		}
		if (minEnd && end <= minEnd) {
			Log(LogDebug, "ScheduledDowntime") << "ending too early.";
			continue;
		}

		if (!bestSegment || end > bestEnd) {
			Log(LogDebug, "ScheduledDowntime") << "(best match yet)";
			bestSegment = segment;
			bestBegin = begin;
			bestEnd = end;
		}
	}

	if (bestSegment)
		return std::make_pair(bestBegin, bestEnd);

	return std::make_pair(0, 0);
}

std::pair<double, double> ScheduledDowntime::FindNextSegment()
{
	time_t refts = Utility::GetTime();
	tm reference = Utility::LocalTime(refts);

	Log(LogDebug, "ScheduledDowntime")
		<< "Finding next scheduled downtime segment for time " << refts;

	Dictionary::Ptr ranges = GetRanges();

	if (!ranges)
		return std::make_pair(0, 0);

	Array::Ptr segments = new Array();

	Dictionary::Ptr bestSegment;
	double bestBegin = 0.0, bestEnd = 0.0;
	double now = Utility::GetTime();

	ObjectLock olock(ranges);

	/* Find the segment starting earliest */
	for (const Dictionary::Pair& kv : ranges) {
		Log(LogDebug, "ScheduledDowntime")
			<< "Evaluating segment: " << kv.first << ": " << kv.second;

		Dictionary::Ptr segment = LegacyTimePeriod::FindNextSegment(kv.first, kv.second, &reference);

		if (!segment)
			continue;

		double begin = segment->Get("begin");
		double end = segment->Get("end");

		Log(LogDebug, "ScheduledDowntime")
			<< "Considering segment: " << Utility::FormatDateTime("%c", begin) << " -> " << Utility::FormatDateTime("%c", end);

		if (begin < now) {
			Log(LogDebug, "ScheduledDowntime") << "already running.";
			continue;
		}

		if (!bestSegment || begin < bestBegin) {
			Log(LogDebug, "ScheduledDowntime") << "(best match yet)";
			bestSegment = segment;
			bestBegin = begin;
			bestEnd = end;
		}
	}

	if (bestSegment)
		return std::make_pair(bestBegin, bestEnd);

	return std::make_pair(0, 0);
}

void ScheduledDowntime::CreateNextDowntime()
{
	/* HA enabled zones. */
	if (IsActive() && IsPaused()) {
		Log(LogNotice, "Checkable")
			<< "Skipping downtime creation for HA-paused Scheduled Downtime object '" << GetName() << "'";
		return;
	}

	double minEnd = 0;

	for (const Downtime::Ptr& downtime : GetCheckable()->GetDowntimes()) {
		double end = downtime->GetEndTime();
		if (end > minEnd)
			minEnd = end;

		if (downtime->GetScheduledBy() != GetName() ||
			downtime->GetStartTime() < Utility::GetTime())
			continue;

		/* We've found a downtime that is owned by us and that hasn't started yet - we're done. */
		return;
	}

	Log(LogDebug, "ScheduledDowntime")
		<< "Creating new Downtime for ScheduledDowntime \"" << GetName() << "\"";

	std::pair<double, double> segment = FindRunningSegment(minEnd);
	if (segment.first == 0 && segment.second == 0) {
		segment = FindNextSegment();
		if (segment.first == 0 && segment.second == 0)
			return;
	}

	String downtimeName = Downtime::AddDowntime(GetCheckable(), GetAuthor(), GetComment(),
		segment.first, segment.second,
		GetFixed(), String(), GetDuration(), GetName(), GetName());

	Downtime::Ptr downtime = Downtime::GetByName(downtimeName);

	int childOptions = Downtime::ChildOptionsFromValue(GetChildOptions());
	if (childOptions > 0) {
		/* 'DowntimeTriggeredChildren' schedules child downtimes triggered by the parent downtime.
		 * 'DowntimeNonTriggeredChildren' schedules non-triggered downtimes for all children.
		 */
		String triggerName;
		if (childOptions == 1)
			triggerName = downtimeName;

		Log(LogNotice, "ScheduledDowntime")
				<< "Processing child options " << childOptions << " for downtime " << downtimeName;

		for (const Checkable::Ptr& child : GetCheckable()->GetAllChildren()) {
			Log(LogNotice, "ScheduledDowntime")
				<< "Scheduling downtime for child object " << child->GetName();

			String childDowntimeName = Downtime::AddDowntime(child, GetAuthor(), GetComment(),
				segment.first, segment.second, GetFixed(), triggerName, GetDuration(), GetName(), GetName());

			Log(LogNotice, "ScheduledDowntime")
				<< "Add child downtime '" << childDowntimeName << "'.";
		}
	}
}

void ScheduledDowntime::ValidateRanges(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<ScheduledDowntime>::ValidateRanges(lvalue, utils);

	if (!lvalue())
		return;

	/* create a fake time environment to validate the definitions */
	time_t refts = Utility::GetTime();
	tm reference = Utility::LocalTime(refts);
	Array::Ptr segments = new Array();

	ObjectLock olock(lvalue());
	for (const Dictionary::Pair& kv : lvalue()) {
		try {
			tm begin_tm, end_tm;
			int stride;
			LegacyTimePeriod::ParseTimeRange(kv.first, &begin_tm, &end_tm, &stride, &reference);
		} catch (const std::exception& ex) {
			BOOST_THROW_EXCEPTION(ValidationError(this, { "ranges" }, "Invalid time specification '" + kv.first + "': " + ex.what()));
		}

		try {
			LegacyTimePeriod::ProcessTimeRanges(kv.second, &reference, segments);
		} catch (const std::exception& ex) {
			BOOST_THROW_EXCEPTION(ValidationError(this, { "ranges" }, "Invalid time range definition '" + kv.second + "': " + ex.what()));
		}
	}
}

void ScheduledDowntime::ValidateChildOptions(const Lazy<Value>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<ScheduledDowntime>::ValidateChildOptions(lvalue, utils);

	try {
		Downtime::ChildOptionsFromValue(lvalue());
	} catch (const std::exception&) {
		BOOST_THROW_EXCEPTION(ValidationError(this, { "child_options" }, "Invalid child_options specified"));
	}
}

bool ScheduledDowntime::AllConfigIsLoaded()
{
	return m_AllConfigLoaded.load();
}

std::atomic<bool> ScheduledDowntime::m_AllConfigLoaded (false);
