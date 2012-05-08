#include <functional>
#include <algorithm>
#include "i2-base.h"

using namespace icinga;

time_t Timer::NextCall;
Timer::CollectionType Timer::Timers;

/**
 * Constructor for the Timer class.
 */
Timer::Timer(void)
{
	m_Interval = 0;
}

/**
 * GetNextCall
 *
 * Retrieves when the next timer is due.
 *
 * @returns Time when the next timer is due.
 */
time_t Timer::GetNextCall(void)
{
	if (NextCall < time(NULL))
		Timer::RescheduleTimers();

	return NextCall;
}

/**
 * RescheduleTimers
 *
 * Reschedules all timers, thereby updating the NextCall
 * timestamp used by the GetNextCall() function.
 */
void Timer::RescheduleTimers(void)
{
	/* Make sure we wake up at least once every 30 seconds */
	NextCall = time(NULL) + 30;

	for (Timer::CollectionType::iterator i = Timers.begin(); i != Timers.end(); i++) {
		Timer::Ptr timer = i->lock();

		if (timer == NULL)
			continue;

		if (timer->m_Next < NextCall)
			NextCall = timer->m_Next;
	}
}

/**
 * CallExpiredTimers
 *
 * Calls all expired timers and reschedules them.
 */
void Timer::CallExpiredTimers(void)
{
	time_t now;

	time(&now);

	Timer::CollectionType::iterator prev, i;
	for (i = Timers.begin(); i != Timers.end(); ) {
		Timer::Ptr timer = i->lock();

		prev = i;
		i++;

		if (!timer) {
			Timers.erase(prev);
			continue;
		}

		if (timer->m_Next <= now) {
			timer->Call();
			timer->Reschedule(now + timer->GetInterval());
		}
	}
}

/**
 * Call
 *
 * Calls this timer. Note: the timer delegate must not call
 * Disable() on any other timers than the timer that originally
 * invoked the delegate.
 */
void Timer::Call(void)
{
	TimerEventArgs tea;
	tea.Source = shared_from_this();
	tea.UserArgs = m_UserArgs;
	OnTimerExpired(tea);
}

/**
 * SetInterval
 *
 * Sets the interval for this timer.
 *
 * @param interval The new interval.
 */
void Timer::SetInterval(unsigned int interval)
{
	m_Interval = interval;
}

/**
 * GetInterval
 *
 * Retrieves the interval for this timer.
 *
 * @returns The interval.
 */
unsigned int Timer::GetInterval(void) const
{
	return m_Interval;
}

/**
 * SetUserArgs
 *
 * Sets user arguments for the timer callback.
 *
 * @param userArgs The user arguments.
 */
void Timer::SetUserArgs(const EventArgs& userArgs)
{
	m_UserArgs = userArgs;
}

/**
 * GetUserArgs
 *
 * Retrieves the user arguments for the timer callback.
 *
 * @returns The user arguments.
 */
EventArgs Timer::GetUserArgs(void) const
{
	return m_UserArgs;
}

/**
 * Start
 *
 * Registers the timer and starts processing events for it.
 */
void Timer::Start(void)
{
	Timers.push_back(static_pointer_cast<Timer>(shared_from_this()));

	Reschedule(time(NULL) + m_Interval);
}

/**
 * Stop
 *
 * Unregisters the timer and stops processing events for it.
 */
void Timer::Stop(void)
{
	Timers.remove_if(weak_ptr_eq_raw<Timer>(this));
}

/**
 * Reschedule
 *
 * Reschedules this timer.
 *
 * @param next The time when this timer should be called again.
 */
void Timer::Reschedule(time_t next)
{
	m_Next = next;
	RescheduleTimers();
}
