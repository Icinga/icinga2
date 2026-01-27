// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/utility.hpp"
#include "test/utils.hpp"
#include <chrono>
#include <BoostTestTargetConfig.h>

#ifdef _WIN32
# include <windows.h>
# include <shellapi.h>
#endif /* _WIN32 */

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_utility)

BOOST_AUTO_TEST_CASE(parse_version)
{
	BOOST_CHECK(Utility::ParseVersion("2.11.0-0.rc1.1") == "2.11.0");
	BOOST_CHECK(Utility::ParseVersion("v2.10.5") == "2.10.5");
	BOOST_CHECK(Utility::ParseVersion("r2.11.1") == "2.11.1");
	BOOST_CHECK(Utility::ParseVersion("v2.11.0-rc1-58-g7c1f716da") == "2.11.0");

	BOOST_CHECK(Utility::ParseVersion("v2.11butactually3.0") == "v2.11butactually3.0");
}

BOOST_AUTO_TEST_CASE(compare_version)
{
	BOOST_CHECK(Utility::CompareVersion("2.10.5", Utility::ParseVersion("v2.10.4")) < 0);
	BOOST_CHECK(Utility::CompareVersion("2.11.0", Utility::ParseVersion("2.11.0-0")) == 0);
	BOOST_CHECK(Utility::CompareVersion("2.10.5", Utility::ParseVersion("2.11.0-0.rc1.1")) > 0);
}

BOOST_AUTO_TEST_CASE(comparepasswords_works)
{
	BOOST_CHECK(Utility::ComparePasswords("", ""));

	BOOST_CHECK(!Utility::ComparePasswords("x", ""));
	BOOST_CHECK(!Utility::ComparePasswords("", "x"));

	BOOST_CHECK(Utility::ComparePasswords("x", "x"));
	BOOST_CHECK(!Utility::ComparePasswords("x", "y"));

	BOOST_CHECK(Utility::ComparePasswords("abcd", "abcd"));
	BOOST_CHECK(!Utility::ComparePasswords("abc", "abcd"));
	BOOST_CHECK(!Utility::ComparePasswords("abcde", "abcd"));
}

BOOST_AUTO_TEST_CASE(comparepasswords_issafe)
{
	using std::chrono::duration_cast;
	using std::chrono::microseconds;
	using std::chrono::steady_clock;

	String a, b;

	a.Append(200000001, 'a');
	b.Append(200000002, 'b');

	auto start1 (steady_clock::now());

	Utility::ComparePasswords(a, a);

	auto duration1 (steady_clock::now() - start1);

	auto start2 (steady_clock::now());

	Utility::ComparePasswords(a, b);

	auto duration2 (steady_clock::now() - start2);

	double diff = (double)duration_cast<microseconds>(duration1).count() / (double)duration_cast<microseconds>(duration2).count();
	BOOST_WARN(0.9 <= diff && diff <= 1.1);
}

BOOST_AUTO_TEST_CASE(validateutf8)
{
	BOOST_CHECK(Utility::ValidateUTF8("") == "");
	BOOST_CHECK(Utility::ValidateUTF8("a") == "a");
	BOOST_CHECK(Utility::ValidateUTF8("\xC3") == "\xEF\xBF\xBD");
	BOOST_CHECK(Utility::ValidateUTF8("\xC3\xA4") == "\xC3\xA4");
}

BOOST_AUTO_TEST_CASE(EscapeCreateProcessArg)
{
#ifdef _WIN32
	using convert = std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>, wchar_t>;

	std::vector<std::string> testdata = {
		R"(foobar)",
		R"(foo bar)",
		R"(foo"bar)",
		R"("foo bar")",
		R"(" \" \\" \\\" \\\\")",
		R"( !"#$$%&'()*+,-./09:;<=>?@AZ[\]^_`az{|}~ " \" \\" \\\" \\\\")",
		"'foo\nbar'",
	};

	for (const auto& t : testdata) {
		// Prepend some fake exec name as the first argument is handled differently.
		std::string escaped = "some.exe " + Utility::EscapeCreateProcessArg(t);
		int argc;
		std::shared_ptr<LPWSTR> argv(CommandLineToArgvW(convert{}.from_bytes(escaped.c_str()).data(), &argc), LocalFree);
		BOOST_CHECK_MESSAGE(argv != nullptr, "CommandLineToArgvW() should not return nullptr for " << t);
		BOOST_CHECK_MESSAGE(argc == 2, "CommandLineToArgvW() should find 2 arguments for " << t);
		if (argc >= 2) {
			std::string unescaped = convert{}.to_bytes(argv.get()[1]);
			BOOST_CHECK_MESSAGE(unescaped == t,
				"CommandLineToArgvW() should return original value for " << t << " (got: " << unescaped << ")");
		}
	}
