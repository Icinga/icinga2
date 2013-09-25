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

#include "icinga/timeperiod.h"
#include "config/configitem.h"
#include "base/dynamictype.h"
#include "base/scriptfunction.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/timer.h"
#include "base/utility.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(TimePeriod);
REGISTER_SCRIPTFUNCTION(EmptyTimePeriod, &TimePeriod::EmptyTimePeriodUpdate);
REGISTER_SCRIPTFUNCTION(EvenMinutesTimePeriod, &TimePeriod::EvenMinutesTimePeriodUpdate);

static Timer::Ptr l_UpdateTimer;

void TimePeriod::Start(void)
{
	DynamicObject::Start();

	if (!l_UpdateTimer) {
		l_UpdateTimer = boost::make_shared<Timer>();
		l_UpdateTimer->SetInterval(300);
		l_UpdateTimer->OnTimerExpired.connect(boost::bind(&TimePeriod::UpdateTimerHandler));
		l_UpdateTimer->Start();
	}

	/* Pre-fill the time period for the next 24 hours. */
	double now = Utility::GetTime();
	UpdateRegion(now, now + 24 * 3600, true);
	Dump();
}

String TimePeriod::GetDisplayName(void) const
{
        if (!m_DisplayName.IsEmpty())
                return m_DisplayName;
        else
                return GetName();
}

Dictionary::Ptr TimePeriod::GetRanges(void) const
{
	return m_Ranges;
}

