// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <BoostTestTargetConfig.h>
#include "base/logger.hpp"
#include "test/base-testloggerfixture.hpp"

using namespace icinga;
using namespace std::string_view_literals;
using namespace std::chrono_literals;

BOOST_FIXTURE_TEST_SUITE(base_logger, TestLoggerFixture)

BOOST_AUTO_TEST_CASE(log_quote_next)
{
	std::string_view quoted = "'some already quoted string'";

	Log(LogInformation, "Test") << "Quoted: " << QuoteNext << quoted << " Unquoted: " << quoted;
	CHECK_LOG_MESSAGE(R"(Quoted: '\'some already quoted string\'' Unquoted: 'some already quoted string')", 0s);
}

BOOST_AUTO_TEST_CASE(log_quote_start_end)
{
	std::string_view host = "bad\nexample"sv, service = "don't\\do\0this"sv;

	Log(LogInformation, "Test") << "Checking " << QuoteStart << host << '!' << service << QuoteEnd << " now";
	CHECK_LOG_MESSAGE(R"(Checking 'bad\nexample!don\'t\\do\0this' now)", 0s);
}

BOOST_AUTO_TEST_CASE(log_quote_missing_end)
{
	std::string_view command = "some 'command with spaces' in it";
	// Check that if the terminating QuoteEnd is missing, it's added at the end of the log message to prevent unclosed
	// string literals.
	Log(LogInformation, "Test") << "Executing " << QuoteStart << command;
	CHECK_LOG_MESSAGE(R"(Executing 'some \'command with spaces\' in it')", 0s);
}

BOOST_AUTO_TEST_CASE(log_fallback_sanitization)
{
	Log(LogInformation, "Test") << "The string 'newline=\n tab=\t quote=' null=\0' may be odd"sv;
	CHECK_LOG_MESSAGE("The string 'newline=\n\t tab=\t quote=' null=\\0' may be odd", 0s);
}

BOOST_AUTO_TEST_SUITE_END()
