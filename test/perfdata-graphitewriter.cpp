// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <BoostTestTargetConfig.h>
#include "base/perfdatavalue.hpp"
#include "perfdata/graphitewriter.hpp"
#include "test/base-testloggerfixture.hpp"
#include "test/perfdata-perfdatawriterfixture.hpp"
#include "test/utils.hpp"

using namespace icinga;

BOOST_FIXTURE_TEST_SUITE(perfdata_graphitewriter, PerfdataWriterFixture<GraphiteWriter>,
	*boost::unit_test::label("perfdata")
	*boost::unit_test::label("network")
)

BOOST_AUTO_TEST_CASE(connect)
{
	ResumeWriter();

	ReceiveCheckResults(1, ServiceState::ServiceCritical);

	Accept();
	auto msg = GetDataUntil('\n');

	// Just some basic sanity tests. It's not important to check if everything is entirely correct here.
	std::string_view cmpStr{"icinga2.h1.host.dummy.perfdata.dummy.value 42"};
	BOOST_REQUIRE_EQUAL(msg.substr(0, cmpStr.length()), cmpStr);
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
	REQUIRE_LOG_MESSAGE("'GraphiteWriter' paused\\.", 10s);
}

BOOST_AUTO_TEST_SUITE_END()
