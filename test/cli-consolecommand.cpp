// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "cli/consolecommand.hpp"
#include "test/test-ctest.hpp"
#include <BoostTestTargetConfig.h>
#include <boost/program_options.hpp>
#include <iterator>

using namespace icinga;
namespace po = boost::program_options;

BOOST_AUTO_TEST_SUITE(cli_consolecommand)

BOOST_AUTO_TEST_CASE(tls_options)
{
	ConsoleCommand command;
	po::options_description visible;
	po::options_description hidden;
	command.InitParameters(visible, hidden);

	const char *args[] = {
		"--connect", "https://example.test:5665/",
		"--cert", "/tmp/client.crt",
		"--key", "/tmp/client.key",
		"--ca", "/tmp/ca.crt"
	};
	po::variables_map vm;
	po::store(po::command_line_parser(std::size(args), args).options(visible).run(), vm);
	po::notify(vm);

	BOOST_CHECK_EQUAL(vm["cert"].as<std::string>(), "/tmp/client.crt");
	BOOST_CHECK_EQUAL(vm["key"].as<std::string>(), "/tmp/client.key");
	BOOST_CHECK_EQUAL(vm["ca"].as<std::string>(), "/tmp/ca.crt");
}

BOOST_AUTO_TEST_SUITE_END()
