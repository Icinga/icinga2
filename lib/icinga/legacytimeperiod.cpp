/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "icinga/legacytimeperiod.hpp"
#include "base/function.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/debug.hpp"
#include "base/utility.hpp"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace icinga;

REGISTER_SCRIPTFUNCTION_NS(Internal, LegacyTimePeriod, &LegacyTimePeriod::ScriptFunc, "tp:begin:end");

bool LegacyTimePeriod::IsInTimeRange(tm *begin, tm *end, int stride, tm *reference)
{
	time_t tsbegin, tsend, tsref;
	tsbegin = mktime(begin);
	tsend = mktime(end);
	tsref = mktime(reference);

	if (tsref < tsbegin || tsref > tsend)
		return false;

	int daynumber = (tsref - tsbegin) / (24 * 60 * 60);

	if (stride > 1 && daynumber % stride == 0)
		return false;

	return true;
}

void LegacyTimePeriod::FindNthWeekday(int wday, int n, tm *reference)
{
	int dir, seen = 0;

	if (n > 0) {
		dir = 1;
	} else {
		n *= -1;
		dir = -1;

		/* Negative days are relative to the next month. */
		reference->tm_mon++;
	}

	ASSERT(n > 0);

	reference->tm_mday = 1;

	for (;;) {
		mktime(reference);

		if (reference->tm_wday == wday) {
			seen++;

			if (seen == n)
				return;
		}

		reference->tm_mday += dir;
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

void LegacyTimePeriod::ParseTimeSpec(const String& timespec, tm *begin, tm *end, tm *reference)
{
	/* Let mktime() figure out whether we're in DST or not. */
	reference->tm_isdst = -1;

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
		}

		if (end) {
			*end = *reference;
			end->tm_year = year - 1900;
			end->tm_mon = month - 1;
			end->tm_mday = day;
			end->tm_hour = 24;
			end->tm_min = 0;
			end->tm_sec = 0;
		}

		return;
	}

	std::vector<String> tokens;
	boost::algorithm::split(tokens, timespec, boost::is_any_of(" "));

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

			/* Negative days are relative to the next month. */
			if (mday < 0) {
				begin->tm_mday = mday * -1 - 1;
				begin->tm_mon++;
			}
		}

		if (end) {
			*end = *reference;
			end->tm_mon = mon;
			end->tm_mday = mday;
			end->tm_hour = 24;
			end->tm_min = 0;
			end->tm_sec = 0;

			/* Negative days are relative to the next month. */
			if (mday < 0) {
				end->tm_mday = mday * -1 - 1;
				end->tm_mon++;
			}
		}

		return;
	}

	int wday;

	if (tokens.size() >= 1 && (wday = WeekdayFromString(tokens[0])) != -1) {
		tm myref = *reference;

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

void LegacyTimePeriod::ParseTimeRange(const String& timerange, tm *begin, tm *end, int *stride, tm *reference)
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

		ParseTimeSpec(first, begin, NULL, reference);

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

		ParseTimeSpec(second, NULL, end, reference);
	} else {
		ParseTimeSpec(def, begin, end, reference);
	}
}

bool LegacyTimePeriod::IsInDayDefinition(const String& daydef, tm *reference)
{
	tm begin, end;
	int stride;

	ParseTimeRange(daydef, &begin, &end, &stride, reference);

	Log(LogDebug, "LegacyTimePeriod")
	    << "ParseTimeRange: '" << daydef << "' => " << mktime(&begin)
	    << " -> " << mktime(&end) << ", stride: " << stride;

	return IsInTimeRange(&begin, &end, stride, reference);
}

void LegacyTimePeriod::ProcessTimeRangeRaw(const String& timerange, tm *reference, tm *begin, tm *end)
{
	std::vector<String> times;

	boost::algorithm::split(times, timerange, boost::is_any_of("-"));

	if (times.size() != 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid timerange: " + timerange));

	std::vector<String> hd1, hd2;
	boost::algorithm::split(hd1, times[0], boost::is_any_of(":"));

	if (hd1.size() != 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid time specification: " + times[0]));

	boost::algorithm::split(hd2, times[1], boost::is_any_of(":"));

	if (hd2.size() != 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid time specification: " + times[1]));

	*begin = *reference;
	begin->tm_sec = 0;
	begin->tm_min = Convert::ToLong(hd1[1]);
	begin->tm_hour = Convert::ToLong(hd1[0]);

	*end = *reference;
	end->tm_sec = 0;
	end->tm_min = Convert::ToLong(hd2[1]);
	end->tm_hour = Convert::ToLong(hd2[0]);

	if (begin->tm_hour * 3600 + begin->tm_min * 60 + begin->tm_sec >=
	    end->tm_hour * 3600 + end->tm_min * 60 + end->tm_sec)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Time period segment ends before it begins"));
}

Dictionary::Ptr LegacyTimePeriod::ProcessTimeRange(const String& timestamp, tm *reference)
{
	tm begin, end;

	ProcessTimeRangeRaw(timestamp, reference, &begin, &end);

	Dictionary::Ptr segment = new Dictionary();
	segment->Set("begin", (long)mktime(&begin));
	segment->Set("end", (long)mktime(&end));
	return segment;
}

void LegacyTimePeriod::ProcessTimeRanges(const String& timeranges, tm *reference, const Array::Ptr& result)
{
	std::vector<String> ranges;

	boost::algorithm::split(ranges, timeranges, boost::is_any_of(","));

	for (const String& range : ranges) {
		Dictionary::Ptr segment = ProcessTimeRange(range, reference);

		if (segment->Get("begin") >= segment->Get("end"))
			continue;

		result->Add(segment);
	}
}

Dictionary::Ptr LegacyTimePeriod::FindNextSegment(const String& daydef, const String& timeranges, tm *reference)
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

		tsref = mktime(&ref);

		ParseTimeRange(daydef, &begin, &end, &stride, &ref);

		iter = begin;

		tsend = mktime(&end);

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
			tsiter = mktime(&iter);
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
			tm reference = Utility::LocalTime(refts);

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
