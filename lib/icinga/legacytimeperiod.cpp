/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/legacytimeperiod.hpp"
#include "base/function.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"
#include "base/locale.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/debug.hpp"
#include "base/utility.hpp"
#include <boost/locale.hpp>

using namespace icinga;

namespace lc = boost::locale;

REGISTER_FUNCTION_NONCONST(Internal, LegacyTimePeriod, &LegacyTimePeriod::ScriptFunc, "tp:begin:end");

bool LegacyTimePeriod::IsInTimeRange(LocaleDateTime *begin, LocaleDateTime *end, int stride, LocaleDateTime *reference)
{
	time_t tsbegin, tsend, tsref;
	tsbegin = begin->time();
	tsend = end->time();
	tsref = reference->time();

	if (tsref < tsbegin || tsref > tsend)
		return false;

	int daynumber = (tsref - tsbegin) / (24 * 60 * 60);

	if (stride > 1 && daynumber % stride > 0)
		return false;

	return true;
}

void LegacyTimePeriod::FindNthWeekday(int wday, int n, LocaleDateTime *reference)
{
	int dir, seen = 0;

	if (n > 0) {
		dir = 1;
	} else {
		n *= -1;
		dir = -1;

		/* Negative days are relative to the next month. */
		*reference += lc::period::month(1);
	}

	ASSERT(n > 0);

	reference->set(lc::period::day(), 1);

	for (;;) {
		if (reference->get(lc::period::day_of_week()) == wday) {
			seen++;

			if (seen == n)
				return;
		}

		*reference += lc::period::day(dir);
	}
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

void LegacyTimePeriod::ParseTimeSpec(const String& timespec, LocaleDateTime *begin, LocaleDateTime *end, LocaleDateTime *reference)
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

			begin->set(lc::period::year(), year);
			begin->set(lc::period::month(), month - 1);
			begin->set(lc::period::day(), day);
			begin->set(lc::period::hour(), 0);
			begin->set(lc::period::minute(), 0);
			begin->set(lc::period::second(), 0);
		}

		if (end) {
			*end = *reference;

			end->set(lc::period::year(), year);
			end->set(lc::period::month(), month - 1);
			end->set(lc::period::day(), day);
			end->set(lc::period::hour(), 0);
			end->set(lc::period::minute(), 0);
			end->set(lc::period::second(), 0);

			*end += lc::period::day(1);
		}

		return;
	}

	std::vector<String> tokens = timespec.Split(" ");

	int mon = -1;

	if (tokens.size() > 1 && (tokens[0] == "day" || (mon = MonthFromString(tokens[0])) != -1)) {
		if (mon == -1)
			mon = reference->get(lc::period::month());

		int mday = Convert::ToLong(tokens[1]);

		if (begin) {
			*begin = *reference;

			begin->set(lc::period::month(), mon);
			begin->set(lc::period::hour(), 0);
			begin->set(lc::period::minute(), 0);
			begin->set(lc::period::second(), 0);

			/* day -X: Negative days are relative to the next month. */
			if (mday < 0) {
				begin->set(lc::period::day(), 1);
				*begin += lc::period::month(1);
				*begin -= lc::period::day(-mday);
			} else {
				begin->set(lc::period::day(), mday);
			}
		}

		if (end) {
			*end = *reference;

			end->set(lc::period::month(), mon);
			end->set(lc::period::hour(), 0);
			end->set(lc::period::minute(), 0);
			end->set(lc::period::second(), 0);

			/* day -X: Negative days are relative to the next month. */
			if (mday < 0) {
				end->set(lc::period::day(), 1);
				*end += lc::period::month(1);
				*end -= lc::period::day(-mday);
			} else {
				end->set(lc::period::day(), mday);
			}

			*end += lc::period::day(1);
		}

		return;
	}

	int wday;

	if (tokens.size() >= 1 && (wday = WeekdayFromString(tokens[0])) != -1) {
		LocaleDateTime myref = *reference;

		if (tokens.size() > 2) {
			mon = MonthFromString(tokens[2]);

			if (mon == -1)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid month in time specification: " + timespec));

			myref.set(lc::period::month(), mon);
		}

		int n = 0;

		if (tokens.size() > 1)
			n = Convert::ToLong(tokens[1]);

		if (begin) {
			*begin = myref;

			if (tokens.size() > 1)
				FindNthWeekday(wday, n, begin);
			else
				*begin += lc::period::day((7 - begin->get(lc::period::day_of_week()) + wday) % 7);

			begin->set(lc::period::hour(), 0);
			begin->set(lc::period::minute(), 0);
			begin->set(lc::period::second(), 0);
		}

		if (end) {
			*end = myref;

			if (tokens.size() > 1)
				FindNthWeekday(wday, n, end);
			else
				*end += lc::period::day((7 - end->get(lc::period::day_of_week()) + wday) % 7);

			end->set(lc::period::hour(), 0);
			end->set(lc::period::minute(), 0);
			end->set(lc::period::second(), 0);
			*end += lc::period::day(1);
		}

		return;
	}

	BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid time specification: " + timespec));
}

