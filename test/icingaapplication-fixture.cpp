// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "icingaapplication-fixture.hpp"

using namespace icinga;

static bool IcingaInitialized = false;

IcingaApplicationFixture::IcingaApplicationFixture()
{
	if (!IcingaInitialized)
		InitIcingaApplication();
}

void IcingaApplicationFixture::InitIcingaApplication()
{
	BOOST_TEST_MESSAGE("Initializing Application...");
	Application::InitializeBase();

	BOOST_TEST_MESSAGE("Initializing IcingaApplication...");
	IcingaApplication::Ptr appInst = new IcingaApplication();
	static_pointer_cast<ConfigObject>(appInst)->OnConfigLoaded();

	IcingaInitialized = true;
}

IcingaApplicationFixture::~IcingaApplicationFixture()
{
	IcingaApplication::GetInstance().reset();
}

BOOST_GLOBAL_FIXTURE(IcingaApplicationFixture);
