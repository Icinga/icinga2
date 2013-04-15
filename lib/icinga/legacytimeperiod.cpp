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

#include "icinga/legacytimeperiod.h"
#include "base/scriptfunction.h"
#include "base/convert.h"
#include "base/exception.h"
#include "base/logger_fwd.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_SCRIPTFUNCTION(LegacyTimePeriod, &LegacyTimePeriod::ScriptFunc);

bool LegacyTimePeriod::IsInDayDefinition(const String& daydef, tm *reference)
{
	if (daydef == "sunday" || daydef == "monday" || daydef == "tuesday" ||
	    daydef == "wednesday" || daydef == "thursday" || daydef == "friday" ||
	    daydef == "saturday") {
		int wday;

		if (daydef == "sunday")
			wday = 0;
		else if (daydef == "monday")
			wday = 1;
		else if (daydef == "tuesday")
			wday = 2;
		else if (daydef == "wednesday")
			wday = 3;
		else if (daydef == "thursday")
			wday = 4;
		else if (daydef == "friday")
			wday = 5;
		else if (daydef == "saturday")
			wday = 6;

		return reference->tm_wday == wday;
	}

	BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid day definition: " + daydef));
}

Dictionary::Ptr LegacyTimePeriod::ProcessTimeRange(const String& timerange, tm *reference)
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

	tm begin, end;

	begin = *reference;
	begin.tm_sec = 0;
	begin.tm_min = Convert::ToLong(hd1[1]);
	begin.tm_hour = Convert::ToLong(hd1[0]);

	end = *reference;
	end.tm_sec = 0;
	end.tm_min = Convert::ToLong(hd2[1]);
	end.tm_hour = Convert::ToLong(hd2[0]);

	Dictionary::Ptr segment = boost::make_shared<Dictionary>();
	segment->Set("begin", mktime(&begin));
	segment->Set("end", mktime(&end));

	return segment;
}

void LegacyTimePeriod::ProcessTimeRanges(const String& timeranges, tm *reference, const Array::Ptr& result)
{
	std::vector<String> ranges;

	boost::algorithm::split(ranges, timeranges, boost::is_any_of(","));

	BOOST_FOREACH(const String& range, ranges) {
		Dictionary::Ptr segment = ProcessTimeRange(range, reference);
		result->Add(segment);
	}
}

Array::Ptr LegacyTimePeriod::ScriptFunc(const TimePeriod::Ptr& tp, double begin, double end)
{
	Array::Ptr segments = boost::make_shared<Array>();

	Dictionary::Ptr ranges = tp->Get("ranges");

	time_t tempts = begin;
	tm reference;

#ifdef _MSC_VER
	tm *temp = localtime(&tempts);

	if (temp == NULL) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("localtime")
		    << boost::errinfo_errno(errno));
	}

	reference = *temp;
#else /* _MSC_VER */
	if (localtime_r(&tempts, &reference) == NULL) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("localtime_r")
		    << boost::errinfo_errno(errno));
	}
#endif /* _MSC_VER */

	if (ranges) {
		for (int i = 0; i <= (end - begin) / (24 * 60 * 60); i++) {
			String key;
			Value value;
			BOOST_FOREACH(boost::tie(key, value), ranges) {
				if (!IsInDayDefinition(key, &reference))
					continue;

				ProcessTimeRanges(value, &reference, segments);
			}

			reference.tm_wday++;
		}
	}

	Log(LogDebug, "icinga", "Legacy timeperiod update returned " + Convert::ToString(segments->GetLength()) + " segments.");

	return segments;
}

