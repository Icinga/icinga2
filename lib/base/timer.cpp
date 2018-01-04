/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/timer.hpp"
#include "base/debug.hpp"
#include "base/utility.hpp"
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/key_extractors.hpp>
#include <thread>

using namespace icinga;

namespace icinga {

class TimerHolder {
public:
	TimerHolder(Timer *timer)
		: m_Timer(timer)
	{ }

	inline Timer *GetObject() const
	{
		return m_Timer;
	}

	inline double GetNextUnlocked() const
	{
		return m_Timer->m_Next;
	}

	operator Timer *() const
	{
		return m_Timer;
	}

private:
	Timer *m_Timer;
};

}

typedef boost::multi_index_container<
	TimerHolder,
	boost::multi_index::indexed_by<
		boost::multi_index::ordered_unique<boost::multi_index::const_mem_fun<TimerHolder, Timer *, &TimerHolder::GetObject> >,
		boost::multi_index::ordered_non_unique<boost::multi_index::const_mem_fun<TimerHolder, double, &TimerHolder::GetNextUnlocked> >
	>
> TimerSet;

static boost::mutex l_TimerMutex;
static boost::condition_variable l_TimerCV;
static std::thread l_TimerThread;
static bool l_StopTimerThread;
static TimerSet l_Timers;
static int l_AliveTimers;

/**
 * Constructor for the Timer class.
 */
Timer::Timer()
	: m_Interval(0), m_Next(0), m_Started(false), m_Running(false)
{ }

/**
 * Destructor for the Timer class.
 */
Timer::~Timer()
{
	Stop(true);
}

void Timer::Uninitialize()
{
	{
		boost::mutex::scoped_lock lock(l_TimerMutex);
		l_StopTimerThread = true;
		l_TimerCV.notify_all();
	}

	if (l_TimerThread.joinable())
		l_TimerThread.join();
}

/**
 * Calls this timer.
 */
void Timer::Call()
{
	try {
		OnTimerExpired(Timer::Ptr(this));
	} catch (...) {
		InternalReschedule(true);

		throw;
	}

	InternalReschedule(true);
}

/**
 * Sets the interval for this timer.
 *
 * @param interval The new interval.
 */
void Timer::SetInterval(double interval)
{
	boost::mutex::scoped_lock lock(l_TimerMutex);
	m_Interval = interval;
}

/**
 * Retrieves the interval for this timer.
 *
 * @returns The interval.
 */
double Timer::GetInterval() const
{
	boost::mutex::scoped_lock lock(l_TimerMutex);
	return m_Interval;
}

/**
 * Registers the timer and starts processing events for it.
 */
void Timer::Start()
{
	{
		boost::mutex::scoped_lock lock(l_TimerMutex);
		m_Started = true;

		if (l_AliveTimers++ == 0) {
			l_StopTimerThread = false;
			l_TimerThread = std::thread(&Timer::TimerThreadProc);
		}
	}

	InternalReschedule(false);
}

/**
 * Unregisters the timer and stops processing events for it.
 */
void Timer::Stop(bool wait)
{
	if (l_StopTimerThread)
		return;

	boost::mutex::scoped_lock lock(l_TimerMutex);

	if (m_Started && --l_AliveTimers == 0) {
		l_StopTimerThread = true;
		l_TimerCV.notify_all();

		lock.unlock();

		if (l_TimerThread.joinable() && l_TimerThread.get_id() != std::this_thread::get_id())
			l_TimerThread.join();

		lock.lock();
	}

	m_Started = false;
	l_Timers.erase(this);

	/* Notify the worker thread that we've disabled a timer. */
	l_TimerCV.notify_all();

	while (wait && m_Running)
		l_TimerCV.wait(lock);
}

void Timer::Reschedule(double next)
{
	InternalReschedule(false, next);
}

/**
 * Reschedules this timer.
 *
 * @param completed Whether the timer has just completed its callback.
 * @param next The time when this timer should be called again. Use -1 to let
 *        the timer figure out a suitable time based on the interval.
 */
void Timer::InternalReschedule(bool completed, double next)
{
	boost::mutex::scoped_lock lock(l_TimerMutex);

	if (completed)
		m_Running = false;

	if (next < 0) {
		/* Don't schedule the next call if this is not a periodic timer. */
		if (m_Interval <= 0)
			return;

		next = Utility::GetTime() + m_Interval;
	}

	m_Next = next;

	if (m_Started && !m_Running) {
		/* Remove and re-add the timer to update the index. */
		l_Timers.erase(this);
		l_Timers.insert(this);

		/* Notify the worker that we've rescheduled a timer. */
		l_TimerCV.notify_all();
	}
}

/**
 * Retrieves when the timer is next due.
 *
 * @returns The timestamp.
 */
double Timer::GetNext() const
{
	boost::mutex::scoped_lock lock(l_TimerMutex);
	return m_Next;
}

/**
 * Adjusts all timers by adding the specified amount of time to their
 * next scheduled timestamp.
 *
 * @param adjustment The adjustment.
 */
void Timer::AdjustTimers(double adjustment)
{
	boost::mutex::scoped_lock lock(l_TimerMutex);

	double now = Utility::GetTime();

	typedef boost::multi_index::nth_index<TimerSet, 1>::type TimerView;
	TimerView& idx = boost::get<1>(l_Timers);

	std::vector<Timer *> timers;

	for (Timer *timer : idx) {
		if (std::fabs(now - (timer->m_Next + adjustment)) <
			std::fabs(now - timer->m_Next)) {
			timer->m_Next += adjustment;
			timers.push_back(timer);
		}
	}

	for (Timer *timer : timers) {
		l_Timers.erase(timer);
		l_Timers.insert(timer);
	}

	/* Notify the worker that we've rescheduled some timers. */
	l_TimerCV.notify_all();
}

/**
 * Worker thread proc for Timer objects.
 */
void Timer::TimerThreadProc()
{
	Utility::SetThreadName("Timer Thread");

	for (;;) {
		boost::mutex::scoped_lock lock(l_TimerMutex);

		typedef boost::multi_index::nth_index<TimerSet, 1>::type NextTimerView;
		NextTimerView& idx = boost::get<1>(l_Timers);

		/* Wait until there is at least one timer. */
		while (idx.empty() && !l_StopTimerThread)
			l_TimerCV.wait(lock);

		if (l_StopTimerThread)
			break;

		auto it = idx.begin();
		Timer *timer = *it;

		double wait = timer->m_Next - Utility::GetTime();

		if (wait > 0.01) {
			/* Wait for the next timer. */
			l_TimerCV.timed_wait(lock, boost::posix_time::milliseconds(wait * 1000));

			continue;
		}

		Timer::Ptr ptimer = timer;

		/* Remove the timer from the list so it doesn't get called again
		 * until the current call is completed. */
		l_Timers.erase(timer);

		timer->m_Running = true;

		lock.unlock();

		/* Asynchronously call the timer. */
		Utility::QueueAsyncCallback(std::bind(&Timer::Call, ptimer));
	}
}