void TimePeriod::AddSegment(double begin, double end)
{
	ASSERT(OwnsLock());

	Log(LogDebug, "icinga", "Adding segment '" + Utility::FormatDateTime("%c", begin) + "' <-> '" + Utility::FormatDateTime("%c", end) + "' to TimePeriod '" + GetName() + "'");

	if (m_ValidBegin.IsEmpty() || begin < m_ValidBegin)
		m_ValidBegin = begin;

	if (m_ValidEnd.IsEmpty() || end > m_ValidEnd)
		m_ValidEnd = end;

	Array::Ptr segments = m_Segments;

	if (segments) {
		/* Try to merge the new segment into an existing segment. */
		ObjectLock dlock(segments);
		BOOST_FOREACH(const Dictionary::Ptr& segment, segments) {
			if (segment->Get("begin") <= begin && segment->Get("end") >= end)
				return; /* New segment is fully contained in this segment. */

			if (segment->Get("begin") < begin && segment->Get("end") > begin) {
				segment->Set("end", end); /* Extend an existing segment. */
				return;
			}

			if (segment->Get("begin") > begin && segment->Get("begin") < end) {
				segment->Set("begin", begin); /* Extend an existing segment. */
				return;
			}
		}
	}

	/* Create new segment if we weren't able to merge this into an existing segment. */
	Dictionary::Ptr segment = boost::make_shared<Dictionary>();
	segment->Set("begin", begin);
	segment->Set("end", end);

	if (!segments) {
		segments = boost::make_shared<Array>();
		m_Segments = segments;
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

	Log(LogDebug, "icinga", "Removing segment '" + Utility::FormatDateTime("%c", begin) + "' <-> '" + Utility::FormatDateTime("%c", end) + "' from TimePeriod '" + GetName() + "'");

	if (m_ValidBegin.IsEmpty() || begin < m_ValidBegin)
		m_ValidBegin = begin;

	if (m_ValidEnd.IsEmpty() || end > m_ValidEnd)
		m_ValidEnd = end;

	Array::Ptr segments = m_Segments;

	if (!segments)
		return;

	Array::Ptr newSegments = boost::make_shared<Array>();

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

	m_Segments = newSegments;

	Dump();
}

void TimePeriod::PurgeSegments(double end)
{
	ASSERT(OwnsLock());

	Log(LogDebug, "icinga", "Purging segments older than '" + Utility::FormatDateTime("%c", end) + "' from TimePeriod '" + GetName() + "'");

	if (m_ValidBegin.IsEmpty() || end < m_ValidBegin)
		return;

	m_ValidBegin = end;

	Array::Ptr segments = m_Segments;

	if (!segments)
		return;

	Array::Ptr newSegments = boost::make_shared<Array>();

	/* Remove old segments. */
	ObjectLock dlock(segments);
	BOOST_FOREACH(const Dictionary::Ptr& segment, segments) {
		if (segment->Get("end") >= end)
			newSegments->Add(segment);
	}

	m_Segments = newSegments;
}

void TimePeriod::UpdateRegion(double begin, double end, bool clearExisting)
{
	if (!clearExisting) {
		if (begin < m_ValidEnd)
			begin = m_ValidEnd;

		if (end < m_ValidEnd)
			return;
	}

	TimePeriod::Ptr self = GetSelf();

	std::vector<Value> arguments;
	arguments.push_back(self);
	arguments.push_back(begin);
	arguments.push_back(end);

	Array::Ptr segments = InvokeMethod("update", arguments);

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

bool TimePeriod::IsInside(double ts) const
{
	ObjectLock olock(this);

	if (m_ValidBegin.IsEmpty() || ts < m_ValidBegin || m_ValidEnd.IsEmpty() || ts > m_ValidEnd)
		return true; /* Assume that all invalid regions are "inside". */

	Array::Ptr segments = m_Segments;

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

	Array::Ptr segments = m_Segments;

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

	BOOST_FOREACH(const TimePeriod::Ptr& tp, DynamicType::GetObjects<TimePeriod>()) {
		double valid_end;

		{
			ObjectLock olock(tp);
			tp->PurgeSegments(now - 3600);

			valid_end = tp->m_ValidEnd;
		}

		tp->UpdateRegion(valid_end, now + 24 * 3600, false);
		tp->Dump();
	}
}

Array::Ptr TimePeriod::EmptyTimePeriodUpdate(const TimePeriod::Ptr&, double, double)
{
	Array::Ptr segments = boost::make_shared<Array>();
	return segments;
}

Array::Ptr TimePeriod::EvenMinutesTimePeriodUpdate(const TimePeriod::Ptr&, double begin, double end)
{
	Array::Ptr segments = boost::make_shared<Array>();

	for (long t = begin / 60 - 1; t * 60 < end; t++) {
		if ((t % 2) == 0) {
			Dictionary::Ptr segment = boost::make_shared<Dictionary>();
			segment->Set("begin", t * 60);
			segment->Set("end", (t + 1) * 60);

			segments->Add(segment);
		}
	}

	return segments;
}

void TimePeriod::Dump(void)
{
	Array::Ptr segments = m_Segments;

	Log(LogDebug, "icinga", "Dumping TimePeriod '" + GetName() + "'");
	Log(LogDebug, "icinga", "Valid from '" + Utility::FormatDateTime("%c", m_ValidBegin) + "' until '" + Utility::FormatDateTime("%c", m_ValidEnd));

	if (segments) {
		ObjectLock dlock(segments);
		BOOST_FOREACH(const Dictionary::Ptr& segment, segments) {
			Log(LogDebug, "icinga", "Segment: " +
			    Utility::FormatDateTime("%c", segment->Get("begin")) + " <-> " +
			    Utility::FormatDateTime("%c", segment->Get("end")));
		}
	}

	Log(LogDebug, "icinga", "---");
}

void TimePeriod::InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const
{
	DynamicObject::InternalSerialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		bag->Set("display_name", m_DisplayName);
		bag->Set("ranges", m_Ranges);
	}

	if (attributeTypes & Attribute_State) {
		bag->Set("valid_begin", m_ValidBegin);
		bag->Set("valid_end", m_ValidEnd);
		bag->Set("segments", m_Segments);
	}
}

void TimePeriod::InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes)
{
	DynamicObject::InternalDeserialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		m_DisplayName = bag->Get("display_name");
		m_Ranges = bag->Get("ranges");
	}

	if (attributeTypes & Attribute_State) {
		m_ValidBegin = bag->Get("valid_begin");
		m_ValidEnd = bag->Get("valid_end");
		m_Segments = bag->Get("segments");
	}
}
