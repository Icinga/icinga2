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

#include <time.h>

namespace icinga {

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

	static Timer::CollectionType Timers;

	Timer(void);

	void SetInterval(time_t interval);
	time_t GetInterval(void) const;

	static time_t GetNextCall(void);
	static void CallExpiredTimers(void);

	void Start(void);
	void Stop(void);

	void Reschedule(time_t next);

	boost::signal<void(const Timer::Ptr&)> OnTimerExpired;

private:
	time_t m_Interval; /**< The interval of the timer. */
	time_t m_Next; /**< When the next event should happen. */

	static time_t NextCall; /**< When the next event should happen (for all timers). */

	static void RescheduleTimers(void);

	void Call(void);
};

}

#endif /* TIMER_H */
