/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/timeperiod.hpp"
#include "icinga/timeperiod-ti.cpp"
#include "icinga/legacytimeperiod.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/exception.hpp"
#include "base/logger.hpp"
#include "base/timer.hpp"
#include "base/utility.hpp"
#include <boost/thread/once.hpp>

using namespace icinga;

REGISTER_TYPE(TimePeriod);

static Timer::Ptr l_UpdateTimer;

void TimePeriod::Start(bool runtimeCreated)
{
	ObjectImpl<TimePeriod>::Start(runtimeCreated);

	static boost::once_flag once = BOOST_ONCE_INIT;

	boost::call_once(once, [this]() {
		l_UpdateTimer = new Timer();
		l_UpdateTimer->SetInterval(300);
		l_UpdateTimer->OnTimerExpired.connect([](const Timer * const&) { UpdateTimerHandler(); });
		l_UpdateTimer->Start();
	});

	/* Pre-fill the time period for the next 24 hours. */
	double now = Utility::GetTime();
	UpdateRegion(now, now + 24 * 3600, true);
#ifdef _DEBUG
	Dump();
#endif /* _DEBUG */
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
		for (const Dictionary::Ptr& segment : segments) {
			if (segment->Get("begin") <= begin && segment->Get("end") >= end)
				return; /* New segment is fully contained in this segment. */

			if (segment->Get("begin") >= begin && segment->Get("end") <= end) {
				segment->Set("begin", begin);
				segment->Set("end", end); /* Extend an existing segment to both sides */
				return;
			}

			if (segment->Get("end") >= begin && segment->Get("end") <= end) {
				segment->Set("end", end); /* Extend an existing segment to right. */
				return;
			}

			if (segment->Get("begin") >= begin && segment->Get("begin") <= end) {
				segment->Set("begin", begin); /* Extend an existing segment to left. */
				return;
			}

		}
	}

	/* Create new segment if we weren't able to merge this into an existing segment. */
	Dictionary::Ptr segment = new Dictionary({
		{ "begin", begin },
		{ "end", end }
	});

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
	for (const Dictionary::Ptr& segment : segments) {
		/* Fully contained in the specified range? */
		if (segment->Get("begin") >= begin && segment->Get("end") <= end)
			// Don't add the old segment, because the segment is fully contained into our range
			continue;

		/* Not overlapping at all? */
		if (segment->Get("end") < begin || segment->Get("begin") > end) {
			newSegments->Add(segment);
			continue;
		}

		/* Cut between */
		if (segment->Get("begin") < begin && segment->Get("end") > end) {
			newSegments->Add(new Dictionary({
				{ "begin", segment->Get("begin") },
				{ "end", begin }
			}));

			newSegments->Add(new Dictionary({
				{ "begin", end },
				{ "end", segment->Get("end") }
			}));
			// Don't add the old segment, because we have now two new segments and a gap between
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

#ifdef _DEBUG
	Dump();
#endif /* _DEBUG */
}

void TimePeriod::RemoveSegment(const Dictionary::Ptr& segment)
{
	RemoveSegment(segment->Get("begin"), segment->Get("end"));
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
	for (const Dictionary::Ptr& segment : segments) {
		if (segment->Get("end") >= end)
			newSegments->Add(segment);
	}

	SetSegments(newSegments);
}

void TimePeriod::Merge(const TimePeriod::Ptr& timeperiod, bool include)
{
	Log(LogDebug, "TimePeriod")
		<< "Merge TimePeriod '" << GetName() << "' with '" << timeperiod->GetName() << "' "
		<< "Method: " << (include ? "include" : "exclude");

	Array::Ptr segments = timeperiod->GetSegments();

	if (segments) {
		ObjectLock dlock(segments);
		ObjectLock ilock(this);
		for (const Dictionary::Ptr& segment : segments) {
			include ? AddSegment(segment) : RemoveSegment(segment);
		}
	}
}

void TimePeriod::UpdateRegion(double begin, double end, bool clearExisting)
{
	if (clearExisting) {
		ObjectLock olock(this);
		SetSegments(new Array());
	} else {
		if (begin < GetValidEnd())
			begin = GetValidEnd();

		if (end < GetValidEnd())
			return;
	}

	Array::Ptr segments = GetUpdate()->Invoke({ this, begin, end });

	{
		ObjectLock olock(this);
		RemoveSegment(begin, end);

		if (segments) {
			ObjectLock dlock(segments);
			for (const Dictionary::Ptr& segment : segments) {
				AddSegment(segment);
			}
		}
	}

	bool preferInclude = GetPreferIncludes();

	/* First handle the non preferred timeranges */
	Array::Ptr timeranges = preferInclude ? GetExcludes() : GetIncludes();

	if (timeranges) {
		ObjectLock olock(timeranges);
		for (const String& name : timeranges) {
			const TimePeriod::Ptr timeperiod = TimePeriod::GetByName(name);

			if (timeperiod)
				Merge(timeperiod, !preferInclude);
		}
	}

	/* Preferred timeranges must be handled at the end */
	timeranges = preferInclude ? GetIncludes() : GetExcludes();

	if (timeranges) {
		ObjectLock olock(timeranges);
		for (const String& name : timeranges) {
			const TimePeriod::Ptr timeperiod = TimePeriod::GetByName(name);

			if (timeperiod)
				Merge(timeperiod, preferInclude);
		}
	}
}

bool TimePeriod::GetIsInside() const
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
		for (const Dictionary::Ptr& segment : segments) {
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
		for (const Dictionary::Ptr& segment : segments) {
			if (segment->Get("begin") > begin && (segment->Get("begin") < closestTransition || closestTransition == -1))
				closestTransition = segment->Get("begin");

			if (segment->Get("end") > begin && (segment->Get("end") < closestTransition || closestTransition == -1))
				closestTransition = segment->Get("end");
		}
	}

	return closestTransition;
}

void TimePeriod::UpdateTimerHandler()
{
	double now = Utility::GetTime();

	for (const TimePeriod::Ptr& tp : ConfigType::GetObjectsByType<TimePeriod>()) {
		if (!tp->IsActive())
			continue;

		double valid_end;

		{
			ObjectLock olock(tp);
			tp->PurgeSegments(now - 3600);

			valid_end = tp->GetValidEnd();
		}

		tp->UpdateRegion(valid_end, now + 24 * 3600, false);
#ifdef _DEBUG
		tp->Dump();
#endif /* _DEBUG */
	}
}

void TimePeriod::Dump()
{
	ObjectLock olock(this);

	Array::Ptr segments = GetSegments();

	Log(LogDebug, "TimePeriod")
		<< "Dumping TimePeriod '" << GetName() << "'";

	Log(LogDebug, "TimePeriod")
		<< "Valid from '" << Utility::FormatDateTime("%c", GetValidBegin())
		<< "' until '" << Utility::FormatDateTime("%c", GetValidEnd());

	if (segments) {
		ObjectLock dlock(segments);
		for (const Dictionary::Ptr& segment : segments) {
			Log(LogDebug, "TimePeriod")
				<< "Segment: " << Utility::FormatDateTime("%c", segment->Get("begin")) << " <-> "
				<< Utility::FormatDateTime("%c", segment->Get("end"));
		}
	}

	Log(LogDebug, "TimePeriod", "---");
}

void TimePeriod::ValidateRanges(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils)
{
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
