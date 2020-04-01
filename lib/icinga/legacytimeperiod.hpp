/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef LEGACYTIMEPERIOD_H
#define LEGACYTIMEPERIOD_H

#include "icinga/i2-icinga.hpp"
#include "icinga/timeperiod.hpp"
#include "base/dictionary.hpp"
#include "base/locale.hpp"
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

	static bool IsInTimeRange(LocaleDateTime *begin, LocaleDateTime *end, int stride, LocaleDateTime *reference);
	static void FindNthWeekday(int wday, int n, LocaleDateTime *reference);
	static int WeekdayFromString(const String& daydef);
	static int MonthFromString(const String& monthdef);
	static void ParseTimeSpec(const String& timespec, LocaleDateTime *begin, LocaleDateTime *end, LocaleDateTime *reference);
	static void ParseTimeRange(const String& timerange, LocaleDateTime *begin, LocaleDateTime *end, int *stride, LocaleDateTime *reference);
	static bool IsInDayDefinition(const String& daydef, LocaleDateTime *reference);
	static void ProcessTimeRangeRaw(const String& timerange, LocaleDateTime *reference, LocaleDateTime *begin, LocaleDateTime *end);
	static Dictionary::Ptr ProcessTimeRange(const String& timerange, LocaleDateTime *reference);
	static void ProcessTimeRanges(const String& timeranges, LocaleDateTime *reference, const Array::Ptr& result);
	static Dictionary::Ptr FindNextSegment(const String& daydef, const String& timeranges, LocaleDateTime *reference);
	static Dictionary::Ptr FindRunningSegment(const String& daydef, const String& timeranges, LocaleDateTime *reference);

private:
	LegacyTimePeriod();

	static boost::gregorian::date GetEndOfMonthDay(int year, int month);
};

}

#endif /* LEGACYTIMEPERIOD_H */
