// SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <BoostTestTargetConfig.h>
#include "remote/httputility.hpp"
#include "test/icingaapplication-fixture.hpp"

using namespace icinga;

BOOST_AUTO_TEST_SUITE(remote_httputility)

BOOST_AUTO_TEST_CASE(IsValidHeaderName)
{
	// Use string_view literals (""sv) to allow test inputs containing '\0'.
	using namespace std::string_view_literals;

	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderName("Host"sv), true);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderName("X-Powered-By"sv), true);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderName("Content-Security-Policy"sv), true);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderName("Strict-Transport-Security"sv), true);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderName("lowercase-is-fine-too"sv), true);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderName("everything-from-the-spec-!#$%&'*+-.^_`|~0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"sv), true);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderName("-this-seems-to-be-allowed-too-"sv), true);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderName("~http~is~weird~"sv), true);

	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderName(""sv /* empty header name is invalid */), false);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderName("spaces are not allowed"sv), false);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderName("tabs\tare\tnot\tallowed"sv), false);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderName("nul-is-bad\0"sv), false);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderName("del-is-bad\x7f"sv), false);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderName("non-ascii-is-bad\x80"sv), false);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderName("non-ascii-is-bad\xff"sv), false);
}

BOOST_AUTO_TEST_CASE(IsValidHeaderValue)
{
	// Use string_view literals (""sv) to allow test inputs containing '\0'.
	using namespace std::string_view_literals;

	auto everything = []{
		std::string s = "everything-from-the-spec \t ";
		for (int i = 0x21; i <= 0x7e; ++i) {
			s.push_back(char(i));
		}
		for (int i = 0x80; i <= 0xff; ++i) {
			s.push_back(char(i));
		}

		// Sanity checks:
		for (char c : {'\x00', '\x08', '\x0a', '\x1f', '\x7f'}) {
			BOOST_CHECK_EQUAL(s.find(c), std::string::npos);
		}
		for (char c : {'\t' /* == 0x09 */, ' ' /* == 0x20 */, '\x21', '\x7e', '\x80', '\xff'}) {
			BOOST_CHECK_NE(s.find(c), std::string::npos);
		}

		return s;
	};

	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderValue(""sv /* empty header value is allowed */), true);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderValue("example.com"sv), true);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderValue("default-src 'self'; img-src 'self' example.com"sv), true);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderValue("max-age=31536000"sv), true);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderValue("spaces are allowed"sv), true);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderValue("tabs\tare\tallowed"sv), true);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderValue("non-ascii-is-allowed\x80"sv), true);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderValue("non-ascii-is-allowed\xff"sv), true);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderValue(everything()), true);

	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderValue("nul-is-bad\0"sv), false);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderValue("del-is-bad\x7f"sv), false);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderValue(" no leading spaces"sv), false);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderValue("no trailing spaces "sv), false);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderValue("\tno leading tabs"sv), false);
	BOOST_CHECK_EQUAL(HttpUtility::IsValidHeaderValue("no trailing tabs\t"sv), false);
}

BOOST_AUTO_TEST_SUITE_END()
