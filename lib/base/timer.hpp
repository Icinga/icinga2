/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include <boost/signals2.hpp>

namespace icinga {

/**
 * A timer that periodically triggers an event.
 *
 * @ingroup base
 */
class I2_BASE_API Timer : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(Timer);

	Timer(void);
	~Timer(void);

	void SetInterval(double interval);
	double GetInterval(void) const;

	static void AdjustTimers(double adjustment);

	void Start(void);
	void Stop(bool wait = false);

	void Reschedule(double next = -1);
	double GetNext(void) const;

	boost::signals2::signal<void(const Timer::Ptr&)> OnTimerExpired;

	class Holder {
	public:
		Holder(Timer *timer)
			: m_Timer(timer)
		{ }

		inline Timer *GetObject(void) const
		{
			return m_Timer;
		}

		inline double GetNextUnlocked(void) const
		{
			return m_Timer->m_Next;
		}

		operator Timer *(void) const
		{
			return m_Timer;
		}

	private:
		Timer *m_Timer;
	};

private:
	double m_Interval; /**< The interval of the timer. */
	double m_Next; /**< When the next event should happen. */
	bool m_Started; /**< Whether the timer is enabled. */
	bool m_Running; /**< Whether the timer proc is currently running. */

	void Call();
	void InternalReschedule(bool completed, double next = -1);

	static void TimerThreadProc(void);

	static void Initialize(void);
	static void Uninitialize(void);

	friend class Application;
};

}

#endif /* TIMER_H */
