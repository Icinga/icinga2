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
#include "base/application.hpp"
#include "base/loader.hpp"
#include <BoostTestTargetConfig.h>
#include <fstream>

using namespace icinga;

struct IcingaCheckableFixture
{
	IcingaCheckableFixture(void)
	{
		BOOST_TEST_MESSAGE("setup running Icinga 2 core");

		Application::InitializeBase();

		/* start the Icinga application and load the configuration */
		Application::DeclareSysconfDir("etc");
		Application::DeclareLocalStateDir("var");

		ActivationScope ascope;

		Loader::LoadExtensionLibrary("icinga");
		Loader::LoadExtensionLibrary("methods"); //loaded by ITL

		std::vector<std::string> configs;
		std::vector<ConfigItem::Ptr> newItems;

		DaemonUtility::LoadConfigFiles(configs, newItems, "icinga2.debug", "icinga2.vars");

		/* ignore config errors */
		WorkQueue upq;
		ConfigItem::ActivateItems(upq, newItems);
	}

	~IcingaCheckableFixture(void)
	{
		BOOST_TEST_MESSAGE("cleanup Icinga 2 core");
		Application::UninitializeBase();
	}
};

BOOST_GLOBAL_FIXTURE(IcingaCheckableFixture);

