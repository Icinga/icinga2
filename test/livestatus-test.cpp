/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE icinga2_test

#include "cli/daemonutility.hpp"
#include "config/configcompiler.hpp"
#include "config/configitem.hpp"
#include "base/application.hpp"
#include "base/loader.hpp"
#include <fstream>
#include <BoostTestTargetConfig.h>

using namespace icinga;

struct LivestatusFixture
{
	LivestatusFixture(void)
	    : TestConfig("test-config.conf")
	{
		BOOST_TEST_MESSAGE("setup global config fixture");

		Application::InitializeBase();

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

		BOOST_TEST_MESSAGE( "Preparing config objects...");

		/* start the Icinga application and load the configuration */
		Application::DeclareSysconfDir("etc");
		Application::DeclareLocalStateDir("var");

		ActivationScope ascope;

		Loader::LoadExtensionLibrary("icinga");
		Loader::LoadExtensionLibrary("methods"); //loaded by ITL

		std::vector<std::string> configs;
		configs.push_back(TestConfig);

		std::vector<ConfigItem::Ptr> newItems;

		DaemonUtility::LoadConfigFiles(configs, newItems, "icinga2.debug", "icinga2.vars");

		/* ignore config errors */
		WorkQueue upq;
		ConfigItem::ActivateItems(upq, newItems);
	}

	~LivestatusFixture(void)
	{
		BOOST_TEST_MESSAGE("cleanup global config fixture");

		unlink(TestConfig.CStr());

		Application::UninitializeBase();
	}

	String TestConfig;
};

BOOST_GLOBAL_FIXTURE(LivestatusFixture);

