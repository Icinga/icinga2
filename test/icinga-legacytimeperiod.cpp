/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/utility.hpp"
#include "icinga/legacytimeperiod.hpp"
#include "test/utils.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/date_time/gregorian/conversion.hpp>
#include <boost/date_time/date.hpp>
#include <boost/optional.hpp>
#include <iomanip>
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(icinga_legacytimeperiod);

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

BOOST_AUTO_TEST_CASE(is_in_range)
{
	tm tm_beg = Utility::LocalTime(1706518800); // 2024-01-29 09:00:00 UTC
	tm tm_end = Utility::LocalTime(1706520600); // 2024-01-29 09:30:00 UTC

	tm reference = tm_beg; // 2024-01-29 09:00:00 UTC

	// The start date of the range should ofcourse be inside.
	BOOST_CHECK_EQUAL(true, LegacyTimePeriod::IsInTimeRange(&tm_beg, &tm_end, 1, &reference));

	reference = Utility::LocalTime(1706519400); // 2024-01-29 09:10:00 UTC
	// The reference time is only 10 minutes behind the start date, which should be covered by this range.
	BOOST_CHECK_EQUAL(true, LegacyTimePeriod::IsInTimeRange(&tm_beg, &tm_end, 1, &reference));

	reference = Utility::LocalTime(1706518799); // 2024-01-29 08:59:59 UTC

	// The reference time is 1 second ahead of the range start date, which shouldn't be covered by this range.
	BOOST_CHECK_EQUAL(false, LegacyTimePeriod::IsInTimeRange(&tm_beg, &tm_end, 1, &reference));

	reference = Utility::LocalTime(1706520599); // 2024-01-29 09:29:59 UTC

	// The reference time is 1 second before the specified end time, so this should be in the range.
	BOOST_CHECK_EQUAL(true, LegacyTimePeriod::IsInTimeRange(&tm_beg, &tm_end, 1, &reference));

	reference = tm_end; // 2024-01-29 09:30:00 UTC

	// The reference time is exactly the same as the specified end time, so this should definitely not be in the range.
	BOOST_CHECK_EQUAL(false, LegacyTimePeriod::IsInTimeRange(&tm_beg, &tm_end, 1, &reference));

	tm_beg = Utility::LocalTime(1706518800); // 2024-01-29 09:00:00 UTC
	tm_end = Utility::LocalTime(1706720400); // 2024-01-31 17:00:00 UTC

	reference = Utility::LocalTime(1706612400); // 2024-01-30 12:00:00 UTC

	// Even if the reference time is within the specified range, the stride guarantees that the reference
	// should be 2 days after the range start date, which is not the case.
	BOOST_CHECK_EQUAL(false, LegacyTimePeriod::IsInTimeRange(&tm_beg, &tm_end, 2, &reference));

	reference = Utility::LocalTime(1706698800); // 2024-01-31 11:00:00 UTC

	// The reference time is now within the specified range and 2 days after the range start date.
	BOOST_CHECK_EQUAL(true, LegacyTimePeriod::IsInTimeRange(&tm_beg, &tm_end, 2, &reference));
}

