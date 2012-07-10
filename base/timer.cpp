/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "i2-base.h"

using namespace icinga;

Timer::CollectionType Timer::Timers;

/**
 * Constructor for the Timer class.
 */
Timer::Timer(void)
{
	m_Interval = 0;
}

/**
 * Calls expired timers and returned when the next wake-up should happen.
 *
 * @returns Time when the next timer is due.
 */
long Timer::ProcessTimers(void)
{
	long wakeup = 30;

	time_t st;
	time(&st);

	Timer::CollectionType::iterator prev, i;
	for (i = Timers.begin(); i != Timers.end(); ) {
		Timer::Ptr timer = i->lock();

		prev = i;
		i++;

		if (!timer) {
			Timers.erase(prev);
			continue;
		}

		time_t now;
		time(&now);

		if (timer->m_Next <= now) {
			timer->Call();

			/* time may have changed depending on how long the
			 * timer call took - we need to fetch the current time */
			time(&now);

			timer->Reschedule(now + timer->GetInterval());
		}

		assert(timer->m_Next > now);

		if (timer->m_Next - now < wakeup)
			wakeup = timer->m_Next - now;
	}

	assert(wakeup > 0);

	time_t et;
	time(&et);

	stringstream msgbuf;
	msgbuf << "Timers took " << et - st << " seconds";
	Logger::Write(LogDebug, "base", msgbuf.str());

	return wakeup;
}

/**
 * Calls this timer. Note: the timer delegate must not call
 * Disable() on any other timers than the timer that originally
 * invoked the delegate.
 */
void Timer::Call(void)
{
	time_t st;
	time(&st);

	OnTimerExpired(GetSelf());

	time_t et;
	time(&et);

	if (et - st > 3) {
		stringstream msgbuf;
		msgbuf << "Timer call took " << et - st << " seconds.";
		Logger::Write(LogWarning, "base", msgbuf.str());
	}
}

/**
 * Sets the interval for this timer.
 *
 * @param interval The new interval.
 */
void Timer::SetInterval(long interval)
{
	if (interval <= 0)
		throw invalid_argument("interval");

	m_Interval = interval;
}

/**
 * Retrieves the interval for this timer.
 *
 * @returns The interval.
 */
long Timer::GetInterval(void) const
{
	return m_Interval;
}

/**
 * Registers the timer and starts processing events for it.
 */
void Timer::Start(void)
{
	Stop();

	Timers.push_back(GetSelf());

	Reschedule(time(NULL) + m_Interval);
}

/**
 * Unregisters the timer and stops processing events for it.
 */
void Timer::Stop(void)
{
	Timers.remove_if(WeakPtrEqual<Timer>(this));
}

/**
 * Reschedules this timer.
 *
 * @param next The time when this timer should be called again.
 */
void Timer::Reschedule(time_t next)
{
	m_Next = next;
}
