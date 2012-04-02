#include <functional>
#include <algorithm>
#include "i2-base.h"

using namespace icinga;

time_t Timer::NextCall;
list<Timer::WeakPtr> Timer::Timers;

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

	for (list<Timer::WeakPtr>::iterator i = Timers.begin(); i != Timers.end(); i++) {
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

	for (list<Timer::WeakPtr>::iterator i = Timers.begin(); i != Timers.end(); ) {
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
	for (list<Timer::WeakPtr>::iterator i = Timers.begin(); i != Timers.end(); ) {
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
	TimerEventArgs::Ptr ea = new_object<TimerEventArgs>();
	ea->Source = shared_from_this();
	ea->UserArgs = m_UserArgs;
	OnTimerExpired(ea);
}

void Timer::SetInterval(unsigned int interval)
{
	m_Interval = interval;
}

unsigned int Timer::GetInterval(void) const
{
	return m_Interval;
}

void Timer::Start(void)
{
	Stop();

	Timers.push_front(static_pointer_cast<Timer>(shared_from_this()));

	Reschedule(time(NULL) + m_Interval);
}

void Timer::Stop(void)
{
	Timers.remove_if(weak_ptr_eq_raw<Timer>(this));
}

void Timer::Reschedule(time_t next)
{
	m_Next = next;
	RescheduleTimers();
}
