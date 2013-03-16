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

#include "base/timer.h"
#include "base/application.h"
#include <boost/bind.hpp>

using namespace icinga;

Timer::TimerSet Timer::m_Timers;
boost::thread Timer::m_Thread;
boost::mutex Timer::m_Mutex;
boost::condition_variable Timer::m_CV;
bool Timer::m_StopThread;

/**
 * Extracts the next timestamp from a Timer.
 *
 * @param wtimer Weak pointer to the timer.
 * @returns The next timestamp
 * @threadsafety Caller must hold Timer::m_Mutex.
 */
double TimerNextExtractor::operator()(const Timer::WeakPtr& wtimer)
{
	Timer::Ptr timer = wtimer.lock();

	if (!timer)
		return 0;

	return timer->m_Next;
}

/**
 * Constructor for the Timer class.
 *
 * @threadsafety Always.
 */
Timer::Timer(void)
	: m_Interval(0), m_Next(0)
{ }

/**
 * Initializes the timer sub-system.
 *
 * @threadsafety Always.
 */
void Timer::Initialize(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	m_StopThread = false;
	m_Thread = boost::thread(boost::bind(&Timer::TimerThreadProc));
}

/**
 * Disables the timer sub-system.
 *
 * @threadsafety Always.
 */
void Timer::Uninitialize(void)
{
	{
		boost::mutex::scoped_lock lock(m_Mutex);
		m_StopThread = true;
		m_CV.notify_all();
	}

	m_Thread.join();
}

/**
 * Calls this timer.
 *
 * @threadsafety Always.
 */
void Timer::Call(void)
{
	ASSERT(!OwnsLock());

	Timer::Ptr self = GetSelf();

	OnTimerExpired(self);

	Reschedule();
}

/**
 * Sets the interval for this timer.
 *
 * @param interval The new interval.
 * @threadsafety Always.
 */
void Timer::SetInterval(double interval)
{
	ASSERT(!OwnsLock());

	boost::mutex::scoped_lock lock(m_Mutex);
	m_Interval = interval;
}

/**
 * Retrieves the interval for this timer.
 *
 * @returns The interval.
 * @threadsafety Always.
 */
double Timer::GetInterval(void) const
{
	ASSERT(!OwnsLock());

	boost::mutex::scoped_lock lock(m_Mutex);
	return m_Interval;
}

/**
 * Registers the timer and starts processing events for it.
 *
 * @threadsafety Always.
 */
void Timer::Start(void)
{
	ASSERT(!OwnsLock());

	{
		boost::mutex::scoped_lock lock(m_Mutex);
		m_Started = true;
	}

	Reschedule();
}

/**
 * Unregisters the timer and stops processing events for it.
 *
 * @threadsafety Always.
 */
void Timer::Stop(void)
{
	ASSERT(!OwnsLock());

	boost::mutex::scoped_lock lock(m_Mutex);

	m_Started = false;
	m_Timers.erase(GetSelf());

	/* Notify the worker thread that we've disabled a timer. */
	m_CV.notify_all();
}

/**
 * Reschedules this timer.
 *
 * @param next The time when this timer should be called again. Use -1 to let
 * 	       the timer figure out a suitable time based on the interval.
 * @threadsafety Always.
 */
void Timer::Reschedule(double next)
{
	ASSERT(!OwnsLock());

	boost::mutex::scoped_lock lock(m_Mutex);

	if (next < 0)
		next = Utility::GetTime() + m_Interval;

	m_Next = next;

	if (m_Started) {
		/* Remove and re-add the timer to update the index. */
		m_Timers.erase(GetSelf());
		m_Timers.insert(GetSelf());

		/* Notify the worker that we've rescheduled a timer. */
		m_CV.notify_all();
	}
}

/**
 * Retrieves when the timer is next due.
 *
 * @returns The timestamp.
 * @threadsafety Always.
 */
double Timer::GetNext(void) const
{
	ASSERT(!OwnsLock());

	boost::mutex::scoped_lock lock(m_Mutex);
	return m_Next;
}

/**
 * Adjusts all timers by adding the specified amount of time to their
 * next scheduled timestamp.
 *
 * @param adjustment The adjustment.
 * @threadsafety Always.
 */
void Timer::AdjustTimers(double adjustment)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	double now = Utility::GetTime();

	typedef boost::multi_index::nth_index<TimerSet, 1>::type TimerView;
	TimerView& idx = boost::get<1>(m_Timers);

	TimerView::iterator it;
	for (it = idx.begin(); it != idx.end(); it++) {
		Timer::Ptr timer = it->lock();

		if (abs(now - (timer->m_Next + adjustment)) <
		    abs(now - timer->m_Next)) {
			timer->m_Next += adjustment;
			m_Timers.erase(timer);
			m_Timers.insert(timer);
		    }
	}

	/* Notify the worker that we've rescheduled some timers. */
	m_CV.notify_all();
}

/**
 * Worker thread proc for Timer objects.
 *
 * @threadsafety Always.
 */
void Timer::TimerThreadProc(void)
{
	for (;;) {
		boost::mutex::scoped_lock lock(m_Mutex);

		typedef boost::multi_index::nth_index<TimerSet, 1>::type NextTimerView;
		NextTimerView& idx = boost::get<1>(m_Timers);

		/* Wait until there is at least one timer. */
		while (idx.empty() && !m_StopThread)
			m_CV.wait(lock);

		if (m_StopThread)
			break;

		NextTimerView::iterator it = idx.begin();
		Timer::Ptr timer = it->lock();

		if (!timer) {
			/* Remove the timer from the list if it's not alive anymore. */
			idx.erase(it);
			continue;
		}

		double wait = timer->m_Next - Utility::GetTime();

		if (wait > 0) {
			/* Make sure the timer we just examined can be destroyed while we're waiting. */
			timer.reset();

			/* Wait for the next timer. */
			m_CV.timed_wait(lock, boost::posix_time::milliseconds(wait * 1000));

			continue;
		}

		/* Remove the timer from the list so it doesn't get called again
		 * until the current call is completed. */
		m_Timers.erase(timer);

		lock.unlock();

		/* Asynchronously call the timer. */
		Application::GetEQ().Post(boost::bind(&Timer::Call, timer));
	}
}
