/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/legacytimeperiod.hpp"
#include "base/function.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/debug.hpp"
#include "base/utility.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, LegacyTimePeriod, &LegacyTimePeriod::ScriptFunc, "tp:begin:end");

bool LegacyTimePeriod::IsInTimeRange(const tm *begin, const tm *end, int stride, const tm *reference)
{
	time_t tsbegin, tsend, tsref;
	tsbegin = Utility::TmToTimestamp(begin);
	tsend = Utility::TmToTimestamp(end);
	tsref = Utility::TmToTimestamp(reference);

	if (tsref < tsbegin || tsref >= tsend)
		return false;

	int daynumber = (tsref - tsbegin) / (24 * 60 * 60);

	if (stride > 1 && daynumber % stride > 0)
		return false;

	return true;
}

/**
 * Update all day-related fields of reference (tm_year, tm_mon, tm_mday, tm_wday, tm_yday) to reference the n-th
 * occurrence of a weekday (given by wday) in the month represented by the original value of reference.
 *
 * If n is negative, counting is done from the end of the month, so for example with wday=1 and n=-1, the result will be
 * the last Monday in the month given by reference.
 *
 * @param wday Weekday (0 = Sunday, 1 = Monday, ..., 6 = Saturday, like tm_wday)
 * @param n Search the n-th weekday (given by wday) in the month given by reference
 * @param reference Input for the current month and output for the given day of that moth
 */
void LegacyTimePeriod::FindNthWeekday(int wday, int n, tm *reference)
{
	// Work on a copy to only update specific fields of reference (as documented).
	tm t = *reference;

	int dir, seen = 0;

	if (n > 0) {
		dir = 1;

		/* Postitive days are relative to the first day of the month. */
		t.tm_mday = 1;
	} else {
		n *= -1;
		dir = -1;

		/* Negative days are relative to the last day of the month which is
		 * what mktime() normalizes the 0th day of the next month to. */
		t.tm_mon++;
		t.tm_mday = 0;
	}

	ASSERT(n > 0);

	for (;;) {
		// Always operate on 00:00:00 with automatic DST detection, otherwise days could
		// be skipped or counted twice if +-24 hours is not on the next or previous day.
		t.tm_hour = 0;
		t.tm_min = 0;
		t.tm_sec = 0;
		t.tm_isdst = -1;

		Utility::NormalizeTm(&t);

		if (t.tm_wday == wday) {
			seen++;

			if (seen == n)
				break;
		}

		t.tm_mday += dir;
	}

	reference->tm_year = t.tm_year;
	reference->tm_mon  = t.tm_mon;
	reference->tm_mday = t.tm_mday;
	reference->tm_wday = t.tm_wday;
	reference->tm_yday = t.tm_yday;
}

int LegacyTimePeriod::WeekdayFromString(const String& daydef)
{
	if (daydef == "sunday")
		return 0;
	else if (daydef == "monday")
		return 1;
	else if (daydef == "tuesday")
		return 2;
	else if (daydef == "wednesday")
		return 3;
	else if (daydef == "thursday")
		return 4;
	else if (daydef == "friday")
		return 5;
	else if (daydef == "saturday")
		return 6;
	else
		return -1;
}

int LegacyTimePeriod::MonthFromString(const String& monthdef)
{
	if (monthdef == "january")
		return 0;
	else if (monthdef == "february")
		return 1;
	else if (monthdef == "march")
		return 2;
	else if (monthdef == "april")
		return 3;
	else if (monthdef == "may")
		return 4;
	else if (monthdef == "june")
		return 5;
	else if (monthdef == "july")
		return 6;
	else if (monthdef == "august")
		return 7;
	else if (monthdef == "september")
		return 8;
	else if (monthdef == "october")
		return 9;
	else if (monthdef == "november")
		return 10;
	else if (monthdef == "december")
		return 11;
	else
		return -1;
}

boost::gregorian::date LegacyTimePeriod::GetEndOfMonthDay(int year, int month)
{
	boost::gregorian::date d(boost::gregorian::greg_year(year), boost::gregorian::greg_month(month), 1);

	return d.end_of_month();
}

/**
 * Finds the first day on or after the day given by reference and writes the beginning and end time of that day to
 * the output parameters begin and end.
 *
 * @param timespec Day to find, for example "2021-10-20", "sunday", ...
 * @param begin if != nullptr, set to 00:00:00 on that day
 * @param end if != nullptr, set to 24:00:00 on that day (i.e. 00:00:00 of the next day)
 * @param reference Time to begin the search at
 */
