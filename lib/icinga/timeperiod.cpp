/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "i2-icinga.h"

using namespace icinga;

REGISTER_TYPE(TimePeriod);
REGISTER_SCRIPTFUNCTION(EmptyTimePeriod, &TimePeriod::EmptyTimePeriodUpdate);
REGISTER_SCRIPTFUNCTION(EvenMinutesTimePeriod, &TimePeriod::EvenMinutesTimePeriodUpdate);

Timer::Ptr TimePeriod::m_UpdateTimer;

TimePeriod::TimePeriod(const Dictionary::Ptr& serializedUpdate)
	: DynamicObject(serializedUpdate)
{
	RegisterAttribute("valid_begin", Attribute_Replicated, &m_ValidBegin);
	RegisterAttribute("valid_end", Attribute_Replicated, &m_ValidEnd);
	RegisterAttribute("segments", Attribute_Replicated, &m_Segments);

	if (!m_UpdateTimer) {
		m_UpdateTimer = boost::make_shared<Timer>();
		m_UpdateTimer->SetInterval(300);
		m_UpdateTimer->OnTimerExpired.connect(boost::bind(&TimePeriod::UpdateTimerHandler));
		m_UpdateTimer->Start();
	}
}

void TimePeriod::Start(void)
{
	/* Pre-fill the time period for the next 24 hours. */
	double now = Utility::GetTime();
	UpdateRegion(now, now + 24 * 3600);
}

/**
 * @threadsafety Always.
 */
TimePeriod::Ptr TimePeriod::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("TimePeriod", name);

	return dynamic_pointer_cast<TimePeriod>(configObject);
}

void TimePeriod::AddSegment(double begin, double end)
{
	ASSERT(OwnsLock());

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
	Touch("segments");
}

void TimePeriod::RemoveSegment(double begin, double end)
{
	ASSERT(OwnsLock());

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
		if (segment->Get("begin") < end)
			segment->Set("begin", end);

		if (segment->Get("end") > begin)
			segment->Set("end", begin);

		newSegments->Add(segment);
	}

	m_Segments = newSegments;
	Touch("segments");
}

void TimePeriod::PurgeSegments(double end)
{
	ASSERT(OwnsLock());

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
	Touch("segments");
}

void TimePeriod::UpdateRegion(double begin, double end)
{
	if (begin < m_ValidEnd)
		begin = m_ValidEnd;

	if (end < m_ValidEnd)
		return;

	TimePeriod::Ptr self = GetSelf();

	vector<Value> arguments;
	arguments.push_back(self);
	arguments.push_back(begin);
	arguments.push_back(end);
	ScriptTask::Ptr task = MakeMethodTask("update", arguments);

	if (!task) {
		Logger::Write(LogWarning, "icinga", "TimePeriod object '" + GetName() + "' doesn't have an 'update' method.");

		return;
	}

	task->Start();
	task->GetResult();
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

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("TimePeriod")) {
		TimePeriod::Ptr tp = static_pointer_cast<TimePeriod>(object);

		/* Only update time periods that have been defined on this node. */
		if (!ConfigItem::GetObject("TimePeriod", tp->GetName()))
			continue;

		{
			ObjectLock olock(tp);
			tp->PurgeSegments(now - 3600);
		}

		if (tp->m_ValidEnd < now + 3 * 3600)
			tp->UpdateRegion(tp->m_ValidEnd, tp->m_ValidEnd + 24 * 3600);
	}
}

void TimePeriod::EmptyTimePeriodUpdate(const ScriptTask::Ptr& task, const vector<Value>& arguments)
{
	if (arguments.size() < 3)
		BOOST_THROW_EXCEPTION(runtime_error("Expected 3 arguments."));

	TimePeriod::Ptr tp = arguments[0];
	double begin = arguments[1];
	double end = arguments[2];

	ObjectLock olock(tp);
	tp->RemoveSegment(begin, end);

	task->FinishResult(Empty);
}

void TimePeriod::EvenMinutesTimePeriodUpdate(const ScriptTask::Ptr& task, const vector<Value>& arguments)
{
	if (arguments.size() < 3)
		BOOST_THROW_EXCEPTION(runtime_error("Expected 3 arguments."));

	TimePeriod::Ptr tp = arguments[0];
	double begin = arguments[1];
	double end = arguments[2];

	{
		ObjectLock olock(tp);

		tp->RemoveSegment(begin, end);

		for (long t = begin; t < end; t += 60) {
			if ((t / 60) % 2 == 0)
				tp->AddSegment(t, t + 60);
		}
	}

	task->FinishResult(Empty);
}
