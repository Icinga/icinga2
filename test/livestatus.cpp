/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "livestatus/livestatusquery.hpp"
#include "base/application.hpp"
#include "base/stdiostream.hpp"
#include "base/json.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

String LivestatusQueryHelper(const std::vector<String>& lines)
{
	LivestatusQuery::Ptr query = new LivestatusQuery(lines, "");

	std::stringstream stream;
	StdioStream::Ptr sstream = new StdioStream(&stream, false);

	query->Execute(sstream);

	String output;
	String result;

	StreamReadContext src;
	for (;;) {
		StreamReadStatus srs = sstream->ReadLine(&result, src);

		if (srs == StatusEof)
			break;

		if (srs != StatusNewItem)
			continue;

		if (result.GetLength() > 0)
			output += result + "\n";
		else
			break;
	}

	BOOST_TEST_MESSAGE("Query Result: " + output);

	return output;
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE(livestatus)

BOOST_AUTO_TEST_CASE(hosts)
{
	BOOST_TEST_MESSAGE( "Querying Livestatus...");

	std::vector<String> lines;
	lines.emplace_back("GET hosts");
	lines.emplace_back("Columns: host_name address check_command");
	lines.emplace_back("OutputFormat: json");
	lines.emplace_back("\n");

	/* use our query helper */
	String output = LivestatusQueryHelper(lines);

	Array::Ptr query_result = JsonDecode(output);

	/* the outer elements */
	BOOST_CHECK(query_result->GetLength() > 1);

	Array::Ptr res1 = query_result->Get(0);
	Array::Ptr res2 = query_result->Get(1);

	/* results are non-deterministic and not sorted by livestatus */
	BOOST_CHECK(res1->Contains("test-01") || res2->Contains("test-01"));
	BOOST_CHECK(res1->Contains("test-02") || res2->Contains("test-02"));
	BOOST_CHECK(res1->Contains("127.0.0.1") || res2->Contains("127.0.0.1"));
	BOOST_CHECK(res1->Contains("127.0.0.2") || res2->Contains("127.0.0.2"));

	BOOST_TEST_MESSAGE("Done with testing livestatus hosts...");
}

BOOST_AUTO_TEST_CASE(services)
{
	BOOST_TEST_MESSAGE( "Querying Livestatus...");

	std::vector<String> lines;
	lines.emplace_back("GET services");
	lines.emplace_back("Columns: host_name service_description check_command notes");
	lines.emplace_back("OutputFormat: json");
	lines.emplace_back("\n");

	/* use our query helper */
	String output = LivestatusQueryHelper(lines);

	Array::Ptr query_result = JsonDecode(output);

	/* the outer elements */
	BOOST_CHECK(query_result->GetLength() > 1);

	Array::Ptr res1 = query_result->Get(0);
	Array::Ptr res2 = query_result->Get(1);

	/* results are non-deterministic and not sorted by livestatus */
	BOOST_CHECK(res1->Contains("livestatus") || res2->Contains("livestatus")); //service_description
	BOOST_CHECK(res1->Contains("test livestatus") || res2->Contains("test livestatus")); //notes

	BOOST_TEST_MESSAGE("Done with testing livestatus services...");
}
//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
