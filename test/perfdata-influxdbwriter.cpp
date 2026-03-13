// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <BoostTestTargetConfig.h>
#include "perfdata/influxdb2writer.hpp"
#include "test/base-testloggerfixture.hpp"
#include "test/perfdata-perfdatawriterfixture.hpp"

using namespace icinga;

BOOST_FIXTURE_TEST_SUITE(perfdata_influxdbwriter, PerfdataWriterFixture<Influxdb2Writer>,
	*boost::unit_test::label("perfdata")
	*boost::unit_test::label("network")
)

BOOST_AUTO_TEST_CASE(connect)
{
	ResumeWriter();

	ReceiveCheckResults(1, ServiceState::ServiceCritical);

	Accept();
	auto req = GetSplitRequestBody(',');
	SendResponse(boost::beast::http::status::no_content);

	// Just some basic sanity tests. It's not important to check if everything is entirely
	// correct here.
	BOOST_REQUIRE_EQUAL(req.size(), 3);
	BOOST_CHECK_EQUAL(req[0], "dummy");
	BOOST_CHECK_EQUAL(req[1], "hostname=h1");
	std::string_view perfData = "metric=dummy value=42";
	BOOST_CHECK_EQUAL(req[2].substr(0, perfData.length()), perfData);
	PauseWriter();
}

BOOST_AUTO_TEST_CASE(pause_with_pending_work)
{
	ResumeWriter();

	// Process check-results until the writer is stuck.
	BOOST_REQUIRE_MESSAGE(GetWriterStuck(10s), "Failed to get Writer stuck.");

	// Now try to pause.
	PauseWriter();

	REQUIRE_LOG_MESSAGE("Connection stopped\\.", 10s);
	REQUIRE_LOG_MESSAGE("'Influxdb2Writer' paused\\.", 1s);
}

BOOST_AUTO_TEST_SUITE_END()
