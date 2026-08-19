// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <BoostTestTargetConfig.h>
#include "base/perfdatavalue.hpp"
#include "perfdata/opentsdbwriter.hpp"
#include "test/base-testloggerfixture.hpp"
#include "test/perfdata-perfdatawriterfixture.hpp"
#include "test/utils.hpp"

using namespace icinga;

BOOST_FIXTURE_TEST_SUITE(perfdata_opentsdbwriter, PerfdataWriterFixture<OpenTsdbWriter>,
	*boost::unit_test::label("perfdata")
	*boost::unit_test::label("network")
)

BOOST_AUTO_TEST_CASE(connect)
{
	ResumeWriter();

	ReceiveCheckResults(1, ServiceState::ServiceCritical);

	Accept();
	auto msg = GetDataUntil('\n');
	std::vector<std::string> splitMsg;
	boost::split(splitMsg, msg, boost::is_any_of(" "));

	// Just some basic sanity tests. It's not important to check if everything is entirely correct here.
	BOOST_REQUIRE_EQUAL(splitMsg.size(), 5);
	BOOST_REQUIRE_EQUAL(splitMsg[0], "put");
	BOOST_REQUIRE_EQUAL(splitMsg[1], "icinga.host.state");
	BOOST_REQUIRE_CLOSE(boost::lexical_cast<double>(splitMsg[2]), Utility::GetTime(), 1);
	BOOST_REQUIRE_EQUAL(splitMsg[3], "1");
	BOOST_REQUIRE_EQUAL(splitMsg[4], "host=h1");
	PauseWriter();
}

BOOST_AUTO_TEST_CASE(pause_with_pending_work)
{
	ResumeWriter();

	// Process check-results until the writer is stuck.
	BOOST_REQUIRE_MESSAGE(GetWriterStuck(10s), "Failed to get Writer stuck.");

	// Now stop reading and try to pause OpenTsdbWriter.
	PauseWriter();

	REQUIRE_LOG_MESSAGE("Connection stopped\\.", 10s);
	REQUIRE_LOG_MESSAGE("'OpenTsdbWriter' paused\\.", 10s);
}

BOOST_AUTO_TEST_SUITE_END()