BOOST_AUTO_TEST_CASE(out_of_range_segments)
{
	TimePeriod::Ptr tp = new TimePeriod();
	tp->SetUpdate(new Function("LegacyTimePeriod", LegacyTimePeriod::ScriptFunc, {"tp", "begin", "end"}), true);

	// A single day range shouldn't span to the following day too (see https://github.com/Icinga/icinga2/issues/9388).
	tp->SetRanges(new Dictionary({{"2024-06-12", "00:00-24:00"}}), true);
	tp->UpdateRegion(1718150400, 1718236800, true);  // 2024-06-12 00:00:00 - 24:00:00 UTC

	BOOST_CHECK_EQUAL(true, tp->IsInside(1718200800)); // 2024-06-12 14:00:00 UTC
	{
		Array::Ptr segments = tp->GetSegments();
		BOOST_REQUIRE_EQUAL(1, segments->GetLength());

		Dictionary::Ptr segment = segments->Get(0);
		BOOST_CHECK_EQUAL(1718150400, segment->Get("begin")); // 2024-06-12 00:00:00 UTC
		BOOST_CHECK_EQUAL(1718236800, segment->Get("end")); // 2024-06-12 24:00:00 UTC
	}
	tp->UpdateRegion(1718236800, 1718323200, true);  // 2024-06-13 00:00:00 - 24:00:00 UTC

	BOOST_CHECK_EQUAL(false, tp->IsInside(1718287200)); // 2024-06-13 14:00:00 UTC
	{
		Array::Ptr segments = tp->GetSegments();
		BOOST_CHECK_EQUAL(0, segments->GetLength()); // There should be no segments at all!
	}

	// One partially day range shouldn't contain more than a single segment (see https://github.com/Icinga/icinga2/issues/9781).
	tp->SetRanges(new Dictionary({{"2024-06-12", "10:00-12:00"}}), true);
	tp->UpdateRegion(1718150400, 1718236800, true);  // 2024-06-12 00:00:00 - 24:00:00 UTC

	BOOST_CHECK_EQUAL(true, tp->IsInside(1718190000)); // 2024-06-12 11:00:00 UTC
	{
		Array::Ptr segments = tp->GetSegments();
		BOOST_REQUIRE_EQUAL(1, segments->GetLength());

		Dictionary::Ptr segment = segments->Get(0);
		BOOST_CHECK_EQUAL(1718186400, segment->Get("begin")); // 2024-06-12 10:00:00 UTC (range start date)
		BOOST_CHECK_EQUAL(1718193600, segment->Get("end")); // 2024-06-12 12:00:00 UTC (range end date)
	}
	tp->UpdateRegion(1718236800, 1718323200, true);  // 2024-06-13 00:00:00 - 24:00:00 UTC

	BOOST_CHECK_EQUAL(false, tp->IsInside(1718287200)); // 2024-06-13 14:00:00 UTC
	BOOST_CHECK_EQUAL(0, tp->GetSegments()->GetLength()); // There should be no segments at all!
}

