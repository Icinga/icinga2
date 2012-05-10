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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.            *
 ******************************************************************************/

#ifndef TIMER_H
#define TIMER_H

#include <time.h>

namespace icinga {

struct I2_BASE_API TimerEventArgs : public EventArgs
{
	typedef shared_ptr<TimerEventArgs> Ptr;
	typedef weak_ptr<TimerEventArgs> WeakPtr;

	EventArgs UserArgs;
};

class I2_BASE_API Timer : public Object
{
private:
	EventArgs m_UserArgs;
	unsigned int m_Interval;
	time_t m_Next;

	static time_t NextCall;

	static void RescheduleTimers(void);

	void Call(void);

public:
	typedef shared_ptr<Timer> Ptr;
	typedef weak_ptr<Timer> WeakPtr;

	typedef list<Timer::WeakPtr> CollectionType;

	static Timer::CollectionType Timers;

	Timer(void);

	void SetInterval(unsigned int interval);
	unsigned int GetInterval(void) const;

	void SetUserArgs(const EventArgs& userArgs);
	EventArgs GetUserArgs(void) const;

	static time_t GetNextCall(void);
	static void CallExpiredTimers(void);

	void Start(void);
	void Stop(void);

	void Reschedule(time_t next);

	Event<TimerEventArgs> OnTimerExpired;
};

}

#endif /* TIMER_H */
