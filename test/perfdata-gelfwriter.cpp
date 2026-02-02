/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include <BoostTestTargetConfig.h>
#include "perfdata/gelfwriter.hpp"
#include "test/base-testloggerfixture.hpp"
#include "test/perfdata-perfdatawriterfixture.hpp"
#include "test/utils.hpp"

using namespace icinga;

BOOST_FIXTURE_TEST_SUITE(perfdata_gelfwriter, PerfdataWriterFixture<GelfWriter>,
	*boost::unit_test::label("perfdata"))

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

	// Make GelfWriter fill up the connection's buffer with a huge check-result.
	ReceiveCheckResults(1, ServiceState::ServiceCritical, [](const CheckResult::Ptr& cr) {
		cr->SetOutput(GetRandomString("####", 1024UL * 1024));
	});

	// Accept the connection, but only read far enough so we know the writer is now stuck.
	Accept();
	GetDataUntil("####");

	// Now try to pause.
	PauseWriter();

	REQUIRE_LOG_MESSAGE("Operation cancelled\\.", 1s);
	REQUIRE_LOG_MESSAGE("'GelfWriter' paused\\.", 1s);
}

BOOST_AUTO_TEST_SUITE_END()
