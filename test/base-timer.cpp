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

BOOST_AUTO_TEST_CASE(invoke)
{
	int counter = 0;

	Timer::Ptr timer = Timer::Create();
	timer->OnTimerExpired.connect([&counter](const Timer* const&) { counter++; });
	timer->SetInterval(.1);

	timer->Start();
	Utility::Sleep(.55);
	timer->Stop();

	// At this point, the timer should have fired exactly 5 times (0.5 / 0.1) and the sixth time
	// should not have fired yet as we stopped the timer after 0.55 seconds (0.6 would be needed).
	BOOST_CHECK_EQUAL(5, counter);
}

BOOST_AUTO_TEST_CASE(scope)
{
	int counter = 0;

	Timer::Ptr timer = Timer::Create();
	timer->OnTimerExpired.connect([&counter](const Timer* const&) { counter++; });
	timer->SetInterval(.1);

	timer->Start();
	Utility::Sleep(.55);
	timer.reset();
	Utility::Sleep(.1);

	// At this point, the timer should have fired exactly 5 times (0.5 / 0.1) and the sixth time
	// should not have fired yet as we destroyed the timer after 0.55 seconds (0.6 would be needed),
	// and even if we wait another 0.1 seconds after its destruction, it should not fire again.
	BOOST_CHECK_EQUAL(5, counter);
}

BOOST_AUTO_TEST_SUITE_END()
