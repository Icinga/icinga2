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
}

void ScheduledDowntime::Start(bool runtimeCreated)
{
	ObjectImpl<ScheduledDowntime>::Start(runtimeCreated);

	static boost::once_flag once = BOOST_ONCE_INIT;

	boost::call_once(once, [this]() {
		l_Timer = new Timer();
		l_Timer->SetInterval(60);
		l_Timer->OnTimerExpired.connect(std::bind(&ScheduledDowntime::TimerProc));
		l_Timer->Start();
	});

	Utility::QueueAsyncCallback(std::bind(&ScheduledDowntime::CreateNextDowntime, this));
}

void ScheduledDowntime::TimerProc()
{
	for (const ScheduledDowntime::Ptr& sd : ConfigType::GetObjectsByType<ScheduledDowntime>()) {
		if (sd->IsActive())
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
	double bestBegin, bestEnd;
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

std::pair<double, double> ScheduledDowntime::FindNextSegment(double minBegin)
{
	time_t refts = Utility::GetTime();
	tm reference = Utility::LocalTime(refts);

	Log(LogDebug, "ScheduledDowntime")
	    << "Finding next scheduled downtime segment for time " << refts
	    << " (minBegin " << (minBegin > 0 ? Utility::FormatDateTime("%c", minBegin) : "-") << ")";

	Dictionary::Ptr ranges = GetRanges();

	if (!ranges)
		return std::make_pair(0, 0);

	Array::Ptr segments = new Array();

	Dictionary::Ptr bestSegment;
	double bestBegin, bestEnd;
	double now = Utility::GetTime();

	ObjectLock olock(ranges);

	/* Find the segment starting earliest, but not earlier than minBegin */
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
		if (minBegin && begin < minBegin) {
			Log(LogDebug, "ScheduledDowntime") << "beginning to early.";
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
	/* Try to merge the next segment into a running downtime */
	Log(LogDebug, "ScheduledDowntime") << "Try merge";
	for (const Downtime::Ptr& downtime : GetCheckable()->GetDowntimes()) {
		double current_end;
		if (downtime->GetScheduledBy() != GetName()) {
			Log(LogDebug, "ScheduledDowntime") << "Not by us (" << downtime->GetScheduledBy() << " != " << GetName() << ")";
			continue;
		}
		current_end = downtime->GetEndTime();
		if (current_end > Utility::GetTime() + 12*60*60) {
			Log(LogDebug, "ScheduledDowntime") << "By us, long-running (" << Utility::FormatDateTime("%c", current_end) << ")";
			/* return anyway, don't queue a new downtime now */
			return;
		} else {
			Log(LogDebug, "ScheduledDowntime") << "By us, ends soon (" << Utility::FormatDateTime("%c", current_end) << ")";
			std::pair<double, double> segment = FindNextSegment(current_end);
			/* Merge an immediately following segment */
			if (segment.first == current_end) {
				Log(LogDebug, "ScheduledDowntime") << "Next Segment fits, extending end time " << Utility::FormatDateTime("%c", current_end) << " to " << Utility::FormatDateTime("%c", segment.second);
				downtime->SetEndTime(segment.second, false);
				return;
			} else {
				Log(LogDebug, "ScheduledDowntime") << "Next Segment doesn't fit: " << Utility::FormatDateTime("%c", segment.first) << " != " << Utility::FormatDateTime("%c", current_end);
				continue;
			}
		}
	}
	Log(LogDebug, "ScheduledDowntime") << "No merge";

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

	Downtime::AddDowntime(GetCheckable(), GetAuthor(), GetComment(),
		segment.first, segment.second,
		GetFixed(), String(), GetDuration(), GetName(), GetName());
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

