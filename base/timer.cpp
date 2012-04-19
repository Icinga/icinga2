#include <functional>
#include <algorithm>
#include "i2-base.h"

using namespace icinga;

time_t Timer::NextCall;
Timer::CollectionType Timer::Timers;

Timer::Timer(void)
{
	m_Interval = 0;
}

time_t Timer::GetNextCall(void)
{
	if (NextCall < time(NULL))
		Timer::RescheduleTimers();

	return NextCall;
}

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

void Timer::CallExpiredTimers(void)
{
	time_t now;

	time(&now);

	for (Timer::CollectionType::iterator i = Timers.begin(); i != Timers.end(); ) {
		Timer::Ptr timer = Timer::Ptr(*i);
		i++;

		if (timer == NULL)
			continue;

		if (timer->m_Next <= now) {
			timer->Call();
			timer->Reschedule(now + timer->GetInterval());
		}
	}
}

void Timer::StopAllTimers(void)
{
	for (Timer::CollectionType::iterator i = Timers.begin(); i != Timers.end(); ) {
		Timer::Ptr timer = i->lock();

		i++;

		if (timer == NULL)
			continue;

		timer->Stop();
	}
}

/* Note: the timer delegate must not call Disable() on any other timers than
 * the timer that originally invoked the delegate */
void Timer::Call(void)
{
	TimerEventArgs tea;
	tea.Source = shared_from_this();
	tea.UserArgs = m_UserArgs;
	OnTimerExpired(tea);
}

void Timer::SetInterval(unsigned int interval)
{
	m_Interval = interval;
}

unsigned int Timer::GetInterval(void) const
{
	return m_Interval;
}

void Timer::SetUserArgs(const EventArgs& userArgs)
{
	m_UserArgs = userArgs;
}


EventArgs Timer::GetUserArgs(void) const
{
	return m_UserArgs;
}

void Timer::Start(void)
{
	Timers.insert(static_pointer_cast<Timer>(shared_from_this()));

	Reschedule(time(NULL) + m_Interval);
}

void Timer::Stop(void)
{
	Timer::Ptr self = static_pointer_cast<Timer>(shared_from_this());
	Timer::CollectionType::iterator i = Timers.find(self);

	if (i != Timers.end())
		Timers.erase(i);
}

void Timer::Reschedule(time_t next)
{
	m_Next = next;
	RescheduleTimers();
}
