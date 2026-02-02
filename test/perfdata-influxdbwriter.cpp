/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include <BoostTestTargetConfig.h>
#include "perfdata/influxdb2writer.hpp"
#include "test/base-testloggerfixture.hpp"
#include "test/perfdata-perfdatawriterfixture.hpp"

using namespace icinga;

BOOST_FIXTURE_TEST_SUITE(perfdata_influxdbwriter, PerfdataWriterFixture<Influxdb2Writer>,
	*boost::unit_test::label("perfdata"))

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
	std::string_view perfData = "metric=thing value=42";
	BOOST_CHECK_EQUAL(req[2].substr(0, perfData.length()), perfData);
	PauseWriter();
}

BOOST_AUTO_TEST_CASE(pause_with_pending_work)
{
	ResumeWriter();

	// Make Influxdb2Writer fill up the connection's buffer with a huge check-result.
	ReceiveCheckResults(1, ServiceState::ServiceCritical);

	// Accept the connection, but only read far enough so we know the writer is now stuck.
	Accept();

	// Now try to pause.
	PauseWriter();

	REQUIRE_LOG_MESSAGE("Operation cancelled\\.", 1s);
	REQUIRE_LOG_MESSAGE("'Influxdb2Writer' paused\\.", 1s);
}

BOOST_AUTO_TEST_SUITE_END()
