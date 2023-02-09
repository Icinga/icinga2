/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/utility.hpp"
#include <chrono>
#include <BoostTestTargetConfig.h>

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

BOOST_AUTO_TEST_SUITE_END()
