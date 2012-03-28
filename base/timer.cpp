#include <functional>
#include <algorithm>
#include "i2-base.h"

using namespace icinga;
using std::list;
using std::bind2nd;
using std::equal_to;

static time_t g_NextCall;
list<Timer::WeakRefType> Timer::Timers;

Timer::Timer(void)
{
	m_Interval = 0;
}

time_t Timer::GetNextCall(void)
{
	if (g_NextCall < time(NULL))
		Timer::RescheduleTimers();

	return g_NextCall;
}

void Timer::RescheduleTimers(void)
{
	/* Make sure we wake up at least once every 30 seconds */
	g_NextCall = time(NULL) + 30;

	for (list<Timer::WeakRefType>::iterator i = Timers.begin(); i != Timers.end(); i++) {
		Timer::RefType timer = i->lock();

		if (timer == NULL)
			continue;

		if (timer->m_Next < g_NextCall)
			g_NextCall = timer->m_Next;
	}
}

void Timer::CallExpiredTimers(void)
{
	time_t now;

	time(&now);

	for (list<Timer::WeakRefType>::iterator i = Timers.begin(); i != Timers.end(); ) {
		Timer::RefType timer = Timer::RefType(*i);
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
	for (list<Timer::WeakRefType>::iterator i = Timers.begin(); i != Timers.end(); ) {
		Timer::RefType timer = Timer::RefType(*i);
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
	TimerEventArgs::RefType ea = new_object<TimerEventArgs>();
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
