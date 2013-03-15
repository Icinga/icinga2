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

#ifndef TIMER_H
#define TIMER_H

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

namespace icinga {

class Timer;

/**
 * @ingroup base
 */
struct TimerNextExtractor
{
	typedef double result_type;

	double operator()(const weak_ptr<Timer>& wtimer);
};

/**
 * A timer that periodically triggers an event.
 *
 * @ingroup base
 */
class I2_BASE_API Timer : public Object
{
public:
	typedef shared_ptr<Timer> Ptr;
	typedef weak_ptr<Timer> WeakPtr;

	typedef list<Timer::WeakPtr> CollectionType;

	Timer(void);

	void SetInterval(double interval);
	double GetInterval(void) const;

	static void AdjustTimers(double adjustment);

	void Start(void);
	void Stop(void);

	void Reschedule(double next = -1);
	double GetNext(void) const;

	signals2::signal<void(const Timer::Ptr&)> OnTimerExpired;

	static void Initialize(void);
	static void Uninitialize(void);

private:
	double m_Interval; /**< The interval of the timer. */
	double m_Next; /**< When the next event should happen. */
	bool m_Started; /**< Whether the timer is enabled. */

	typedef multi_index_container<
		Timer::WeakPtr,
		indexed_by<
			ordered_unique<identity<Timer::WeakPtr> >,
			ordered_non_unique<TimerNextExtractor>
		>
	> TimerSet;

	static boost::mutex m_Mutex;
	static boost::condition_variable m_CV;
	static boost::thread m_Thread;
	static bool m_StopThread;
	static TimerSet m_Timers;

	void Call();

	static void TimerThreadProc(void);

	friend struct TimerNextExtractor;
};

}

#endif /* TIMER_H */