void LegacyTimePeriod::ParseTimeSpec(const String& timespec, tm *begin, tm *end, const tm *reference)
{
	/* YYYY-MM-DD */
	if (timespec.GetLength() == 10 && timespec[4] == '-' && timespec[7] == '-') {
		int year = Convert::ToLong(timespec.SubStr(0, 4));
		int month = Convert::ToLong(timespec.SubStr(5, 2));
		int day = Convert::ToLong(timespec.SubStr(8, 2));

		if (month < 1 || month > 12)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid month in time specification: " + timespec));
		if (day < 1 || day > 31)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid day in time specification: " + timespec));

		if (begin) {
			*begin = *reference;
			begin->tm_year = year - 1900;
			begin->tm_mon = month - 1;
			begin->tm_mday = day;
			begin->tm_hour = 0;
			begin->tm_min = 0;
			begin->tm_sec = 0;
			begin->tm_isdst = -1;
		}

		if (end) {
			*end = *reference;
			end->tm_year = year - 1900;
			end->tm_mon = month - 1;
			end->tm_mday = day;
			end->tm_hour = 24;
			end->tm_min = 0;
			end->tm_sec = 0;
			end->tm_isdst = -1;
		}

		return;
	}

	std::vector<String> tokens = timespec.Split(" ");

	int mon = -1;

	if (tokens.size() > 1 && (tokens[0] == "day" || (mon = MonthFromString(tokens[0])) != -1)) {
		if (mon == -1)
			mon = reference->tm_mon;

		int mday = Convert::ToLong(tokens[1]);

		if (begin) {
			*begin = *reference;
			begin->tm_mon = mon;
			begin->tm_mday = mday;
			begin->tm_hour = 0;
			begin->tm_min = 0;
			begin->tm_sec = 0;
			begin->tm_isdst = -1;

			/* day -X: Negative days are relative to the next month. */
			if (mday < 0) {
				boost::gregorian::date d(GetEndOfMonthDay(reference->tm_year + 1900, mon + 1)); //TODO: Refactor this mess into full Boost.DateTime

				//Depending on the number, we need to substract specific days (counting starts at 0).
				d = d - boost::gregorian::days(mday * -1 - 1);

				*begin = boost::gregorian::to_tm(d);
				begin->tm_hour = 0;
				begin->tm_min = 0;
				begin->tm_sec = 0;
			}
		}

		if (end) {
			*end = *reference;
			end->tm_mon = mon;
			end->tm_mday = mday;
			end->tm_hour = 24;
			end->tm_min = 0;
			end->tm_sec = 0;
			end->tm_isdst = -1;

			/* day -X: Negative days are relative to the next month. */
			if (mday < 0) {
				boost::gregorian::date d(GetEndOfMonthDay(reference->tm_year + 1900, mon + 1)); //TODO: Refactor this mess into full Boost.DateTime

				//Depending on the number, we need to substract specific days (counting starts at 0).
				d = d - boost::gregorian::days(mday * -1 - 1);

				// End date is one day in the future, starting 00:00:00
				d = d + boost::gregorian::days(1);

				*end = boost::gregorian::to_tm(d);
				end->tm_hour = 0;
				end->tm_min = 0;
				end->tm_sec = 0;
			}
		}

		return;
	}

	int wday;

	if (tokens.size() >= 1 && (wday = WeekdayFromString(tokens[0])) != -1) {
		tm myref = *reference;
		myref.tm_isdst = -1;

		if (tokens.size() > 2) {
			mon = MonthFromString(tokens[2]);

			if (mon == -1)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid month in time specification: " + timespec));

			myref.tm_mon = mon;
		}

		int n = 0;

		if (tokens.size() > 1)
			n = Convert::ToLong(tokens[1]);

		if (begin) {
			*begin = myref;

			if (tokens.size() > 1)
				FindNthWeekday(wday, n, begin);
			else
				begin->tm_mday += (7 - begin->tm_wday + wday) % 7;

			begin->tm_hour = 0;
			begin->tm_min = 0;
			begin->tm_sec = 0;
		}

		if (end) {
			*end = myref;

			if (tokens.size() > 1)
				FindNthWeekday(wday, n, end);
			else
				end->tm_mday += (7 - end->tm_wday + wday) % 7;

			end->tm_hour = 0;
			end->tm_min = 0;
			end->tm_sec = 0;
			end->tm_mday++;
		}

		return;
	}

	BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid time specification: " + timespec));
}

/**
 * Parse a range of days.
 *
 * The input can have the following formats:
 *   begin
 *   begin - end
 *   begin / stride
 *   begin - end / stride
 *
 * @param timerange Text representation of a day range or a single day, for example "2021-10-20", "monday - friday", ...
 * @param begin Output parameter set to 00:00:00 of the first day of the range
 * @param end Output parameter set to 24:00:00 of the last day of the range (i.e. 00:00:00 of the day after)
 * @param stride Output parameter for the stride (for every n-th day)
 * @param reference Expand the range relative to this timestamp
 */
