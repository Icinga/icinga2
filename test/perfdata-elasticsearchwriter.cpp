// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <BoostTestTargetConfig.h>
#include "perfdata/elasticsearchwriter.hpp"
#include "test/base-testloggerfixture.hpp"
#include "test/perfdata-perfdatawriterfixture.hpp"
#include "test/utils.hpp"

using namespace icinga;

BOOST_FIXTURE_TEST_SUITE(perfdata_elasticsearchwriter, PerfdataWriterFixture<ElasticsearchWriter>,
	*boost::unit_test::label("perfdata")
	*boost::unit_test::label("network")
)

BOOST_AUTO_TEST_CASE(connect)
{
	ResumeWriter();

	ReceiveCheckResults(1, ServiceState::ServiceCritical);

	Accept();
	auto resp = GetSplitDecodedRequestBody();
	SendResponse();

	// ElasticsearchWriter wants to send the same message twice, once for the check result
	// and once for the "state change".
	resp = GetSplitDecodedRequestBody();
	SendResponse();

	// Just some basic sanity tests. It's not important to check if everything is entirely
	// correct here.
	BOOST_REQUIRE_GT(resp->GetLength(), 1);
	Dictionary::Ptr cr = resp->Get(1);
	BOOST_CHECK(cr->Contains("@timestamp"));
	BOOST_CHECK_EQUAL(cr->Get("check_command"), "dummy");
	BOOST_CHECK_EQUAL(cr->Get("host"), "h1");

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
	REQUIRE_LOG_MESSAGE("'ElasticsearchWriter' paused\\.", 10s);
}

BOOST_AUTO_TEST_SUITE_END()
