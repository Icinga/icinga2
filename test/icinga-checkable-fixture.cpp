// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "cli/daemonutility.hpp"
#include "base/application.hpp"
#include "base/loader.hpp"
#include <BoostTestTargetConfig.h>
#include <fstream>

using namespace icinga;

struct IcingaCheckableFixture
{
	IcingaCheckableFixture()
	{
		BOOST_TEST_MESSAGE("setup running Icinga 2 core");

		Application::InitializeBase();
	}

	~IcingaCheckableFixture()
	{
		BOOST_TEST_MESSAGE("cleanup Icinga 2 core");
		Application::UninitializeBase();
	}
};

BOOST_GLOBAL_FIXTURE(IcingaCheckableFixture);

