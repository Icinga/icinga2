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

EventQueue::EventQueue(void)
	: m_Stopped(false)
{ }

boost::thread::id EventQueue::GetOwner(void) const
{
	return m_Owner;
}

void EventQueue::SetOwner(boost::thread::id owner)
{
	m_Owner = owner;
}

void EventQueue::Stop(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	m_Stopped = true;
	m_EventAvailable.notify_all();
}

/**
 * Waits for events using the specified timeout value and processes
 * them.
 *
 * @param mtx The mutex that should be unlocked while waiting. Caller
 * 	      must have this mutex locked.
 * @param timeout The wait timeout.
 * @returns false if the queue has been stopped, true otherwise.
 */
bool EventQueue::ProcessEvents(boost::mutex& mtx, millisec timeout)
{
	vector<Callback> events;

	mtx.unlock();

	{
		boost::mutex::scoped_lock lock(m_Mutex);

		while (m_Events.empty() && !m_Stopped) {
			if (!m_EventAvailable.timed_wait(lock, timeout)) {
				mtx.lock();

				return !m_Stopped;
			}
		}

		events.swap(m_Events);
	}

	mtx.lock();

	BOOST_FOREACH(const Callback& ev, events) {
		double st = Utility::GetTime();

		ev();

		double et = Utility::GetTime();

		if (et - st > 1.0) {
			stringstream msgbuf;
			msgbuf << "Event call took " << et - st << " seconds.";
			Logger::Write(LogWarning, "base", msgbuf.str());
		}
	}

	return !m_Stopped;
}

/**
 * Appends an event to the event queue. Events will be processed in FIFO
 * order on the main thread.
 *
 * @param callback The callback function for the event.
 */
void EventQueue::Post(const EventQueue::Callback& callback)
{
	if (boost::this_thread::get_id() == m_Owner) {
		callback();
		return;
	}

	{
		boost::mutex::scoped_lock lock(m_Mutex);
		m_Events.push_back(callback);
		m_EventAvailable.notify_all();
	}
}
