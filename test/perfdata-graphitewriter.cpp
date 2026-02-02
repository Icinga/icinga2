/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include <BoostTestTargetConfig.h>
#include "perfdata/graphitewriter.hpp"
#include "test/base-testloggerfixture.hpp"
#include "test/perfdata-perfdatawriterfixture.hpp"
#include "test/utils.hpp"

using namespace icinga;

BOOST_FIXTURE_TEST_SUITE(
	perfdata_graphitewriter,
	PerfdataWriterFixture<GraphiteWriter>,
	*boost::unit_test::label("perfdata")
)

BOOST_AUTO_TEST_CASE(connect)
{
	ResumeWriter();

	ReceiveCheckResults(1, ServiceState::ServiceCritical);

	Accept();
	auto msg = GetDataUntil('\n');

	// Just some basic sanity tests. It's not important to check if everything is entirely correct here.
	std::string_view cmpStr{"icinga2.h1.host.dummy.perfdata.thing.value 42"};
	BOOST_REQUIRE_EQUAL(msg.substr(0, cmpStr.length()), cmpStr);
	PauseWriter();
}

BOOST_AUTO_TEST_CASE(pause_with_pending_work)
{
	ResumeWriter();

	// Make GraphiteWriter send a huge message that fills up the connection's buffer.
	ReceiveCheckResults(1, ServiceState::ServiceCritical, [&](const CheckResult::Ptr& cr) {
		cr->GetPerformanceData()->Add(new PerfdataValue{GetRandomString("aaaa", 24UL * 1024 * 1024), 1});
	});

	// Accept the connection, but don't read from it to leave the client hanging.
	Accept();
	GetDataUntil("aaaa");

	// Now try to pause.
	PauseWriter();

	REQUIRE_LOG_MESSAGE("Operation Cancelled\\.", 10s);
	REQUIRE_LOG_MESSAGE("'GraphiteWriter' paused\\.", 10s);
}

BOOST_AUTO_TEST_SUITE_END()
