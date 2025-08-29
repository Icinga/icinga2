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

/**
 * A TimeProvider implementation that allows manual control of time.
 *
 * This is used to test the Timer class without relying on the system clock or real time passing.
 * Instead, time can be advanced manually to instantly provoke timer events in a controlled manner.
 *
 * @ingroup base
 */
struct FakeTimeProvider : TimeProvider
{
	DECLARE_PTR_TYPEDEFS(FakeTimeProvider);

	std::chrono::duration<double> Now() const override
	{
		return std::chrono::duration<double>(now);
	}

	void OnTimerFired()
	{
		timerEvents += 1;
		// Advance time by 1 second for each timer event until we've advanced the total number of seconds we want.
		// However, since the initial time is already slightly in the future to allow the timer to immediately fire,
		// we need to subtract that initial offset (1.0 second) from the total seconds we want to advance.
		if (seconds-1.0 >= 0.0) {
			now += 1.0;
			seconds -= 1.0;
		} else {
			// We're done advancing time, so stop the timer by going back in time.
			now -= 10.0;
		}
	}

	int timerEvents{0}; /**< Counts the number of timer events that have occurred. */
	double seconds{0.0}; /**< The total number of seconds to fake. */
	double now{Utility::GetTime()+1.0};
};

BOOST_AUTO_TEST_CASE(invoke)
{
	FakeTimeProvider::Ptr fakeTime = new FakeTimeProvider();
	fakeTime->seconds = 5.5;

	Timer::Ptr timer = Timer::Create(fakeTime);
	timer->OnTimerExpired.connect([fakeTime](const Timer* const&){ fakeTime->OnTimerFired(); });
	timer->SetInterval(1);

	timer->Start();
	Utility::Sleep(.300); // Give the timer a bit of time to start and fire a few times.
	timer->Stop();

	// At this point, the timer should have fired at least 5 times, but possibly 6 times if the timing was just right.
	BOOST_CHECK_MESSAGE(fakeTime->timerEvents >= 5 && fakeTime->timerEvents <= 6, "Counter: " << fakeTime->timerEvents);
}

BOOST_AUTO_TEST_CASE(scope)
{
	FakeTimeProvider::Ptr fakeTime = new FakeTimeProvider();
	fakeTime->seconds = 5.5;

	Timer::Ptr timer = Timer::Create(fakeTime);
	timer->OnTimerExpired.connect([fakeTime](const Timer* const&){ fakeTime->OnTimerFired(); });
	timer->SetInterval(1);

	timer->Start();
	Utility::Sleep(.300);
	timer.reset();
	Utility::Sleep(.100);

	// Same as above, the timer should have fired at least 5 times, but possibly 6 times if the timing was just right.
	BOOST_CHECK_MESSAGE(fakeTime->timerEvents >= 5 && fakeTime->timerEvents <= 6, "Counter: " << fakeTime->timerEvents);
}

BOOST_AUTO_TEST_SUITE_END()
