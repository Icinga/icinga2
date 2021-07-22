/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/utility.hpp"
#include "icinga/legacytimeperiod.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/date_time/gregorian/conversion.hpp>
#include <boost/date_time/date.hpp>
#include <boost/optional.hpp>
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(icinga_legacytimeperiod);

struct GlobalTimezoneFixture
{
	char *tz;

	GlobalTimezoneFixture(const char *fixed_tz = "")
	{
		tz = getenv("TZ");
#ifdef _WIN32
		_putenv_s("TZ", fixed_tz == "" ? "UTC" : fixed_tz);
#else
		setenv("TZ", fixed_tz, 1);
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

tm make_tm(std::string s)
{
	tm t = {};
	int dst = -1;
	size_t l = strlen("YYYY-MM-DD HH:MM:SS");
	if (s.size() > l) {
		std::string zone = s.substr(l);
		if (zone == " CET") {
			dst = 0;
		} else if (zone == " CEST") {
			dst = 1;
		} else {
			// tests should only use CET/CEST (for now)
			BOOST_CHECK_MESSAGE(false, "invalid or unknown time time: " << zone);
		}
	}
	strptime(s.c_str(), "%Y-%m-%d %H:%M:%S", &t);
	t.tm_isdst = dst;
	return t;
}

time_t make_time_t(const tm *t)
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

	bool operator==(const Segment &o) const
	{
		return o.begin == begin && o.end == end;
	}
};

std::string pretty_time(const tm& t)
{
	char buf[128];
	size_t n = strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", &t);
	return std::string(buf, n);
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
	// DST changes in Europe/Berlin:
	// 2021-03-28: 01:59:59 CET  (UTC+1) -> 03:00:00 CEST (UTC+2)
	// 2021-10-31: 02:59:59 CEST (UTC+2) -> 02:00:00 CET  (UTC+1)
	GlobalTimezoneFixture tz("Europe/Berlin");

	// self-test for helper functions
	BOOST_CHECK_EQUAL(make_tm("2021-10-31 02:30:00").tm_isdst, -1);
	BOOST_CHECK_EQUAL(make_tm("2021-10-31 02:30:00 CET").tm_isdst, 0);
	BOOST_CHECK_EQUAL(make_tm("2021-10-31 02:30:00 CEST").tm_isdst, 1);
	BOOST_CHECK_EQUAL(make_time_t("2021-10-31 02:30:00 CET"), 1635643800); // date -d '2021-10-31 02:30:00 CET' +%s
	BOOST_CHECK_EQUAL(make_time_t("2021-10-31 02:30:00 CEST"), 1635640200); // date -d '2021-10-31 02:30:00 CEST' +%s

	struct TestData {
		std::string day;
		std::string ranges;
		std::vector<tm> before;
		std::vector<tm> during;
		boost::optional<Segment> expected;
	};

	// Some of the following test cases have comments describing the current behavior. This might not necessarily be
	// the best possible behavior and it might be perfectly valid to change this. These cases are just there to actually
	// notice these changes in this case.
	std::vector<TestData> tests;

	// 2021-03-28: 01:59:59 CET (UTC+1) -> 03:00:00 CEST (UTC+2)
	for (const std::string &day : {"2021-03-28", "sunday", "sunday -1", "sunday 4"}) {
		// range before DST change
		tests.push_back(TestData{
			day, "00:30-01:30",
			{make_tm("2021-03-28 00:00:00 CET")},
			{make_tm("2021-03-28 01:00:00 CET")},
			Segment("2021-03-28 00:30:00 CET", "2021-03-28 01:30:00 CET"),
		});

		// range end actually does not exist on that day
		tests.push_back(TestData{
			day, "01:30-02:30",
			{make_tm("2021-03-28 01:00:00 CET")},
			{make_tm("2021-03-28 01:59:59 CET")},
			// As 02:30 does not exist on this day, it is parsed as if it was 02:30 CET which is actually 03:30 CEST.
			Segment("2021-03-28 01:30:00 CET", "2021-03-28 03:30:00 CEST"),
		});

		if (day.find("sunday") == std::string::npos) { // skip for non-absolute day specs (would find another sunday)
			// range beginning does not actually exist on that day
			tests.push_back(TestData{
				day, "02:30-03:30",
				{make_tm("2021-03-28 01:00:00 CET"), make_tm("2021-03-28 01:59:59 CET")},
				{make_tm("2021-03-28 03:00:00 CEST")},
				// As 02:30 does not exist on this day, it is parsed as if it was 02:30 CET which is actually 03:30 CEST.
				// Therefore, the result is a segment from 03:30 CEST to 03:30 CEST with a duration of 0, i.e. no segment.
				boost::none,
			});
		}

		// another range where the beginning does not actually exist on that day
		tests.push_back(TestData{
			day, "02:15-03:45",
			{make_tm("2021-03-28 01:00:00 CET"), make_tm("2021-03-28 01:59:59 CET")},
			{make_tm("2021-03-28 03:30:00 CEST")},
			// As 02:15 does not exist on this day, it is parsed as if it was 02:15 CET which is actually 03:15 CEST.
			Segment("2021-03-28 03:15:00 CEST", "2021-03-28 03:45:00 CEST"),
		});

		// range after DST change
		tests.push_back(TestData{
			day, "03:30-04:30",
			{make_tm("2021-03-28 01:00:00 CET"), make_tm("2021-03-28 03:00:00 CEST")},
			{make_tm("2021-03-28 04:00:00 CEST")},
			Segment("2021-03-28 03:30:00 CEST", "2021-03-28 04:30:00 CEST"),
		});

		// range containing DST change
		tests.push_back(TestData{
			day, "01:30-03:30",
			{make_tm("2021-03-28 01:00:00 CET")},
			{make_tm("2021-03-28 01:45:00 CET"), make_tm("2021-03-28 03:15:00 CEST")},
			Segment("2021-03-28 01:30:00 CET", "2021-03-28 03:30:00 CEST"),
		});
	}

	// 2021-10-31: 02:59:59 CEST (UTC+2) -> 02:00:00 CET (UTC+1)
	for (const std::string &day : {"2021-10-31", "sunday", "sunday -1", "sunday 5"}) {
		// range before DST change
		tests.push_back(TestData{
			day, "00:30-01:30",
			{make_tm("2021-10-31 00:00:00 CEST")},
			{make_tm("2021-10-31 01:00:00 CEST")},
			Segment("2021-10-31 00:30:00 CEST", "2021-10-31 01:30:00 CEST"),
		});

		// range existing twice during DST change (first instance)
		tests.push_back(TestData{
			day, "02:15-02:45",
			{make_tm("2021-10-31 02:00:00 CEST")},
			{make_tm("2021-10-31 02:30:00 CEST")},
			Segment("2021-10-31 02:15:00 CEST", "2021-10-31 02:45:00 CEST"),
		});

		if (day.find("sunday") == std::string::npos) { // skip for non-absolute day specs (would find another sunday)
			// range existing twice during DST change (second instance)
			tests.push_back(TestData{
				day, "02:15-02:45",
				{make_tm("2021-10-31 02:00:00 CET")},
				{make_tm("2021-10-31 02:30:00 CET")},
				boost::none,
				// It is a known limitation of the current code that on only catches
				// the first segment. The following would also be fine.
				//Segment("2021-10-31 02:15:00 CET", "2021-10-31 02:45:00 CET"),
			});
		}

		// range after DST change
		tests.push_back(TestData{
			day, "03:30-04:30",
			{make_tm("2021-10-31 01:00:00 CEST"), make_tm("2021-10-31 03:00:00 CET")},
			{make_tm("2021-10-31 04:00:00 CET")},
			Segment("2021-10-31 03:30:00 CET", "2021-10-31 04:30:00 CET"),
		});

		// range containing DST change
		tests.push_back(TestData{
			day, "01:30-03:30",
			{make_tm("2021-10-31 01:00:00 CEST")},
			{make_tm("2021-10-31 01:45:00 CEST"), make_tm("2021-10-31 02:30:00 CEST"), make_tm("2021-10-31 02:30:00 CET"), make_tm("2021-10-31 03:15:00 CET")},
			Segment("2021-10-31 01:30:00 CEST", "2021-10-31 03:30:00 CET"),
		});

		// range ending during duplicate DST hour (first instance)
		tests.push_back(TestData{
			day, "01:30-02:30",
			{make_tm("2021-10-31 01:00:00 CEST")},
			{make_tm("2021-10-31 02:00:00 CEST")},
			// Both times are interpreted as the first instance on that day (i.e both CEST)
			Segment("2021-10-31 01:30:00 CEST", "2021-10-31 02:30:00 CEST")
		});

		// range beginning during duplicate DST hour (first instance)
		tests.push_back(TestData{
			day, "02:30-03:30",
			{make_tm("2021-10-31 02:00:00 CEST")},
			{make_tm("2021-10-31 03:00:00 CEST")},
			// 02:30 is interpreted as the first occurrence (CEST) but since there's no 03:30 CEST, it's CET
			Segment("2021-10-31 02:30:00 CEST", "2021-10-31 03:30:00 CET")
		});

		if (day.find("sunday") == std::string::npos) { // skip for non-absolute day specs (would find another sunday)
			// range ending during duplicate DST hour (second instance)
			tests.push_back(TestData{
				day, "01:30-02:30",
				{make_tm("2021-10-31 01:00:00 CET")},
				{make_tm("2021-10-31 02:00:00 CET")},
				// Both times are parsed as CEST. Thus, 01:00 CET (02:00 CEST) is during
				// the segment and 02:00 CET (03:00 CEST) is after the segment.
				boost::none
			});
		}

		// range beginning during duplicate DST hour (second instance)
		tests.push_back(TestData{
			day, "02:30-03:30",
			{make_tm("2021-10-31 02:00:00 CEST")},
			{make_tm("2021-10-31 02:45:00 CEST"), make_tm("2021-10-31 03:00:00 CET")},
			// As 02:30 always refers to the first occurrence (CEST), this is actually a 2 hour segment
			Segment("2021-10-31 02:30:00 CEST", "2021-10-31 03:30:00 CET")
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

	for (const auto &t : tests) {
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
				BOOST_CHECK_MESSAGE(t.expected->begin < t.expected->end, "rofl");
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

BOOST_AUTO_TEST_SUITE_END()
