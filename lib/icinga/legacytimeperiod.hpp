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

#ifndef LEGACYTIMEPERIOD_H
#define LEGACYTIMEPERIOD_H

#include "icinga/i2-icinga.hpp"
#include "icinga/timeperiod.hpp"
#include "base/dictionary.hpp"

namespace icinga
{

/**
 * Implements Icinga 1.x time periods.
 *
 * @ingroup icinga
 */
class LegacyTimePeriod
{
public:
	static Array::Ptr ScriptFunc(const TimePeriod::Ptr& tp, double start, double end);

	static bool IsInTimeRange(tm *begin, tm *end, int stride, tm *reference);
	static void FindNthWeekday(int wday, int n, tm *reference);
	static int WeekdayFromString(const String& daydef);
	static int MonthFromString(const String& monthdef);
	static void ParseTimeSpec(const String& timespec, tm *begin, tm *end, tm *reference);
	static void ParseTimeRange(const String& timerange, tm *begin, tm *end, int *stride, tm *reference);
	static bool IsInDayDefinition(const String& daydef, tm *reference);
	static void ProcessTimeRangeRaw(const String& timerange, tm *reference, tm *begin, tm *end);
	static Dictionary::Ptr ProcessTimeRange(const String& timerange, tm *reference);
	static void ProcessTimeRanges(const String& timeranges, tm *reference, const Array::Ptr& result);
	static Dictionary::Ptr FindNextSegment(const String& daydef, const String& timeranges, tm *reference);

private:
	LegacyTimePeriod();
};

}

#endif /* LEGACYTIMEPERIOD_H */
