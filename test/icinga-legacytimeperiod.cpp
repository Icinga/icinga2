/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/utility.hpp"
#include "icinga/legacytimeperiod.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/date_time/gregorian/conversion.hpp>
#include <boost/date_time/date.hpp>
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(icinga_legacytimeperiod);

struct GlobalTimezoneFixture
{
	char *tz;

	GlobalTimezoneFixture()
	{
		tz = getenv("TZ");
#ifdef _WIN32
		_putenv_s("TZ", "UTC");
#else
		setenv("TZ", "", 1);
#endif
		tzset();
	}

	~GlobalTimezoneFixture()
	{
#ifdef _WIN32
		if (tz)
			_putenv_s("TZ", tz);
		else
			_putenv_s("TZ", "");
#else
		if (tz)
			setenv("TZ", tz, 1);
		else
			unsetenv("TZ");
#endif
		tzset();
	}
};

BOOST_GLOBAL_FIXTURE(GlobalTimezoneFixture);

BOOST_AUTO_TEST_CASE(simple)
{
	tm tm_beg, tm_end, tm_ref;
	String timestamp;
	boost::posix_time::ptime begin;
	boost::posix_time::ptime end;
	boost::posix_time::ptime expectedBegin;
	boost::posix_time::ptime expectedEnd;

	//-----------------------------------------------------
	// check parsing of "YYYY-MM-DD" specs
	timestamp = "2016-01-01";

	expectedBegin = boost::posix_time::ptime(boost::gregorian::date(2016, 1, 1), boost::posix_time::time_duration(0, 0, 0));

	expectedEnd = boost::posix_time::ptime(boost::gregorian::date(2016, 1, 2), boost::posix_time::time_duration(0, 0, 0));

	// Run test
	LegacyTimePeriod::ParseTimeSpec(timestamp, &tm_beg, &tm_end, &tm_ref);

	// Compare times
	begin = boost::posix_time::ptime_from_tm(tm_beg);
	end = boost::posix_time::ptime_from_tm(tm_end);

	BOOST_CHECK_EQUAL(begin, expectedBegin);
	BOOST_CHECK_EQUAL(end, expectedEnd);

	//-----------------------------------------------------
	timestamp = "2015-12-31";

	expectedBegin = boost::posix_time::ptime(boost::gregorian::date(2015, 12, 31), boost::posix_time::time_duration(0, 0, 0));

	expectedEnd = boost::posix_time::ptime(boost::gregorian::date(2016, 1, 1), boost::posix_time::time_duration(0, 0, 0));

	// Run test
	LegacyTimePeriod::ParseTimeSpec(timestamp, &tm_beg, &tm_end, &tm_ref);

	// Compare times
	begin = boost::posix_time::ptime_from_tm(tm_beg);
	end = boost::posix_time::ptime_from_tm(tm_end);

	BOOST_CHECK_EQUAL(begin, expectedBegin);
	BOOST_CHECK_EQUAL(end, expectedEnd);

	//-----------------------------------------------------
	// Break things forcefully
	BOOST_CHECK_THROW(LegacyTimePeriod::ParseTimeSpec("2015-12-32", &tm_beg, &tm_end, &tm_ref),
	    std::invalid_argument);

	BOOST_CHECK_THROW(LegacyTimePeriod::ParseTimeSpec("2015-28-01", &tm_beg, &tm_end, &tm_ref),
	    std::invalid_argument);

	//-----------------------------------------------------
	// check parsing of "day X" and "day -X" specs
	timestamp = "day 2";
	tm_ref.tm_year = 2016 - 1900;
	tm_ref.tm_mon = 2 - 1;

	expectedBegin = boost::posix_time::ptime(boost::gregorian::date(2016, 2, 2), boost::posix_time::time_duration(0, 0, 0));

	expectedEnd = boost::posix_time::ptime(boost::gregorian::date(2016, 2, 3), boost::posix_time::time_duration(0, 0, 0));

	// Run Tests
	LegacyTimePeriod::ParseTimeSpec(timestamp, &tm_beg, &tm_end, &tm_ref);

	// Compare times
	begin = boost::posix_time::ptime_from_tm(tm_beg);
	end = boost::posix_time::ptime_from_tm(tm_end);

	BOOST_CHECK_EQUAL(begin, expectedBegin);
	BOOST_CHECK_EQUAL(end, expectedEnd);

	//-----------------------------------------------------
	timestamp = "day 31";
	tm_ref.tm_year = 2018 - 1900;
	tm_ref.tm_mon = 12 - 1;

	expectedBegin = boost::posix_time::ptime(boost::gregorian::date(2018, 12, 31), boost::posix_time::time_duration(0, 0, 0));

	expectedEnd = boost::posix_time::ptime(boost::gregorian::date(2019, 1, 1), boost::posix_time::time_duration(0, 0, 0));

	// Run Tests
	LegacyTimePeriod::ParseTimeSpec(timestamp, &tm_beg, &tm_end, &tm_ref);

	// Compare times
	begin = boost::posix_time::ptime_from_tm(tm_beg);
	end = boost::posix_time::ptime_from_tm(tm_end);

	BOOST_CHECK_EQUAL(begin, expectedBegin);
	BOOST_CHECK_EQUAL(end, expectedEnd);

	//-----------------------------------------------------
	// Last day of the month
	timestamp = "day -1";
	tm_ref.tm_year = 2012 - 1900;
	tm_ref.tm_mon = 7 - 1;

	expectedBegin = boost::posix_time::ptime(boost::gregorian::date(2012, 7, 31), boost::posix_time::time_duration(0, 0, 0));

	expectedEnd = boost::posix_time::ptime(boost::gregorian::date(2012, 8, 1), boost::posix_time::time_duration(0, 0, 0));

	// Run Tests
	LegacyTimePeriod::ParseTimeSpec(timestamp, &tm_beg, &tm_end, &tm_ref);

	// Compare times
	begin = boost::posix_time::ptime_from_tm(tm_beg);
	end = boost::posix_time::ptime_from_tm(tm_end);

	BOOST_CHECK_EQUAL(begin, expectedBegin);
	BOOST_CHECK_EQUAL(end, expectedEnd);

	//-----------------------------------------------------
	// Third last day of the month
	timestamp = "day -3";
	tm_ref.tm_year = 2019 - 1900;
	tm_ref.tm_mon = 7 - 1;

	expectedBegin = boost::posix_time::ptime(boost::gregorian::date(2019, 7, 29), boost::posix_time::time_duration(0, 0, 0));

	expectedEnd = boost::posix_time::ptime(boost::gregorian::date(2019, 7, 30), boost::posix_time::time_duration(0, 0, 0));

	// Run Tests
	LegacyTimePeriod::ParseTimeSpec(timestamp, &tm_beg, &tm_end, &tm_ref);

	// Compare times
	begin = boost::posix_time::ptime_from_tm(tm_beg);
	end = boost::posix_time::ptime_from_tm(tm_end);

	BOOST_CHECK_EQUAL(begin, expectedBegin);
	BOOST_CHECK_EQUAL(end, expectedEnd);

	//-----------------------------------------------------
	// Leap year with the last day of the month
	timestamp = "day -1";
	tm_ref.tm_year = 2016 - 1900; // leap year
	tm_ref.tm_mon = 2 - 1;

	expectedBegin = boost::posix_time::ptime(boost::gregorian::date(2016, 2, 29), boost::posix_time::time_duration(0, 0, 0));

	expectedEnd = boost::posix_time::ptime(boost::gregorian::date(2016, 3, 1), boost::posix_time::time_duration(0, 0, 0));

	// Run Tests
	LegacyTimePeriod::ParseTimeSpec("day -1", &tm_beg, &tm_end, &tm_ref);

	// Compare times
	begin = boost::posix_time::ptime_from_tm(tm_beg);
	end = boost::posix_time::ptime_from_tm(tm_end);

	BOOST_CHECK_EQUAL(begin, expectedBegin);
	BOOST_CHECK_EQUAL(end, expectedEnd);
}

