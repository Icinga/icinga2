/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef LEGACYTIMEPERIOD_H
#define LEGACYTIMEPERIOD_H

#include "icinga/i2-icinga.hpp"
#include "icinga/timeperiod.hpp"
#include "base/dictionary.hpp"
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

	static bool IsInTimeRange(const tm *begin, const tm *end, int stride, const tm *reference);
	static void FindNthWeekday(int wday, int n, tm *reference);
	static int WeekdayFromString(const String& daydef);
	static int MonthFromString(const String& monthdef);
	static void ParseTimeSpec(const String& timespec, tm *begin, tm *end, const tm *reference);
	static void ParseTimeRange(const String& timerange, tm *begin, tm *end, int *stride, const tm *reference);
	static bool IsInDayDefinition(const String& daydef, const tm *reference);
	static void ProcessTimeRangeRaw(const String& timerange, const tm *reference, tm *begin, tm *end);
	static Dictionary::Ptr ProcessTimeRange(const String& timerange, const tm *reference);
	static void ProcessTimeRanges(const String& timeranges, const tm *reference, const Array::Ptr& result);
	static Dictionary::Ptr FindNextSegment(const String& daydef, const String& timeranges, const tm *reference);
	static Dictionary::Ptr FindRunningSegment(const String& daydef, const String& timeranges, const tm *reference);

private:
	LegacyTimePeriod();

	static boost::gregorian::date GetEndOfMonthDay(int year, int month);
};

}

#endif /* LEGACYTIMEPERIOD_H */
