/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icingaapplication-fixture.hpp"

using namespace icinga;

static bool IcingaInitialized = false;

IcingaApplicationFixture::IcingaApplicationFixture()
{
	// This limits the number of threads in the thread pool, but does not limit the number of concurrent checks.
	// By default, we'll have only 2 threads in the pool, which is too low for some heavy checker tests. So, we
	// set the concurrency to the number of hardware threads available on the system and the global thread pool
	// size will be the double of that (see threadpool.cpp).
	Configuration::Concurrency = std::thread::hardware_concurrency();

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
