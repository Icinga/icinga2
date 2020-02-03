/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef LEGACYTIMEPERIOD_H
#define LEGACYTIMEPERIOD_H

#include "base/dictionary.hpp"
#include "icinga/i2-icinga.hpp"
#include "icinga/timeperiod.hpp"
#include <boost/date_time/gregorian/gregorian.hpp>

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
	static Dictionary::Ptr FindRunningSegment(const String& daydef, const String& timeranges, tm *reference);

private:
	LegacyTimePeriod();

	static boost::gregorian::date GetEndOfMonthDay(int year, int month);
};

}

#endif /* LEGACYTIMEPERIOD_H */