#endif /* _WIN32 */
}

BOOST_AUTO_TEST_CASE(TruncateUsingHash)
{
	/*
	 * Note: be careful when changing the output of TruncateUsingHash as it is used to derive file names that should not
	 * change between versions or would need special handling if they do (/var/lib/icinga2/api/packages/_api).
	 */

	/* minimum allowed value for maxLength template parameter */
	BOOST_CHECK_EQUAL(Utility::TruncateUsingHash<44>(std::string(64, 'a')),
		"a...0098ba824b5c16427bd7a1122a5a442a25ec644d");

	BOOST_CHECK_EQUAL(Utility::TruncateUsingHash<80>(std::string(100, 'a')),
		std::string(37, 'a') + "...7f9000257a4918d7072655ea468540cdcbd42e0c");

	/* short enough values should not be truncated */
	BOOST_CHECK_EQUAL(Utility::TruncateUsingHash<80>(""), "");
	BOOST_CHECK_EQUAL(Utility::TruncateUsingHash<80>(std::string(60, 'a')), std::string(60, 'a'));
	BOOST_CHECK_EQUAL(Utility::TruncateUsingHash<80>(std::string(79, 'a')), std::string(79, 'a'));

	/* inputs of maxLength are hashed to avoid collisions */
	BOOST_CHECK_EQUAL(Utility::TruncateUsingHash<80>(std::string(80, 'a')),
		std::string(37, 'a') + "...86f33652fcffd7fa1443e246dd34fe5d00e25ffd");
}

BOOST_AUTO_TEST_CASE(FormatDateTime) {
	using time_t_limit = std::numeric_limits<time_t>;
	using double_limit = std::numeric_limits<double>;
	using boost::numeric::negative_overflow;
	using boost::numeric::positive_overflow;

	// Helper to repeat a given string a number of times.
	auto repeat = [](const std::string& s, size_t n) {
		std::ostringstream stream;
		for (size_t i = 0; i < n; ++i) {
			stream << s;
		}
		return stream.str();
	};

	// Valid inputs.
	const double ts = 1136214245.0; // 2006-01-02 15:04:05 UTC
	BOOST_CHECK_EQUAL("2006-01-02 15:04:05", Utility::FormatDateTime("%F %T", ts));
	BOOST_CHECK_EQUAL("2006", Utility::FormatDateTime("%Y", ts));
	BOOST_CHECK_EQUAL("2006#2006", Utility::FormatDateTime("%Y#%Y", ts));
	BOOST_CHECK_EQUAL("%", Utility::FormatDateTime("%%", ts));
	BOOST_CHECK_EQUAL("%Y", Utility::FormatDateTime("%%Y", ts));
	BOOST_CHECK_EQUAL("", Utility::FormatDateTime("", ts));
	BOOST_CHECK_EQUAL("1970-01-01 00:00:00", Utility::FormatDateTime("%F %T", 0.0));
	BOOST_CHECK_EQUAL("2038-01-19 03:14:07", Utility::FormatDateTime("%F %T", 2147483647.0)); // 2^31 - 1
	if constexpr (sizeof(time_t) > sizeof(int32_t)) {
		BOOST_CHECK_EQUAL("2100-03-14 13:37:42", Utility::FormatDateTime("%F %T", 4108714662.0)); // Past year 2038
	} else {
		BOOST_WARN_MESSAGE(false, "skipping test with past 2038 input due to 32 bit time_t");
	}

	// Negative (pre-1970) timestamps.
#ifdef _MSC_VER
	// localtime_s() on Windows doesn't seem to like them and always errors out.
	BOOST_CHECK_THROW(Utility::FormatDateTime("%F %T", -1.0), posix_error);
	BOOST_CHECK_THROW(Utility::FormatDateTime("%F %T", -2147483648.0), posix_error); // -2^31
#else /* _MSC_VER */
	BOOST_CHECK_EQUAL("1969-12-31 23:59:59", Utility::FormatDateTime("%F %T", -1.0));
	BOOST_CHECK_EQUAL("1901-12-13 20:45:52", Utility::FormatDateTime("%F %T", -2147483648.0)); // -2^31
#endif /* _MSC_VER */

	// Values right at the limits of time_t.
	//
	// With 64 bit time_t, there may not be an exact double representation of its min/max value, std::nextafter() is
	// used to move the value towards 0 so that it's within the range of doubles that can be represented as time_t.
	//
	// These are expected to result in an error due to the intermediate struct tm not being able to represent these
	// timestamps, so localtime_r() returns EOVERFLOW which makes the implementation throw an exception.
	if constexpr (sizeof(time_t) > sizeof(int32_t)) {
		BOOST_CHECK_THROW(Utility::FormatDateTime("%Y", std::nextafter(time_t_limit::min(), 0)), posix_error);
		BOOST_CHECK_THROW(Utility::FormatDateTime("%Y", std::nextafter(time_t_limit::max(), 0)), posix_error);
	} else {
		BOOST_WARN_MESSAGE(false, "skipping test for struct tm overflow due to 32 bit time_t");
	}

	// Excessive format strings can result in something too large for the buffer, errors out to the empty string.
	// Note: both returning the proper result or throwing an exception would be fine too, unfortunately, that's
	// not really possible due to limitations in strftime() error handling, see comment in the implementation.
	BOOST_CHECK_EQUAL("", Utility::FormatDateTime(repeat("%Y", 1000).c_str(), ts));

	// Invalid format strings.
	for (const char* format : {"%", "x % y", "x %! y"}) {
		std::string result = Utility::FormatDateTime(format, ts);

		// Implementations of strftime() seem to either keep invalid format specifiers and return them in the output, or
		// treat them as an error which our implementation currently maps to the empty string due to strftime() not
		// properly reporting errors. If this limitation of our implementation is lifted, other behavior like throwing
		// an exception would also be valid.

		std::string percentLessOutput(format);
		// `strftime(3)` seems to return the provided invalid format specifiers on all Platforms as documented above,
		// i.e. even on macOS, but the macOS/*BSD libc implementations of `strftime(3)` appears to behave differently
		// and causes the `%` character not to be populated into the results buffer if invalid format specifiers such
		// as `"x %! y"` are given. If such specifiers are provided, the output will contain `x ! y` instead of the
		// given invalid format specifiers.
		percentLessOutput.erase(std::remove(percentLessOutput.begin(), percentLessOutput.end(), '%'), percentLessOutput.end());

		BOOST_CHECK_MESSAGE(result.empty() || result == format || result == percentLessOutput,
			"FormatDateTime(" << std::quoted(format) << ", " << ts << ") = " << std::quoted(result) <<
			" should be one of [\"\", " << std::quoted(format) << "]");
	}

	// Out of range timestamps.
	//
	// At the limits of a 64 bit time_t, doubles can no longer represent each integer value, so a simple x+1 or x-1 can
	// have x as the result, hence std::nextafter() is used to get the next representable value. However, around the
	// limits of a 32 bit time_t, doubles still can represent decimal places and less than 1 is added or subtracted by
	// std::nextafter() and casting back to time_t simply results in the limit again, so std::ceil()/std::floor() is
	// used to round it to the next integer value that is actually out of range.
	double negative_out_of_range = std::floor(std::nextafter(time_t_limit::min(), -double_limit::infinity()));
	double positive_out_of_range = std::ceil(std::nextafter(time_t_limit::max(), +double_limit::infinity()));
	BOOST_CHECK_THROW(Utility::FormatDateTime("%Y", negative_out_of_range), negative_overflow);
	BOOST_CHECK_THROW(Utility::FormatDateTime("%Y", positive_out_of_range), positive_overflow);
}

