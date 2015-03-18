/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
#include "config/configtype.hpp"
#include "config/configcompiler.hpp"
#include "base/application.hpp"
#include "base/debug.hpp"
#include "base/objectlock.hpp"
#include "base/console.hpp"
#include "base/serializer.hpp"
#include "base/stdiostream.hpp"
#include "base/json.hpp"
#include "base/loader.hpp"
#include "cli/daemonutility.hpp"
#include <boost/test/unit_test.hpp>
#include <fstream>


using namespace icinga;

struct GlobalConfigFixture {
	GlobalConfigFixture()
	    : TestConfig("test-config.conf")
	{
		BOOST_MESSAGE("setup global config fixture");

		String cfg_file_path = TestConfig;
		String cfg_file_path_tmp = TestConfig + ".tmp";

		std::ofstream cfgfp;
		cfgfp.open(cfg_file_path_tmp.CStr(), std::ofstream::out | std::ofstream::trunc);
		cfgfp << std::fixed;
		cfgfp << "object CheckCommand \"dummy\" {\n";
		cfgfp << "  execute = PluginCheck\n";
		cfgfp << "  command = \"/bin/echo\"\n";
		cfgfp << "}\n";
		cfgfp << "object Host \"test-01\" {\n";
		cfgfp << "  address = \"127.0.0.1\"\n";
		cfgfp << "  check_command = \"dummy\"\n";
		cfgfp << "}\n";
		cfgfp << "object Host \"test-02\" {\n";
		cfgfp << "  address = \"127.0.0.2\"\n";
		cfgfp << "  check_command = \"dummy\"\n";
		cfgfp << "}\n";
		cfgfp << "apply Service \"livestatus\"{\n";
		cfgfp << "  check_command = \"dummy\"\n";
		cfgfp << "  notes = \"test livestatus\"\n";
		cfgfp << "  assign where match(\"test-*\", host.name)\n";
		cfgfp << "}\n";

		cfgfp.close();
#ifdef _WIN32
		_unlink(cfg_file_path.CStr());
#endif /* _WIN32 */

		if (rename(cfg_file_path_tmp.CStr(), cfg_file_path.CStr()) < 0) {
			BOOST_THROW_EXCEPTION(posix_error()
			    << boost::errinfo_api_function("rename")
			    << boost::errinfo_errno(errno)
			    << boost::errinfo_file_name(cfg_file_path_tmp));
		}

		BOOST_MESSAGE( "Preparing config objects...");

		/* start the Icinga application and load the configuration */
		Application::DeclareSysconfDir("etc");
		Application::DeclareLocalStateDir("var");

		Loader::LoadExtensionLibrary("icinga");
		Loader::LoadExtensionLibrary("methods"); //loaded by ITL

		std::vector<std::string> configs;
		configs.push_back(TestConfig);

		DaemonUtility::LoadConfigFiles(configs, "icinga2.debug", "icinga2.vars");

		/* ignore config errors */
		ConfigItem::ActivateItems();
	}

	~GlobalConfigFixture()
	{
		BOOST_MESSAGE("cleanup global config fixture");

		unlink(TestConfig.CStr());
	}

	String TestConfig;
};

struct LocalFixture {
	LocalFixture() { }
	~LocalFixture() { }
};

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

	BOOST_MESSAGE("Query Result: " + output);

	return output;
}

//____________________________________________________________________________//

BOOST_GLOBAL_FIXTURE(GlobalConfigFixture)

BOOST_FIXTURE_TEST_SUITE(livestatus, LocalFixture)

BOOST_AUTO_TEST_CASE(hosts)
{
	BOOST_MESSAGE( "Querying Livestatus...");

	std::vector<String> lines;
	lines.push_back("GET hosts");
	lines.push_back("Columns: host_name address check_command");
	lines.push_back("OutputFormat: json");
	lines.push_back("\n");

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

	BOOST_MESSAGE("Done with testing livestatus hosts...");
}

BOOST_AUTO_TEST_CASE(services)
{
	BOOST_MESSAGE( "Querying Livestatus...");

	std::vector<String> lines;
	lines.push_back("GET services");
	lines.push_back("Columns: host_name service_description check_command notes");
	lines.push_back("OutputFormat: json");
	lines.push_back("\n");

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

	BOOST_MESSAGE("Done with testing livestatus services...");
}
//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
