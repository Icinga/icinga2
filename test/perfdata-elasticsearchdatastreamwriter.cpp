/* Icinga 2 | (c) 2026 Icinga GmbH | GPLv2+ */

#include <BoostTestTargetConfig.h>
#include "perfdata/elasticsearchdatastreamwriter.hpp"
#include "test/base-testloggerfixture.hpp"
#include "test/perfdata-perfdatawriterfixture.hpp"
#include "test/utils.hpp"

using namespace icinga;

class ElasticsearchDatastreamWriterFixture : public PerfdataWriterFixture<ElasticsearchDatastreamWriter>
{

};

BOOST_FIXTURE_TEST_SUITE(perfdata_elasticsearchdatastreamwriter, PerfdataWriterFixture<ElasticsearchDatastreamWriter>,
	*boost::unit_test::label("perfdata"))

BOOST_AUTO_TEST_CASE(connect)
{
	Logger::SetConsoleLogSeverity(LogDebug);
	Logger::EnableConsoleLog();
	ReceiveCheckResults(1, ServiceState::ServiceCritical);

	Accept();
	auto resp = GetSplitDecodedRequestBody();
	SendResponse();

	// Just some basic sanity tests. It's not important to check if everything is entirely
	// correct here.
	BOOST_REQUIRE_GT(resp->GetLength(), 1);
	Dictionary::Ptr cr = resp->Get(1);
	BOOST_CHECK(cr->Contains("@timestamp"));
	BOOST_CHECK_EQUAL(cr->Get("check_command"), "dummy");
	BOOST_CHECK_EQUAL(cr->Get("host"), "h1");
	Pause();
}

BOOST_AUTO_TEST_CASE(pause_with_pending_work)
{
	ReceiveCheckResults(1, ServiceState::ServiceCritical, [](const CheckResult::Ptr& cr) {
		cr->SetOutput(GetRandomString("####", 1024UL * 1024));
	});

	// Accept the connection, but don't read from it to leave the client hanging.
	Accept();
	GetDataUntil("####");

	// Now try to pause.
	Pause();

	REQUIRE_LOG_MESSAGE("Operation cancelled\\.", 10s);
	REQUIRE_LOG_MESSAGE("'ElasticsearchWriter' paused\\.", 10s);
}

BOOST_AUTO_TEST_SUITE_END()