BOOST_AUTO_TEST_CASE(NormalizeTm)
{
	GlobalTimezoneFixture tz(GlobalTimezoneFixture::TestTimezoneWithDST);

	auto normalize = [](const std::string_view& input) {
		tm t = make_tm(std::string(input));
		return Utility::NormalizeTm(&t);
	};

	auto is_dst = [](const std::string_view& input) {
		tm t = make_tm(std::string(input));
		Utility::NormalizeTm(&t);
		BOOST_CHECK_GE(t.tm_isdst, 0);
		return t.tm_isdst > 0;
	};

	// The whole day 2021-01-01 uses PST (24h day)
	BOOST_CHECK(!is_dst("2021-01-01 10:00:00"));
	BOOST_CHECK_EQUAL(normalize("2021-01-01 10:00:00"), 1609524000);
	BOOST_CHECK_EQUAL(normalize("2021-01-01 10:00:00 PST"), 1609524000);
	BOOST_CHECK_EQUAL(normalize("2021-01-01 11:00:00 PDT"), 1609524000); // normalized to 10:00 PST
	BOOST_CHECK_EQUAL(normalize("2021-01-02 00:00:00") - normalize("2021-01-01 00:00:00"), 24*60*60);

	// The whole day 2021-07-01 uses PDT (24h day)
	BOOST_CHECK(is_dst("2021-07-01 10:00:00"));
	BOOST_CHECK_EQUAL(normalize("2021-07-01 10:00:00"), 1625158800);
	BOOST_CHECK_EQUAL(normalize("2021-07-01 10:00:00 PDT"), 1625158800);
	BOOST_CHECK_EQUAL(normalize("2021-07-01 09:00:00 PST"), 1625158800); // normalized to 10:00 PDT
	BOOST_CHECK_EQUAL(normalize("2021-07-02 00:00:00") - normalize("2021-07-01 00:00:00"), 24*60*60);

	// On 2021-03-14, PST changes to PDT (23h day)
	BOOST_CHECK(!is_dst("2021-03-14 00:00:00"));
	BOOST_CHECK(is_dst("2021-03-14 23:59:59"));
	BOOST_CHECK_EQUAL(normalize("2021-03-15 00:00:00") - normalize("2021-03-14 00:00:00"), 23*60*60);

	BOOST_CHECK_EQUAL(normalize("2021-03-14 01:59:59 PST"), 1615715999);
	// The following three times do not exist on that day in that timezone.
	// They are interpreted as UTC-8, which is the offset of PST.
	BOOST_CHECK_EQUAL(normalize("2021-03-14 02:00:00 PST"), 1615716000);
	BOOST_CHECK_EQUAL(normalize("2021-03-14 02:30:00 PST"), 1615717800);
	BOOST_CHECK_EQUAL(normalize("2021-03-14 03:00:00 PST"), 1615719600);

	BOOST_CHECK_EQUAL(normalize("2021-03-14 03:00:00 PDT"), 1615716000);
	// The following three times do not exist on that day in that timezone.
	// They are interpreted as UTC-7, which is the offset of PDT.
	BOOST_CHECK_EQUAL(normalize("2021-03-14 01:59:59 PDT"), 1615712399);
	BOOST_CHECK_EQUAL(normalize("2021-03-14 02:00:00 PDT"), 1615712400);
	BOOST_CHECK_EQUAL(normalize("2021-03-14 02:30:00 PDT"), 1615714200);

	BOOST_CHECK_EQUAL(normalize("2021-03-14 01:59:59"), 1615715999);
	BOOST_CHECK_EQUAL(normalize("2021-03-14 03:00:00"), 1615716000);
	// The following two times don't exist on that day, they are within the hour that is skipped.
	// They are interpreted as UTC-8 (offset of PST) and then normalized to PDT.
	BOOST_CHECK_EQUAL(normalize("2021-03-14 02:00:00"), 1615716000);
	BOOST_CHECK_EQUAL(normalize("2021-03-14 02:30:00"), 1615717800);

	// On 2021-11-07, PDT changes to PST (25h day)
	BOOST_CHECK(is_dst("2021-11-07 00:00:00"));
	BOOST_CHECK(!is_dst("2021-11-07 23:59:59"));
	BOOST_CHECK_EQUAL(normalize("2021-11-08 00:00:00") - normalize("2021-11-07 00:00:00"), 25*60*60);

	BOOST_CHECK_EQUAL(normalize("2021-11-07 00:59:59 PDT"), 1636271999);
	BOOST_CHECK_EQUAL(normalize("2021-11-07 01:00:00 PDT"), 1636272000);
	BOOST_CHECK_EQUAL(normalize("2021-11-07 01:30:00 PDT"), 1636273800);
	BOOST_CHECK_EQUAL(normalize("2021-11-07 01:59:59 PDT"), 1636275599);
	// The following time does not exist on that day in that timezone, it's interpreted as 01:00:00 PST.
	BOOST_CHECK_EQUAL(normalize("2021-11-07 02:00:00 PDT"), 1636275600);

	// The following time does not exist on that day in that timezone, it's interpreted as 01:59:59 PDT.
	BOOST_CHECK_EQUAL(normalize("2021-11-07 00:59:59 PST"), 1636275599);
	BOOST_CHECK_EQUAL(normalize("2021-11-07 01:00:00 PST"), 1636275600);
	BOOST_CHECK_EQUAL(normalize("2021-11-07 01:30:00 PST"), 1636277400);
	BOOST_CHECK_EQUAL(normalize("2021-11-07 01:59:59 PST"), 1636279199);
	BOOST_CHECK_EQUAL(normalize("2021-11-07 02:00:00 PST"), 1636279200);

	BOOST_CHECK_EQUAL(normalize("2021-11-07 00:59:59"), 1636271999); // unambiguous: PDT
	BOOST_CHECK_EQUAL(normalize("2021-11-07 01:00:00"), 1636272000); // exists twice, interpreted as PDT
	BOOST_CHECK_EQUAL(normalize("2021-11-07 01:30:00"), 1636273800); // exists twice, interpreted as PDT
	BOOST_CHECK_EQUAL(normalize("2021-11-07 01:59:59"), 1636275599); // exists twice, interpreted as PDT
	BOOST_CHECK_EQUAL(normalize("2021-11-07 02:00:00"), 1636279200); // unambiguous: PST
}

BOOST_AUTO_TEST_SUITE_END()