void LegacyTimePeriod::ParseTimeRange(const String& timerange, tm *begin, tm *end, int *stride, const tm *reference)
{
	String def = timerange;

	/* Figure out the stride. */
	size_t pos = def.FindFirstOf('/');

	if (pos != String::NPos) {
		String strStride = def.SubStr(pos + 1).Trim();
		*stride = Convert::ToLong(strStride);

		/* Remove the stride parameter from the definition. */
		def = def.SubStr(0, pos);
	} else {
		*stride = 1; /* User didn't specify anything, assume default. */
	}

	/* Figure out whether the user has specified two dates. */
	pos = def.Find("- ");

	if (pos != String::NPos) {
		String first = def.SubStr(0, pos).Trim();

		String second = def.SubStr(pos + 1).Trim();

		ParseTimeSpec(first, begin, nullptr, reference);

		/* If the second definition starts with a number we need
		 * to add the first word from the first definition, e.g.:
		 * day 1 - 15 --> "day 15" */
		bool is_number = true;
		size_t xpos = second.FindFirstOf(' ');
		String fword = second.SubStr(0, xpos);

		try {
			Convert::ToLong(fword);
		} catch (...) {
			is_number = false;
		}

		if (is_number) {
			xpos = first.FindFirstOf(' ');
			ASSERT(xpos != String::NPos);
			second = first.SubStr(0, xpos + 1) + second;
		}

		ParseTimeSpec(second, nullptr, end, reference);
	} else {
		ParseTimeSpec(def, begin, end, reference);
	}
}

bool LegacyTimePeriod::IsInDayDefinition(const String& daydef, const tm *reference)
{
	tm begin, end;
	int stride;

	ParseTimeRange(daydef, &begin, &end, &stride, reference);

	Log(LogDebug, "LegacyTimePeriod")
		<< "ParseTimeRange: '" << daydef << "' => " << Utility::TmToTimestamp(&begin)
		<< " -> " << Utility::TmToTimestamp(&end) << ", stride: " << stride;

	return IsInTimeRange(&begin, &end, stride, reference);
}

static inline
void ProcessTimeRaw(const String& in, const tm *reference, tm *out)
{
	*out = *reference;

	auto hd (in.Split(":"));

	switch (hd.size()) {
		case 2:
			out->tm_sec = 0;
			break;
		case 3:
			out->tm_sec = Convert::ToLong(hd[2]);
			break;
		default:
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid time specification: " + in));
	}

	out->tm_hour = Convert::ToLong(hd[0]);
	out->tm_min = Convert::ToLong(hd[1]);
}