struct DateTime
{
	struct {
		int Year, Month, Day;
	} Date;
	struct {
		int Hour, Minute, Second;
	} Time;
};

static inline
void AdvancedHelper(const char *timestamp, DateTime from, DateTime to)
{
	using boost::gregorian::date;
	using boost::posix_time::ptime;
	using boost::posix_time::ptime_from_tm;
	using boost::posix_time::time_duration;

	tm tm_beg, tm_end, tm_ref;

	tm_ref.tm_year = from.Date.Year - 1900;
	tm_ref.tm_mon = from.Date.Month - 1;
	tm_ref.tm_mday = from.Date.Day;

	// Run test
	LegacyTimePeriod::ProcessTimeRangeRaw(timestamp, &tm_ref, &tm_beg, &tm_end);

	// Compare times
	BOOST_CHECK_EQUAL(ptime_from_tm(tm_beg), ptime(date(from.Date.Year, from.Date.Month, from.Date.Day), time_duration(from.Time.Hour, from.Time.Minute, from.Time.Second)));
	BOOST_CHECK_EQUAL(ptime_from_tm(tm_end), ptime(date(to.Date.Year, to.Date.Month, to.Date.Day), time_duration(to.Time.Hour, to.Time.Minute, to.Time.Second)));
}

BOOST_AUTO_TEST_CASE(advanced)
{
	tm tm_beg, tm_end, tm_ref;
	String timestamp;
	boost::posix_time::ptime begin;
	boost::posix_time::ptime end;
	boost::posix_time::ptime expectedBegin;
	boost::posix_time::ptime expectedEnd;

	//-----------------------------------------------------
	// 2019-05-06 where Icinga celebrates 10 years #monitoringlove
	// 2019-05-06 22:00:00 - 2019-05-07 06:00:00
	AdvancedHelper("22:00-06:00", {{2019, 5, 6}, {22, 0, 0}}, {{2019, 5, 7}, {6, 0, 0}});
	AdvancedHelper("22:00:01-06:00", {{2019, 5, 6}, {22, 0, 1}}, {{2019, 5, 7}, {6, 0, 0}});
	AdvancedHelper("22:00-06:00:02", {{2019, 5, 6}, {22, 0, 0}}, {{2019, 5, 7}, {6, 0, 2}});
	AdvancedHelper("22:00:03-06:00:04", {{2019, 5, 6}, {22, 0, 3}}, {{2019, 5, 7}, {6, 0, 4}});

	//-----------------------------------------------------
	// 2019-05-06 Icinga is unleashed.
	// 09:00:00 - 17:00:00
	AdvancedHelper("09:00-17:00", {{2009, 5, 6}, {9, 0, 0}}, {{2009, 5, 6}, {17, 0, 0}});
	AdvancedHelper("09:00:01-17:00", {{2009, 5, 6}, {9, 0, 1}}, {{2009, 5, 6}, {17, 0, 0}});
	AdvancedHelper("09:00-17:00:02", {{2009, 5, 6}, {9, 0, 0}}, {{2009, 5, 6}, {17, 0, 2}});
	AdvancedHelper("09:00:03-17:00:04", {{2009, 5, 6}, {9, 0, 3}}, {{2009, 5, 6}, {17, 0, 4}});

	//-----------------------------------------------------
	// At our first Icinga Camp in SFO 2014 at GitHub HQ, we partied all night long with an overflow.
	// 2014-09-24 09:00:00 - 2014-09-25 06:00:00
	AdvancedHelper("09:00-30:00", {{2014, 9, 24}, {9, 0, 0}}, {{2014, 9, 25}, {6, 0, 0}});
	AdvancedHelper("09:00:01-30:00", {{2014, 9, 24}, {9, 0, 1}}, {{2014, 9, 25}, {6, 0, 0}});
	AdvancedHelper("09:00-30:00:02", {{2014, 9, 24}, {9, 0, 0}}, {{2014, 9, 25}, {6, 0, 2}});
	AdvancedHelper("09:00:03-30:00:04", {{2014, 9, 24}, {9, 0, 3}}, {{2014, 9, 25}, {6, 0, 4}});
}

BOOST_AUTO_TEST_SUITE_END()