void LegacyTimePeriod::ParseTimeRange(const String& timerange, LocaleDateTime *begin, LocaleDateTime *end, int *stride, LocaleDateTime *reference)
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

bool LegacyTimePeriod::IsInDayDefinition(const String& daydef, LocaleDateTime *reference)
{
	LocaleDateTime begin, end;
	int stride;

	ParseTimeRange(daydef, &begin, &end, &stride, reference);

	Log(LogDebug, "LegacyTimePeriod")
		<< "ParseTimeRange: '" << daydef << "' => " << begin.time()
		<< " -> " << end.time() << ", stride: " << stride;

	return IsInTimeRange(&begin, &end, stride, reference);
}

static inline
void ProcessTimeRaw(const String& in, LocaleDateTime *reference, LocaleDateTime *out)
{
	*out = *reference;

	auto hd (in.Split(":"));

	switch (hd.size()) {
		case 2:
			out->set(lc::period::second(), 0);
			break;
		case 3:
			out->set(lc::period::second(), Convert::ToLong(hd[2]));
			break;
		default:
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid time specification: " + in));
	}

	out->set(lc::period::hour(), Convert::ToLong(hd[0]));
	out->set(lc::period::minute(), Convert::ToLong(hd[1]));
}

void LegacyTimePeriod::ProcessTimeRangeRaw(const String& timerange, LocaleDateTime *reference, LocaleDateTime *begin, LocaleDateTime *end)
{
	std::vector<String> times = timerange.Split("-");

	if (times.size() != 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid timerange: " + timerange));

	ProcessTimeRaw(times[0], reference, begin);
	ProcessTimeRaw(times[1], reference, end);

	if (*begin >= *end)
		*end += lc::period::day(1);
}

Dictionary::Ptr LegacyTimePeriod::ProcessTimeRange(const String& timestamp, LocaleDateTime *reference)
{
	LocaleDateTime begin, end;

	ProcessTimeRangeRaw(timestamp, reference, &begin, &end);

	return new Dictionary({
		{ "begin", begin.time() },
		{ "end", end.time() }
	});
}

void LegacyTimePeriod::ProcessTimeRanges(const String& timeranges, LocaleDateTime *reference, const Array::Ptr& result)
{
	std::vector<String> ranges = timeranges.Split(",");

	for (const String& range : ranges) {
		Dictionary::Ptr segment = ProcessTimeRange(range, reference);

		if (segment->Get("begin") >= segment->Get("end"))
			continue;

		result->Add(segment);
	}
}

Dictionary::Ptr LegacyTimePeriod::FindRunningSegment(const String& daydef, const String& timeranges, LocaleDateTime *reference)
{
	LocaleDateTime begin, end, iter;
	time_t tsend, tsiter, tsref;
	int stride;

	tsref = reference->time();

	ParseTimeRange(daydef, &begin, &end, &stride, reference);

	iter = begin;

	tsend = end.time();

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

		iter += lc::period::day(1);
		iter.set(lc::period::hour(), 0);
		iter.set(lc::period::minute(), 0);
		iter.set(lc::period::second(), 0);
		tsiter = iter.time();
	} while (tsiter < tsend);

	return nullptr;
}

Dictionary::Ptr LegacyTimePeriod::FindNextSegment(const String& daydef, const String& timeranges, LocaleDateTime *reference)
{
	LocaleDateTime begin, end, iter, ref;
	time_t tsend, tsiter, tsref;
	int stride;

	for (int pass = 1; pass <= 2; pass++) {
		if (pass == 1) {
			ref = *reference;
		} else {
			ref = end;
			ref += lc::period::day(1);
		}

		tsref = ref.time();

		ParseTimeRange(daydef, &begin, &end, &stride, &ref);

		iter = begin;

		tsend = end.time();

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

			iter += lc::period::day(1);
			iter.set(lc::period::hour(), 0);
			iter.set(lc::period::minute(), 0);
			iter.set(lc::period::second(), 0);
			tsiter = iter.time();
		} while (tsiter < tsend);
	}

	return nullptr;
}

Array::Ptr LegacyTimePeriod::ScriptFunc(const TimePeriod::Ptr& tp, double begin, double end)
{
	Array::Ptr segments = new Array();

	Dictionary::Ptr ranges = tp->GetRanges();

	if (ranges) {
		for (int i = 0; i <= (end - begin) / (24 * 60 * 60); i++) {
			time_t refts = begin + i * 24 * 60 * 60;
			LocaleDateTime reference (refts);

#ifdef I2_DEBUG
			Log(LogDebug, "LegacyTimePeriod")
				<< "Checking reference time " << refts;
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