void LegacyTimePeriod::ProcessTimeRangeRaw(const String& timerange, const tm *reference, tm *begin, tm *end)
{
	std::vector<String> times = timerange.Split("-");

	if (times.size() != 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid timerange: " + timerange));

	ProcessTimeRaw(times[0], reference, begin);
	ProcessTimeRaw(times[1], reference, end);

	if (begin->tm_hour * 3600 + begin->tm_min * 60 + begin->tm_sec >=
		end->tm_hour * 3600 + end->tm_min * 60 + end->tm_sec)
		end->tm_hour += 24;
}

Dictionary::Ptr LegacyTimePeriod::ProcessTimeRange(const String& timestamp, const tm *reference)
{
	tm begin, end;

	ProcessTimeRangeRaw(timestamp, reference, &begin, &end);

	return new Dictionary({
		{ "begin", (long)Utility::TmToTimestamp(&begin) },
		{ "end", (long)Utility::TmToTimestamp(&end) }
	});
}

/**
 * Takes a list of timeranges end expands them to concrete timestamp based on a reference time.
 *
 * @param timeranges String of comma separated time ranges, for example "10:00-12:00", "12:15:30-12:23:43,16:00-18:00"
 * @param reference Starting point for searching the segments
 * @param result For each range, a dict with keys "begin" and "end" is added
 */
void LegacyTimePeriod::ProcessTimeRanges(const String& timeranges, const tm *reference, const Array::Ptr& result)
{
	std::vector<String> ranges = timeranges.Split(",");

	for (const String& range : ranges) {
		Dictionary::Ptr segment = ProcessTimeRange(range, reference);

		if (segment->Get("begin") >= segment->Get("end"))
			continue;

		result->Add(segment);
	}
}

Dictionary::Ptr LegacyTimePeriod::FindRunningSegment(const String& daydef, const String& timeranges, const tm *reference)
{
	tm begin, end, iter;
	time_t tsend, tsiter, tsref;
	int stride;

	tsref = Utility::TmToTimestamp(reference);

	ParseTimeRange(daydef, &begin, &end, &stride, reference);

	iter = begin;

	tsend = Utility::NormalizeTm(&end);

	do {
		if (IsInTimeRange(&begin, &end, stride, &iter)) {
			Array::Ptr segments = new Array();
			ProcessTimeRanges(timeranges, &iter, segments);

			Dictionary::Ptr bestSegment;
			double bestEnd = 0.0;

			ObjectLock olock(segments);
			for (const Dictionary::Ptr& segment : segments) {
				double begin = segment->Get("begin");
				double end = segment->Get("end");

				if (begin >= tsref || end < tsref)
					continue;

				if (!bestSegment || end > bestEnd) {
					bestSegment = segment;
					bestEnd = end;
				}
			}

			if (bestSegment)
				return bestSegment;
		}

		iter.tm_mday++;
		iter.tm_hour = 0;
		iter.tm_min = 0;
		iter.tm_sec = 0;
		tsiter = Utility::NormalizeTm(&iter);
	} while (tsiter < tsend);

	return nullptr;
}

Dictionary::Ptr LegacyTimePeriod::FindNextSegment(const String& daydef, const String& timeranges, const tm *reference)
{
	tm begin, end, iter, ref;
	time_t tsend, tsiter, tsref;
	int stride;

	for (int pass = 1; pass <= 2; pass++) {
		if (pass == 1) {
			ref = *reference;
		} else {
			ref = end;
			ref.tm_mday++;
		}

		tsref = Utility::NormalizeTm(&ref);

		ParseTimeRange(daydef, &begin, &end, &stride, &ref);

		iter = begin;

		tsend = Utility::NormalizeTm(&end);

		do {
			if (IsInTimeRange(&begin, &end, stride, &iter)) {
				Array::Ptr segments = new Array();
				ProcessTimeRanges(timeranges, &iter, segments);

				Dictionary::Ptr bestSegment;
				double bestBegin;

				ObjectLock olock(segments);
				for (const Dictionary::Ptr& segment : segments) {
					double begin = segment->Get("begin");

					if (begin < tsref)
						continue;

					if (!bestSegment || begin < bestBegin) {
						bestSegment = segment;
						bestBegin = begin;
					}
				}

				if (bestSegment)
					return bestSegment;
			}

			iter.tm_mday++;
			iter.tm_hour = 0;
			iter.tm_min = 0;
			iter.tm_sec = 0;
			tsiter = Utility::NormalizeTm(&iter);
		} while (tsiter < tsend);
	}

	return nullptr;
}

Array::Ptr LegacyTimePeriod::ScriptFunc(const TimePeriod::Ptr& tp, double begin, double end)
{
	Array::Ptr segments = new Array();

	Dictionary::Ptr ranges = tp->GetRanges();

	if (ranges) {
		tm tm_begin = Utility::LocalTime(begin);

		// Always evaluate time periods for full days as their ranges are given per day.
		tm_begin.tm_hour = 0;
		tm_begin.tm_min = 0;
		tm_begin.tm_sec = 0;
		tm_begin.tm_isdst = -1;

		// Helper to move a struct tm to midnight of the next day for the loop below.
		// Due to DST changes, this may move the time by something else than 24 hours.
		auto advance_to_next_day = [](tm *t) {
			t->tm_mday++;
			t->tm_hour = 0;
			t->tm_min = 0;
			t->tm_sec = 0;
			t->tm_isdst = -1;

			// Normalize fields using mktime.
			Utility::NormalizeTm(t);

			// Reset tm_isdst so that future calls figure out the correct time zone after setting tm_hour/tm_min/tm_sec.
			t->tm_isdst = -1;
		};

		for (tm reference = tm_begin; Utility::TmToTimestamp(&reference) <= end; advance_to_next_day(&reference)) {

#ifdef I2_DEBUG
			Log(LogDebug, "LegacyTimePeriod")
				<< "Checking reference time " << Utility::TmToTimestamp(&reference);
#endif /* I2_DEBUG */

			ObjectLock olock(ranges);
			for (const Dictionary::Pair& kv : ranges) {
				if (!IsInDayDefinition(kv.first, &reference)) {
#ifdef I2_DEBUG
					Log(LogDebug, "LegacyTimePeriod")
						<< "Not in day definition '" << kv.first << "'.";
#endif /* I2_DEBUG */
					continue;
				}

#ifdef I2_DEBUG
				Log(LogDebug, "LegacyTimePeriod")
					<< "In day definition '" << kv.first << "'.";
#endif /* I2_DEBUG */

				ProcessTimeRanges(kv.second, &reference, segments);
			}
		}
	}

	Log(LogDebug, "LegacyTimePeriod")
		<< "Legacy timeperiod update returned " << segments->GetLength() << " segments.";

	return segments;
}
