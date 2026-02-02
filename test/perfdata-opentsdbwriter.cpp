/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include <BoostTestTargetConfig.h>
#include "perfdata/opentsdbwriter.hpp"
#include "test/base-testloggerfixture.hpp"
#include "test/perfdata-perfdatawriterfixture.hpp"
#include "test/utils.hpp"

using namespace icinga;

BOOST_FIXTURE_TEST_SUITE(perfdata_opentsdbwriter, PerfdataWriterFixture<OpenTsdbWriter>,
	*boost::unit_test::label("perfdata"))

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

	// Make OpenTsdbWriter send a huge message that fills up the connection's buffer.
	ReceiveCheckResults(1, ServiceState::ServiceCritical, [&](const CheckResult::Ptr& cr) {
		cr->GetPerformanceData()->Add(new PerfdataValue{GetRandomString("aaaaa", 24 * 1024 * 1024), 1});
	});

	// Accept the connection, and read until OpenTsdbWriter has started sending the large part
	// of the PerfdataValue sent above.
	Accept();
	GetDataUntil("aaaaa");

	// Now stop reading and try to pause OpenTsdbWriter.
	PauseWriter();

	REQUIRE_LOG_MESSAGE("Operation canceled\\.", 10s);
	REQUIRE_LOG_MESSAGE("'OpenTsdbWriter' paused\\.", 10s);
}

BOOST_AUTO_TEST_SUITE_END()
