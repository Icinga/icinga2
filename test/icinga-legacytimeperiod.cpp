/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/utility.hpp"
#include "icinga/legacytimeperiod.hpp"
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
	tm beg, end, ref;

	// check parsing of "YYYY-MM-DD" specs
	LegacyTimePeriod::ParseTimeSpec("2016-01-01", &beg, &end, &ref);
	BOOST_CHECK_EQUAL(mktime(&beg), (time_t) 1451606400);
	BOOST_CHECK_EQUAL(mktime(&end), (time_t) 1451692800);

	LegacyTimePeriod::ParseTimeSpec("2015-12-31", &beg, &end, &ref);
	BOOST_CHECK_EQUAL(mktime(&beg), (time_t) 1451520000);
	BOOST_CHECK_EQUAL(mktime(&end), (time_t) 1451606400);

	BOOST_CHECK_THROW(LegacyTimePeriod::ParseTimeSpec("2015-12-32", &beg, &end, &ref),
	    std::invalid_argument);

	BOOST_CHECK_THROW(LegacyTimePeriod::ParseTimeSpec("2015-28-01", &beg, &end, &ref),
	    std::invalid_argument);

	// check parsing of "day X" and "day -X" specs
	ref.tm_year = 2016 - 1900;
	ref.tm_mon = 1;
	LegacyTimePeriod::ParseTimeSpec("day 2", &beg, &end, &ref);
	BOOST_CHECK_EQUAL(mktime(&beg), (time_t) 1454371200); // 2016-02-02
	BOOST_CHECK_EQUAL(mktime(&end), (time_t) 1454457600); // 2016-02-03

	ref.tm_year = 2018 - 1900;
	ref.tm_mon = 11;
	LegacyTimePeriod::ParseTimeSpec("day 31", &beg, &end, &ref);
	BOOST_CHECK_EQUAL(mktime(&beg), (time_t) 1546214400); // 2018-12-31
	BOOST_CHECK_EQUAL(mktime(&end), (time_t) 1546300800); // 2019-01-01

	ref.tm_year = 2012 - 1900;
	ref.tm_mon = 6;
	LegacyTimePeriod::ParseTimeSpec("day -1", &beg, &end, &ref);
	BOOST_CHECK_EQUAL(mktime(&beg), (time_t) 1343692800); // 2012-07-31
	BOOST_CHECK_EQUAL(mktime(&end), (time_t) 1343779200); // 2012-08-01

	ref.tm_year = 2016 - 1900; // leap year
	ref.tm_mon = 1;
	LegacyTimePeriod::ParseTimeSpec("day -1", &beg, &end, &ref);
	BOOST_CHECK_EQUAL(mktime(&beg), (time_t) 1456704000); // 2016-02-29
	BOOST_CHECK_EQUAL(mktime(&end), (time_t) 1456790400); // 2016-03-01
}

BOOST_AUTO_TEST_CASE(advanced) {
	tm beg, end, ref;
	time_t ts_beg, ts_end, ts_beg_exp, ts_end_exp;

	//2019-05-06 where Icinga celebrates 10 years #monitoringlove
	String timestamp = "22:00-06:00";
	ref.tm_year = 2019 - 1900;
	ref.tm_mon = 5 - 1;
	ref.tm_mday = 6;

	LegacyTimePeriod::ProcessTimeRangeRaw(timestamp, &ref, &beg, &end);
	ts_beg = mktime(&beg);
	ts_end = mktime(&end);
	ts_beg_exp = 1557180000; // 2019-05-06 22:00:00
	ts_end_exp = 1557208800; // 2019-05-07 06:00:00

	BOOST_CHECK_EQUAL(ts_beg, ts_beg_exp);
	BOOST_TEST_MESSAGE("Begin date: " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", ts_beg) << " expected " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", ts_beg_exp));
	BOOST_CHECK_EQUAL(ts_end, ts_end_exp);
	BOOST_TEST_MESSAGE("End date: " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", ts_end) << " expected " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", ts_end_exp));


	//2019-05-06 Icinga is unleashed.
	timestamp = "09:00-17:00";
	ref.tm_year = 2009 - 1900;
	ref.tm_mon = 5 - 1;
	ref.tm_mday = 6;

	LegacyTimePeriod::ProcessTimeRangeRaw(timestamp, &ref, &beg, &end);
	ts_beg = mktime(&beg);
	ts_end = mktime(&end);
	ts_beg_exp = 1241600400; // 2009-05-06 09:00:00
	ts_end_exp = 1241629200; // 2009-05-06 17:00:00

	BOOST_CHECK_EQUAL(ts_beg, ts_beg_exp);
	BOOST_TEST_MESSAGE("Begin date: " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", ts_beg) << " expected " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", ts_beg_exp));
	BOOST_CHECK_EQUAL(ts_end, ts_end_exp);
	BOOST_TEST_MESSAGE("End date: " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", ts_end) << " expected " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", ts_end_exp));

	//At our first Icinga Camp in SFO 2014 at GitHub HQ, we partied all night long with an overflow.
	timestamp = "09:00-30:00";
	ref.tm_year = 2014 - 1900;
	ref.tm_mon = 9 - 1;
	ref.tm_mday = 25;

	LegacyTimePeriod::ProcessTimeRangeRaw(timestamp, &ref, &beg, &end);
	ts_beg = mktime(&beg);
	ts_end = mktime(&end);
	ts_beg_exp = 1411549200; // 2014-09-24 09:00:00
	ts_end_exp = 1411624800; // 2014-09-25 06:00:00

	BOOST_CHECK_EQUAL(ts_beg, ts_beg_exp);
	BOOST_TEST_MESSAGE("Begin date: " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", ts_beg) << " expected " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", ts_beg_exp));
	BOOST_CHECK_EQUAL(ts_end, ts_end_exp);
	BOOST_TEST_MESSAGE("End date: " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", ts_end) << " expected " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", ts_end_exp));

 }

BOOST_AUTO_TEST_SUITE_END()