BOOST_AUTO_TEST_CASE(include_exclude_timeperiods)
{
	Function::Ptr update = new Function("LegacyTimePeriod", LegacyTimePeriod::ScriptFunc, {"tp", "begin", "end"});
	TimePeriod::Ptr excludedTp = new TimePeriod();
	excludedTp->SetName("excluded", true);
	excludedTp->SetUpdate(update, true);
	excludedTp->SetRanges(new Dictionary({{"2024-06-11", "00:00-24:00"}}), true);

	excludedTp->UpdateRegion(1718064000, 1718323200, true);  // 2024-06-11 00:00:00 - 2024-06-13 24:00:00 UTC

	BOOST_CHECK_EQUAL(1, excludedTp->GetSegments()->GetLength());
	BOOST_CHECK_EQUAL(true, excludedTp->IsInside(1718114400)); // 2024-06-11 14:00:00 UTC
	BOOST_CHECK_EQUAL(false, excludedTp->IsInside(1718200800)); // 2024-06-12 14:00:00 UTC
	BOOST_CHECK_EQUAL(false, excludedTp->IsInside(1718287200)); // 2024-06-13 14:00:00 UTC

	// Register the excluded timeperiod to make it globally visible.
	excludedTp->Register();

	Dictionary::Ptr ranges = new Dictionary({
		{"2024-06-11", "09:00-17:00"},
		{"2024-06-12", "09:00-17:00"},
		{"2024-06-13", "09:00-17:00"}
	});

	TimePeriod::Ptr tp = new TimePeriod();
	tp->SetExcludes(new Array({"excluded"}), true);
	tp->SetUpdate(update, true);
	tp->SetRanges(ranges, true);
	tp->UpdateRegion(1718064000, 1718323200, true);  // 2024-06-11 00:00:00 - 2024-06-13 24:00:00 UTC

	BOOST_CHECK_EQUAL(false, tp->IsInside(1718114400)); // 2024-06-11 14:00:00 UTC
	BOOST_CHECK_EQUAL(false, tp->IsInside(1718150400)); // 2024-06-12 00:00:00 UTC
	BOOST_CHECK_EQUAL(true, tp->IsInside(1718200800)); // 2024-06-12 14:00:00 UTC
	BOOST_CHECK_EQUAL(false, tp->IsInside(1718323200)); // 2024-06-13 00:00:00 UTC
	BOOST_CHECK_EQUAL(true, tp->IsInside(1718287200)); // 2024-06-13 14:00:00 UTC
	{
		Array::Ptr segments = tp->GetSegments();
		// The updated region is 2024-06-11 - 13, so there should only be 2 segements, when the excludes works correctly.
		BOOST_REQUIRE_EQUAL(2, segments->GetLength());

		Dictionary::Ptr segment = segments->Get(0);
		BOOST_CHECK_EQUAL(1718182800, segment->Get("begin")); // 2024-06-12 09:00:00 UTC
		BOOST_CHECK_EQUAL(1718211600, segment->Get("end")); // 2024-06-12 17:00:00 UTC

		BOOST_CHECK_EQUAL(true, tp->IsInside(segment->Get("begin")));
		BOOST_CHECK_EQUAL(false, tp->IsInside(segment->Get("end")));

		BOOST_CHECK_EQUAL(false, excludedTp->IsInside(segment->Get("begin")));
		BOOST_CHECK_EQUAL(false, excludedTp->IsInside(segment->Get("end")));

		segment = segments->Get(1);
		BOOST_CHECK_EQUAL(1718269200, segment->Get("begin")); // 2024-06-13 09:00:00 UTC
		BOOST_CHECK_EQUAL(1718298000, segment->Get("end")); // 2024-06-13 17:00:00 UTC

		BOOST_CHECK_EQUAL(true, tp->IsInside(segment->Get("begin")));
		BOOST_CHECK_EQUAL(false, tp->IsInside(segment->Get("end")));

		BOOST_CHECK_EQUAL(false, excludedTp->IsInside(segment->Get("begin")));
		BOOST_CHECK_EQUAL(false, excludedTp->IsInside(segment->Get("end")));
	}

	// Include timeperiod test cases ...
	TimePeriod::Ptr includedTp = new TimePeriod();
	includedTp->SetName("included", true);
	includedTp->SetUpdate(update, true);
	includedTp->SetRanges(new Dictionary({{"2024-06-11", "08:00-17:00"}}), true);

	includedTp->UpdateRegion(1718064000, 1718323200, true);  // 2024-06-11 00:00:00 - 2024-06-13 24:00:00 UTC

	BOOST_CHECK_EQUAL(1, includedTp->GetSegments()->GetLength());
	BOOST_CHECK_EQUAL(true, includedTp->IsInside(1718114400)); // 2024-06-11 14:00:00 UTC
	BOOST_CHECK_EQUAL(false, includedTp->IsInside(1718200800)); // 2024-06-12 14:00:00 UTC
	BOOST_CHECK_EQUAL(false, includedTp->IsInside(1718287200)); // 2024-06-13 14:00:00 UTC

	// Register the timeperiod to make it globally visible.
	includedTp->Register();

	tp->SetIncludes(new Array({"included"}), true);
	tp->UpdateRegion(1718064000, 1718323200, true);  // 2024-06-11 00:00:00 - 2024-06-13 24:00:00 UTC
	{
		Array::Ptr segments = tp->GetSegments();
		// The updated region is 2024-06-11 - 13, so there should be 3 segements, when the *prefer* includes works correctly.
		BOOST_REQUIRE_EQUAL(3, segments->GetLength());

		Dictionary::Ptr segment = segments->Get(0);
		BOOST_CHECK_EQUAL(1718182800, segment->Get("begin")); // 2024-06-12 09:00:00 UTC
		BOOST_CHECK_EQUAL(1718211600, segment->Get("end")); // 2024-06-12 17:00:00 UTC

		segment = segments->Get(1);
		BOOST_CHECK_EQUAL(1718269200, segment->Get("begin")); // 2024-06-13 09:00:00 UTC
		BOOST_CHECK_EQUAL(1718298000, segment->Get("end")); // 2024-06-13 17:00:00 UTC

		segment = segments->Get(2);
		BOOST_CHECK_EQUAL(1718092800, segment->Get("begin")); // 2024-06-11 08:00:00 UTC
		BOOST_CHECK_EQUAL(1718125200, segment->Get("end")); // 2024-06-11 17:00:00 UTC

		BOOST_CHECK_EQUAL(true, tp->IsInside(segment->Get("begin")));
		BOOST_CHECK_EQUAL(true, includedTp->IsInside(segment->Get("begin")));
		BOOST_CHECK_EQUAL(false, tp->IsInside(segment->Get("end")));
		BOOST_CHECK_EQUAL(false, includedTp->IsInside(segment->Get("end")));
	}
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

time_t make_time_t(const tm* t)
{
	tm copy = *t;
	return mktime(&copy);
}

time_t make_time_t(std::string s)
{
	tm t = make_tm(s);
	return mktime(&t);
}

struct Segment
{
	time_t begin, end;

	Segment(time_t begin, time_t end) : begin(begin), end(end) {}
	Segment(std::string begin, std::string end) : begin(make_time_t(begin)), end(make_time_t(end)) {}

	bool operator==(const Segment& o) const
	{
		return o.begin == begin && o.end == end;
	}
};

std::string pretty_time(const tm& t)
{
	return Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %Z", &t);
}

std::string pretty_time(time_t t)
{
	return pretty_time(Utility::LocalTime(t));
}

std::ostream& operator<<(std::ostream& o, const Segment& s)
{
	return o << "(" << pretty_time(s.begin) << " (" << s.begin << ") .. " << pretty_time(s.end) << " (" << s.end << "))";
}

std::ostream& operator<<(std::ostream& o, const boost::optional<Segment>& s)
{
	if (s) {
		return o << *s;
	} else {
		return o << "none";
	}
}

BOOST_AUTO_TEST_CASE(dst)
{
	GlobalTimezoneFixture tz(GlobalTimezoneFixture::TestTimezoneWithDST);

	// Self-tests for helper functions
	BOOST_CHECK_EQUAL(make_tm("2021-11-07 02:30:00").tm_isdst, -1);
	BOOST_CHECK_EQUAL(make_tm("2021-11-07 02:30:00 PST").tm_isdst, 0);
	BOOST_CHECK_EQUAL(make_tm("2021-11-07 02:30:00 PDT").tm_isdst, 1);
	BOOST_CHECK_EQUAL(make_time_t("2021-11-07 01:30:00 PST"), 1636277400); // date -d '2021-11-07 01:30:00 PST' +%s
	BOOST_CHECK_EQUAL(make_time_t("2021-11-07 01:30:00 PDT"), 1636273800); // date -d '2021-11-07 01:30:00 PDT' +%s

	struct TestData {
		std::string day;
		std::string ranges;
		std::vector<tm> before;
		std::vector<tm> during;
		boost::optional<Segment> expected;
	};

	// Some of the following test cases have comments describing the current behavior. This might not necessarily be the
	// best possible behavior, especially that it differs on Windows. So it might be perfectly valid to change this.
	// These cases are just there to actually notice these changes in this case.
	std::vector<TestData> tests;

	// 2021-03-14: 01:59:59 PST (UTC-8) -> 03:00:00 PDT (UTC-7)
	for (const std::string& day : {"2021-03-14", "sunday", "sunday 2", "sunday -3"}) {
		// range before DST change
		tests.push_back(TestData{
			day, "00:30-01:30",
			{make_tm("2021-03-14 00:00:00 PST")},
			{make_tm("2021-03-14 01:00:00 PST")},
			Segment("2021-03-14 00:30:00 PST", "2021-03-14 01:30:00 PST"),
		});

		if (day.find("sunday") == std::string::npos) { // skip for non-absolute day specs (would find another sunday)
			// range end actually does not exist on that day
			tests.push_back(TestData{
				day, "01:30-02:30",
				{make_tm("2021-03-14 01:00:00 PST")},
				{make_tm("2021-03-14 01:59:59 PST")},
#ifndef _WIN32
				// As 02:30 does not exist on this day, it is parsed as if it was 02:30 PST which is actually 03:30 PDT.
				Segment("2021-03-14 01:30:00 PST", "2021-03-14 03:30:00 PDT"),
#else
				// Windows interpretes 02:30 as 01:30 PST, so it is an empty segment.
				boost::none,
#endif
			});
		}

		if (day.find("sunday") == std::string::npos) { // skip for non-absolute day specs (would find another sunday)
			// range beginning does not actually exist on that day
			tests.push_back(TestData{
				day, "02:30-03:30",
				{make_tm("2021-03-14 01:00:00 PST")},
				{make_tm("2021-03-14 03:00:00 PDT")},
#ifndef _WIN32
				// As 02:30 does not exist on this day, it is parsed as if it was 02:30 PST which is actually 03:30 PDT.
				// Therefore, the result is a segment from 03:30 PDT to 03:30 PDT with a duration of 0, i.e. no segment.
				boost::none,
#else
				// Windows parses non-existing 02:30 as 01:30 PST, resulting in an 1 hour segment.
				Segment("2021-03-14 01:30:00 PST", "2021-03-14 03:30:00 PDT"),
#endif
			});
		}

		// another range where the beginning does not actually exist on that day
		tests.push_back(TestData{
			day, "02:15-03:45",
			{make_tm("2021-03-14 01:00:00 PST")},
			{make_tm("2021-03-14 03:30:00 PDT")},
#ifndef _WIN32
			// As 02:15 does not exist on this day, it is parsed as if it was 02:15 PST which is actually 03:15 PDT.
			Segment("2021-03-14 03:15:00 PDT", "2021-03-14 03:45:00 PDT"),
#else
			// Windows interprets 02:15 as 01:15 PST though.
			Segment("2021-03-14 01:15:00 PST", "2021-03-14 03:45:00 PDT"),
#endif
		});

		// range after DST change
		tests.push_back(TestData{
			day, "03:30-04:30",
			{make_tm("2021-03-14 01:00:00 PST"), make_tm("2021-03-14 03:00:00 PDT")},
			{make_tm("2021-03-14 04:00:00 PDT")},
			Segment("2021-03-14 03:30:00 PDT", "2021-03-14 04:30:00 PDT"),
		});

		// range containing DST change
		tests.push_back(TestData{
			day, "01:30-03:30",
			{make_tm("2021-03-14 01:00:00 PST")},
			{make_tm("2021-03-14 01:45:00 PST"), make_tm("2021-03-14 03:15:00 PDT")},
			Segment("2021-03-14 01:30:00 PST", "2021-03-14 03:30:00 PDT"),
		});
	}

	// 2021-11-07: 01:59:59 PDT (UTC-7) -> 01:00:00 PST (UTC-8)
	for (const std::string& day : {"2021-11-07", "sunday", "sunday 1", "sunday -4"}) {
		// range before DST change
		tests.push_back(TestData{
			day, "00:15-00:45",
			{make_tm("2021-11-07 00:00:00 PDT")},
			{make_tm("2021-11-07 00:30:00 PDT")},
			Segment("2021-11-07 00:15:00 PDT", "2021-11-07 00:45:00 PDT"),
		});

		if (day.find("sunday") == std::string::npos) { // skip for non-absolute day specs (would find another sunday)
			// range existing twice during DST change (first instance)
#ifndef _WIN32
			tests.push_back(TestData{
				day, "01:15-01:45",
				{make_tm("2021-11-07 01:00:00 PDT")},
				{make_tm("2021-11-07 01:30:00 PDT")},
				// Duplicate times are interpreted as the first occurrence.
				Segment("2021-11-07 01:15:00 PDT", "2021-11-07 01:45:00 PDT"),
			});
#else
			tests.push_back(TestData{
				day, "01:15-01:45",
				{make_tm("2021-11-07 01:00:00 PDT")},
				{make_tm("2021-11-07 01:30:00 PST")},
				// However, Windows always uses the second occurrence.
				Segment("2021-11-07 01:15:00 PST", "2021-11-07 01:45:00 PST"),
			});
#endif
		}

		if (day.find("sunday") == std::string::npos) { // skip for non-absolute day specs (would find another sunday)
			// range existing twice during DST change (second instance)
			tests.push_back(TestData{
				day, "01:15-01:45",
				{make_tm("2021-11-07 01:00:00 PST")},
				{make_tm("2021-11-07 01:30:00 PST")},
#ifndef _WIN32
				// Interpreted as the first occurrence, so it's in the past.
				boost::none,
#else
				// On Windows, it's the second occurrence, so it's still in the present/future and is found.
				Segment("2021-11-07 01:15:00 PST", "2021-11-07 01:45:00 PST"),
#endif
			});
		}

		// range after DST change
		tests.push_back(TestData{
			day, "03:30-04:30",
			{make_tm("2021-11-07 01:00:00 PDT"), make_tm("2021-11-07 03:00:00 PST")},
			{make_tm("2021-11-07 04:00:00 PST")},
			Segment("2021-11-07 03:30:00 PST", "2021-11-07 04:30:00 PST"),
		});

		// range containing DST change
		tests.push_back(TestData{
			day, "00:30-02:30",
			{make_tm("2021-11-07 00:00:00 PDT")},
			{make_tm("2021-11-07 00:45:00 PDT"), make_tm("2021-11-07 01:30:00 PDT"),
			make_tm("2021-11-07 01:30:00 PST"), make_tm("2021-11-07 02:15:00 PST")},
			Segment("2021-11-07 00:30:00 PDT", "2021-11-07 02:30:00 PST"),
		});

		// range ending during duplicate DST hour (first instance)
		tests.push_back(TestData{
			day, "00:30-01:30",
			{make_tm("2021-11-07 00:00:00 PDT")},
			{make_tm("2021-11-07 01:00:00 PDT")},
#ifndef _WIN32
			// Both times are interpreted as the first instance on that day (i.e both PDT).
			Segment("2021-11-07 00:30:00 PDT", "2021-11-07 01:30:00 PDT")
#else
			// Windows interprets duplicate times as the second instance (i.e. both PST).
			Segment("2021-11-07 00:30:00 PDT", "2021-11-07 01:30:00 PST")
#endif
		});

		// range beginning during duplicate DST hour (first instance)
		tests.push_back(TestData{
			day, "01:30-02:30",
			{make_tm("2021-11-07 01:00:00 PDT")},
			{make_tm("2021-11-07 02:00:00 PST")},
#ifndef _WIN32
			// 01:30 is interpreted as the first occurrence (PDT) but since there's no 02:30 PDT, it's PST.
			Segment("2021-11-07 01:30:00 PDT", "2021-11-07 02:30:00 PST")
#else
			// Windows interprets both as PST though.
			Segment("2021-11-07 01:30:00 PST", "2021-11-07 02:30:00 PST")
#endif
		});

		if (day.find("sunday") == std::string::npos) { // skip for non-absolute day specs (would find another sunday)
			// range ending during duplicate DST hour (second instance)
#ifndef _WIN32
			tests.push_back(TestData{
				day, "00:30-01:30",
				{make_tm("2021-11-07 00:00:00 PST")},
				{make_tm("2021-11-07 01:00:00 PST")},
				// Both times are parsed as PDT. Thus, 00:00 PST (01:00 PDT) is during the segment and
				// 01:00 PST (02:00 PDT) is after the segment.
				boost::none,
			});
#else
			tests.push_back(TestData{
				day, "00:30-01:30",
				{make_tm("2021-11-07 00:00:00 PDT")},
				{make_tm("2021-11-07 01:00:00 PST")},
				// As Windows interprets the end as PST, it's still in the future and the segment is found.
				Segment("2021-11-07 00:30:00 PDT", "2021-11-07 01:30:00 PST"),
			});
#endif
		}

		// range beginning during duplicate DST hour (second instance)
		tests.push_back(TestData{
			day, "01:30-02:30",
			{make_tm("2021-11-07 01:00:00 PDT")},
			{make_tm("2021-11-07 02:00:00 PST")},
#ifndef _WIN32
			// As 01:30 always refers to the first occurrence (PDT), this is actually a 2 hour segment.
			Segment("2021-11-07 01:30:00 PDT", "2021-11-07 02:30:00 PST"),
#else
			// On Windows, it refers t the second occurrence (PST), therefore it's an 1 hour segment.
			Segment("2021-11-07 01:30:00 PST", "2021-11-07 02:30:00 PST"),
#endif
		});
	}

	auto seg = [](const Dictionary::Ptr& segment) -> boost::optional<Segment> {
		if (segment == nullptr) {
			return boost::none;
		}

		BOOST_CHECK(segment->Contains("begin"));
		BOOST_CHECK(segment->Contains("end"));

		return Segment{time_t(segment->Get("begin")), time_t(segment->Get("end"))};
	};

	for (const TestData& t : tests) {
		for (const tm& ref : t.during) {
			if (t.expected) {
				// test data sanity check
				time_t ref_ts = make_time_t(&ref);
				BOOST_CHECK_MESSAGE(t.expected->begin < ref_ts, "[day='" << t.day << "' ranges='" << t.ranges
					<< "'] expected.begin='"<< pretty_time(t.expected->begin) << "' < ref='" << pretty_time(ref_ts)
					<< "' violated");
				BOOST_CHECK_MESSAGE(ref_ts < t.expected->end, "[day='" << t.day << "' ranges='" << t.ranges
					<< "'] ref='" << pretty_time(ref_ts) << "' < expected.end='" << pretty_time(t.expected->end)
					<< "' violated");
			}

			tm mutRef = ref;
			auto runningSeg = seg(LegacyTimePeriod::FindRunningSegment(t.day, t.ranges, &mutRef));
			BOOST_CHECK_MESSAGE(runningSeg == t.expected, "FindRunningSegment(day='" << t.day
				<< "' ranges='" << t.ranges << "' ref='" << pretty_time(ref) << "'): got=" << runningSeg
				<< " expected=" << t.expected);
		}

		for (const tm& ref : t.before) {
			if (t.expected) {
				// test data sanity check
				time_t ref_ts = make_time_t(&ref);
				BOOST_CHECK_MESSAGE(ref_ts < t.expected->begin, "[day='" << t.day << "' ranges='" << t.ranges
					<< "'] ref='"<< pretty_time(ref_ts) << "' < expected.begin='" << pretty_time(t.expected->begin)
					<< "' violated");
				BOOST_CHECK_MESSAGE(t.expected->begin < t.expected->end, "[day='" << t.day << "' ranges='" << t.ranges
					<< "'] expected.begin='" << pretty_time(t.expected->begin)
					<< "' < expected.end='" << pretty_time(t.expected->end) << "' violated");
			}

			tm mutRef = ref;
			auto nextSeg = seg(LegacyTimePeriod::FindNextSegment(t.day, t.ranges, &mutRef));
			BOOST_CHECK_MESSAGE(nextSeg == t.expected, "FindNextSegment(day='" << t.day << "' ranges='" << t.ranges
				<< "' ref='" << pretty_time(ref) << "'): got=" << nextSeg << " expected=" << t.expected);
		}
	}
}

// This tests checks that TimePeriod::IsInside() always returns true for a 24x7 period, even around DST changes.
BOOST_AUTO_TEST_CASE(dst_isinside)
{
	GlobalTimezoneFixture tz(GlobalTimezoneFixture::TestTimezoneWithDST);

	Function::Ptr update = new Function("LegacyTimePeriod", LegacyTimePeriod::ScriptFunc, {"tp", "begin", "end"});
	Dictionary::Ptr ranges = new Dictionary({
		{"monday",    "00:00-24:00"},
		{"tuesday",   "00:00-24:00"},
		{"wednesday", "00:00-24:00"},
		{"thursday",  "00:00-24:00"},
		{"friday",    "00:00-24:00"},
		{"saturday",  "00:00-24:00"},
		{"sunday",    "00:00-24:00"},
	});

	// Vary begin from Sat 06 Nov 2021 00:00:00 PDT to Mon 08 Nov 2021 00:00:00 PST in 30 minute intervals.
	for (time_t t_begin = 1636182000; t_begin <= 1636358400; t_begin += 30*60) {
		// Test varying interval lengths: 60 minutes, 24 hours, 48 hours.
		for (time_t len : {60*60, 24*60*60, 48*60*60}) {
			time_t t_end = t_begin + len;

			TimePeriod::Ptr p = new TimePeriod();
			p->SetUpdate(update, true);
			p->SetRanges(ranges, true);

			p->UpdateRegion(double(t_begin), double(t_end), true);

			{
				// Print resulting segments for easier debugging.
				Array::Ptr segments = p->GetSegments();
				ObjectLock lock(segments);
				for (Dictionary::Ptr segment: segments) {
					BOOST_TEST_MESSAGE("t_begin=" << t_begin << " t_end=" << t_end
						<< " segment.begin=" << segment->Get("begin") << " segment.end=" << segment->Get("end"));
				}
			}

			time_t step = 10*60;
			for (time_t t = t_begin+step; t < t_end; t += step) {
				BOOST_CHECK_MESSAGE(p->IsInside(double(t)),
					t << " should be inside for t_begin=" << t_begin << " t_end=" << t_end);
			}
		}
	}
}

BOOST_AUTO_TEST_CASE(find_nth_weekday) {
	auto run = [](const std::string& refDay, int wday, int n, const std::string& expectedDay) {
		tm expected = make_tm(expectedDay + " 00:00:00");

		tm t = make_tm(refDay + " 00:00:00");
		LegacyTimePeriod::FindNthWeekday(wday, n, &t);

		BOOST_CHECK_MESSAGE(mktime(&expected) == mktime(&t),
			"[ref=" << refDay << ", wday=" << wday << ", n=" << n << "] "
			"expected: " << pretty_time(expected) << ", "
			"got: " << pretty_time(t));
	};

	/*      March 2019
	 * Mo Tu We Th Fr Sa Su
	 *              1  2  3
	 *  4  5  6  7  8  9 10
	 * 11 12 13 14 15 16 17
	 * 18 19 20 21 22 23 24
	 * 25 26 27 28 29 30 31
	 */

	// Use every day of the month as reference day, all must give the same result for that month.
	for (int i = 1; i <= 31; ++i) {
		std::stringstream refDayStream;
		refDayStream << "2019-03-" << std::setw(2) << std::setfill('0') << i;
		std::string refDay = refDayStream.str();

		const int monday = 1; // 4 ocurrences in March 2019
		run(refDay, monday, 1, "2019-03-04");
		run(refDay, monday, 2, "2019-03-11");
		run(refDay, monday, 3, "2019-03-18");
		run(refDay, monday, 4, "2019-03-25");
		run(refDay, monday, -1, "2019-03-25");
		run(refDay, monday, -2, "2019-03-18");
		run(refDay, monday, -3, "2019-03-11");
		run(refDay, monday, -4, "2019-03-04");

		const int friday = 5; // 5 ocurrences in March 2019
		run(refDay, friday, 1, "2019-03-01");
		run(refDay, friday, 2, "2019-03-08");
		run(refDay, friday, 3, "2019-03-15");
		run(refDay, friday, 4, "2019-03-22");
		run(refDay, friday, 5, "2019-03-29");
		run(refDay, friday, -1, "2019-03-29");
		run(refDay, friday, -2, "2019-03-22");
		run(refDay, friday, -3, "2019-03-15");
		run(refDay, friday, -4, "2019-03-08");
		run(refDay, friday, -5, "2019-03-01");
	}
}

BOOST_AUTO_TEST_SUITE_END()
