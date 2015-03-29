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

#include "icinga/timeperiod.hpp"
#include "icinga/legacytimeperiod.hpp"
#include "base/dynamictype.hpp"
#include "base/objectlock.hpp"
#include "base/exception.hpp"
#include "base/logger.hpp"
#include "base/timer.hpp"
#include "base/utility.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(TimePeriod);
REGISTER_SCRIPTFUNCTION(ValidateTimePeriodRanges, &TimePeriod::ValidateRanges);

static Timer::Ptr l_UpdateTimer;

INITIALIZE_ONCE(&TimePeriod::StaticInitialize);

void TimePeriod::StaticInitialize(void)
{
	l_UpdateTimer = new Timer();
	l_UpdateTimer->SetInterval(300);
	l_UpdateTimer->OnTimerExpired.connect(boost::bind(&TimePeriod::UpdateTimerHandler));
	l_UpdateTimer->Start();
}

void TimePeriod::Start(void)
{
	DynamicObject::Start();

	/* Pre-fill the time period for the next 24 hours. */
	double now = Utility::GetTime();
	UpdateRegion(now, now + 24 * 3600, true);
	Dump();
}

void TimePeriod::AddSegment(double begin, double end)
{
	ASSERT(OwnsLock());

	Log(LogDebug, "TimePeriod")
	    << "Adding segment '" << Utility::FormatDateTime("%c", begin) << "' <-> '"
	    << Utility::FormatDateTime("%c", end) << "' to TimePeriod '" << GetName() << "'";

	if (GetValidBegin().IsEmpty() || begin < GetValidBegin())
		SetValidBegin(begin);

	if (GetValidEnd().IsEmpty() || end > GetValidEnd())
		SetValidEnd(end);

	Array::Ptr segments = GetSegments();

	if (segments) {
		/* Try to merge the new segment into an existing segment. */
		ObjectLock dlock(segments);
		BOOST_FOREACH(const Dictionary::Ptr& segment, segments) {
			if (segment->Get("begin") <= begin && segment->Get("end") >= end)
				return; /* New segment is fully contained in this segment. */

			if (segment->Get("begin") <= begin && segment->Get("end") >= begin) {
				segment->Set("end", end); /* Extend an existing segment. */
				return;
			}

			if (segment->Get("begin") >= begin && segment->Get("begin") <= end) {
				segment->Set("begin", begin); /* Extend an existing segment. */
				return;
			}
		}
	}

	/* Create new segment if we weren't able to merge this into an existing segment. */
	Dictionary::Ptr segment = new Dictionary();
	segment->Set("begin", begin);
	segment->Set("end", end);

	if (!segments) {
		segments = new Array();
		SetSegments(segments);
	}

	segments->Add(segment);
}

void TimePeriod::AddSegment(const Dictionary::Ptr& segment)
{
	AddSegment(segment->Get("begin"), segment->Get("end"));
}

void TimePeriod::RemoveSegment(double begin, double end)
{
	ASSERT(OwnsLock());

	Log(LogDebug, "TimePeriod")
	    << "Removing segment '" << Utility::FormatDateTime("%c", begin) << "' <-> '"
	    << Utility::FormatDateTime("%c", end) << "' from TimePeriod '" << GetName() << "'";

	if (GetValidBegin().IsEmpty() || begin < GetValidBegin())
		SetValidBegin(begin);

	if (GetValidEnd().IsEmpty() || end > GetValidEnd())
		SetValidEnd(end);

	Array::Ptr segments = GetSegments();

	if (!segments)
		return;

	Array::Ptr newSegments = new Array();

	/* Try to split or adjust an existing segment. */
	ObjectLock dlock(segments);
	BOOST_FOREACH(const Dictionary::Ptr& segment, segments) {
		/* Fully contained in the specified range? */
		if (segment->Get("begin") >= begin && segment->Get("end") <= end)
			continue;

		/* Not overlapping at all? */
		if (segment->Get("end") < begin || segment->Get("begin") > end) {
			newSegments->Add(segment);

			continue;
		}

		/* Adjust the begin/end timestamps so as to not overlap with the specified range. */
		if (segment->Get("begin") > begin && segment->Get("begin") < end)
			segment->Set("begin", end);

		if (segment->Get("end") > begin && segment->Get("end") < end)
			segment->Set("end", begin);

		newSegments->Add(segment);
	}

	SetSegments(newSegments);

	Dump();
}

void TimePeriod::PurgeSegments(double end)
{
	ASSERT(OwnsLock());

	Log(LogDebug, "TimePeriod")
	    << "Purging segments older than '" << Utility::FormatDateTime("%c", end)
	    << "' from TimePeriod '" << GetName() << "'";

	if (GetValidBegin().IsEmpty() || end < GetValidBegin())
		return;

	SetValidBegin(end);

	Array::Ptr segments = GetSegments();

	if (!segments)
		return;

	Array::Ptr newSegments = new Array();

	/* Remove old segments. */
	ObjectLock dlock(segments);
	BOOST_FOREACH(const Dictionary::Ptr& segment, segments) {
		if (segment->Get("end") >= end)
			newSegments->Add(segment);
	}

	SetSegments(newSegments);
}

