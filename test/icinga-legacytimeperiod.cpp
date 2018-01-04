/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

BOOST_AUTO_TEST_SUITE_END()
