/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
	BOOST_TEST_MESSAGE("Uninitializing Application...");
	Application::UninitializeBase();
	IcingaApplication::GetInstance().reset();
}

BOOST_GLOBAL_FIXTURE(IcingaApplicationFixture);
