// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <BoostTestTargetConfig.h>
#include "perfdata/gelfwriter.hpp"
#include "test/base-testloggerfixture.hpp"
#include "test/perfdata-perfdatawriterfixture.hpp"
#include "test/utils.hpp"

using namespace icinga;

BOOST_FIXTURE_TEST_SUITE(perfdata_gelfwriter, PerfdataWriterFixture<GelfWriter>,
	*boost::unit_test::label("perfdata")
	*boost::unit_test::label("network")
)

BOOST_AUTO_TEST_CASE(connect)
{
	ResumeWriter();

	ReceiveCheckResults(1, ServiceState::ServiceCritical);

	Accept();
	Dictionary::Ptr resp = JsonDecode(GetDataUntil('\0'));

	// Just some basic sanity tests. It's not important to check if everything is entirely
	// correct here.
	BOOST_CHECK_CLOSE(resp->Get("timestamp").Get<double>(), Utility::GetTime(), 0.5);
	BOOST_CHECK_EQUAL(resp->Get("_check_command"), "dummy");
	BOOST_CHECK_EQUAL(resp->Get("_hostname"), "h1");
	PauseWriter();
}

BOOST_AUTO_TEST_CASE(pause_with_pending_work)
{
	ResumeWriter();

	// Process check-results until the writer is stuck.
	BOOST_REQUIRE_MESSAGE(GetWriterStuck(10s), "Failed to get Writer stuck.");

	// Now stop reading and try to pause OpenTsdbWriter.
	PauseWriter();

	REQUIRE_LOG_MESSAGE("Connection stopped\\.", 1s);
	REQUIRE_LOG_MESSAGE("'GelfWriter' paused\\.", 1s);
}

BOOST_AUTO_TEST_SUITE_END()
