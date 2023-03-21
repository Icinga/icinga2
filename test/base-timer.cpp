/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/timer.hpp"
#include "base/utility.hpp"
#include "base/application.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_timer)

BOOST_AUTO_TEST_CASE(construct)
{
	Timer::Ptr timer = Timer::Create();
	BOOST_CHECK(timer);
}

BOOST_AUTO_TEST_CASE(interval)
{
	Timer::Ptr timer = Timer::Create();
	timer->SetInterval(1.5);
	BOOST_CHECK(timer->GetInterval() == 1.5);
}

int counter = 0;

static void Callback(const Timer * const&)
{
	counter++;
}

BOOST_AUTO_TEST_CASE(invoke)
{
	Timer::Ptr timer = Timer::Create();
	timer->OnTimerExpired.connect(&Callback);
	timer->SetInterval(1);

	counter = 0;
	timer->Start();
	Utility::Sleep(5.5);
	timer->Stop();

	BOOST_CHECK(counter >= 4 && counter <= 6);
}

BOOST_AUTO_TEST_CASE(scope)
{
	Timer::Ptr timer = Timer::Create();
	timer->OnTimerExpired.connect(&Callback);
	timer->SetInterval(1);

	counter = 0;
	timer->Start();
	Utility::Sleep(5.5);
	timer.reset();
	Utility::Sleep(5.5);

	BOOST_CHECK(counter >= 4 && counter <= 6);
}

BOOST_AUTO_TEST_SUITE_END()