void TimePeriod::UpdateRegion(double begin, double end, bool clearExisting)
{
	if (!clearExisting) {
		if (begin < GetValidEnd())
			begin = GetValidEnd();

		if (end < GetValidEnd())
			return;
	}

	std::vector<Value> arguments;
	arguments.push_back(this);
	arguments.push_back(begin);
	arguments.push_back(end);

	Array::Ptr segments = GetUpdate()->Invoke(arguments);

	{
		ObjectLock olock(this);
		RemoveSegment(begin, end);

		if (segments) {
			ObjectLock dlock(segments);
			BOOST_FOREACH(const Dictionary::Ptr& segment, segments) {
				AddSegment(segment);
			}
		}
	}
}

bool TimePeriod::GetIsInside(void) const
{
	return IsInside(Utility::GetTime());
}

bool TimePeriod::IsInside(double ts) const
{
	ObjectLock olock(this);

	if (GetValidBegin().IsEmpty() || ts < GetValidBegin() || GetValidEnd().IsEmpty() || ts > GetValidEnd())
		return true; /* Assume that all invalid regions are "inside". */

	Array::Ptr segments = GetSegments();

	if (segments) {
		ObjectLock dlock(segments);
		BOOST_FOREACH(const Dictionary::Ptr& segment, segments) {
			if (ts > segment->Get("begin") && ts < segment->Get("end"))
				return true;
		}
	}

	return false;
}

double TimePeriod::FindNextTransition(double begin)
{
	ObjectLock olock(this);

	Array::Ptr segments = GetSegments();

	double closestTransition = -1;

	if (segments) {
		ObjectLock dlock(segments);
		BOOST_FOREACH(const Dictionary::Ptr& segment, segments) {
			if (segment->Get("begin") > begin && (segment->Get("begin") < closestTransition || closestTransition == -1))
				closestTransition = segment->Get("begin");

			if (segment->Get("end") > begin && (segment->Get("end") < closestTransition || closestTransition == -1))
				closestTransition = segment->Get("end");
		}
	}

	return closestTransition;
}

void TimePeriod::UpdateTimerHandler(void)
{
	double now = Utility::GetTime();

	BOOST_FOREACH(const TimePeriod::Ptr& tp, DynamicType::GetObjectsByType<TimePeriod>()) {
		double valid_end;

		{
			ObjectLock olock(tp);
			tp->PurgeSegments(now - 3600);

			valid_end = tp->GetValidEnd();
		}

		tp->UpdateRegion(valid_end, now + 24 * 3600, false);
		tp->Dump();
	}
}

void TimePeriod::Dump(void)
{
	Array::Ptr segments = GetSegments();

	Log(LogDebug, "TimePeriod")
	    << "Dumping TimePeriod '" << GetName() << "'";

	Log(LogDebug, "TimePeriod")
	    << "Valid from '" << Utility::FormatDateTime("%c", GetValidBegin())
	    << "' until '" << Utility::FormatDateTime("%c", GetValidEnd());

	if (segments) {
		ObjectLock dlock(segments);
		BOOST_FOREACH(const Dictionary::Ptr& segment, segments) {
			Log(LogDebug, "TimePeriod")
			    << "Segment: " << Utility::FormatDateTime("%c", segment->Get("begin")) << " <-> "
			    << Utility::FormatDateTime("%c", segment->Get("end"));
		}
	}

	Log(LogDebug, "TimePeriod", "---");
}

void TimePeriod::ValidateRanges(const String& location, const TimePeriod::Ptr& object)
{
	Dictionary::Ptr ranges = object->GetRanges();

	if (!ranges)
		return;

	/* create a fake time environment to validate the definitions */
	time_t refts = Utility::GetTime();
	tm reference = Utility::LocalTime(refts);
	Array::Ptr segments = new Array();

	ObjectLock olock(ranges);
	BOOST_FOREACH(const Dictionary::Pair& kv, ranges) {
		try {
			tm begin_tm, end_tm;
			int stride;
			LegacyTimePeriod::ParseTimeRange(kv.first, &begin_tm, &end_tm, &stride, &reference);
		} catch (const std::exception& ex) {
			BOOST_THROW_EXCEPTION(ScriptError("Validation failed for " +
			    location + ": Invalid time specification '" + kv.first + "': " + ex.what(), object->GetDebugInfo()));
		}

		try {
			LegacyTimePeriod::ProcessTimeRanges(kv.second, &reference, segments);
		} catch (const std::exception& ex) {
			BOOST_THROW_EXCEPTION(ScriptError("Validation failed for " +
			    location + ": Invalid time range definition '" + kv.second + "': " + ex.what(), object->GetDebugInfo()));
		}
	}
}
