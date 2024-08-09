/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/utility.hpp"
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
	// Helper to repeat a given string a number of times.
	auto repeat = [](const std::string& s, size_t n) {
		std::ostringstream stream;
		for (size_t i = 0; i < n; ++i) {
			stream << s;
		}
		return stream.str();
	};

	const time_t t = 1136214245; // 2006-01-02 15:04:05 UTC

	BOOST_CHECK_EQUAL("2006-01-02 15:04:05", Utility::FormatDateTime("%F %T", t));
	BOOST_CHECK_EQUAL("2006", Utility::FormatDateTime("%Y", t));
	BOOST_CHECK_EQUAL("2006#2006", Utility::FormatDateTime("%Y#%Y", t));
	BOOST_CHECK_EQUAL("%", Utility::FormatDateTime("%%", t));
	BOOST_CHECK_EQUAL("%Y", Utility::FormatDateTime("%%Y", t));
	BOOST_CHECK_EQUAL("", Utility::FormatDateTime("", t));

	// Inconsistent behavior between platforms: Windows prefers negative 0, others prefer positive 0.
	std::string z = Utility::FormatDateTime("%z", t);
	BOOST_CHECK_MESSAGE(z == "+0000" || z == "-0000",
		"FormatDateTime(\"%z\", " << t << ") = " << std::quoted(z) << " should be one of [\"+0000\", \"-0000\"]");

	// Long format string with a long result.
	BOOST_CHECK_EQUAL(repeat("2024", 1000), Utility::FormatDateTime(repeat("%Y", 1000).c_str(), 1723105155));

	for (const char* format : {"%", "x % y", "x %! y"}) {
		try {
			std::string result = Utility::FormatDateTime(format, t);

			// An invalid format string should either return a predictable result ...
			BOOST_CHECK_MESSAGE(result.empty() || result == format,
				"FormatDateTime(" << std::quoted(format) << ", " << t << ") = " << std::quoted(result) <<
				" should be one of [\"\", " << std::quoted(format) << "]");
		} catch (const std::invalid_argument& ex) {
			// ... or throw an exception.
			BOOST_TEST_MESSAGE("FormatDateTime(" << std::quoted(format) << ", " << t << ") threw: " << ex.what());
		}
	}
}

BOOST_AUTO_TEST_SUITE_END()
