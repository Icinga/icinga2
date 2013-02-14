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

vector<Event> Event::m_Events;
condition_variable Event::m_EventAvailable;
boost::mutex Event::m_Mutex;

/**
 * Constructor for the Event class
 *
 * @param callback The callback function for the new event object.
 */
Event::Event(const Event::Callback& callback)
	: m_Callback(callback)
{ }

/**
 * Waits for events using the specified timeout value and processes
 * them.
 *
 * @param timeout The wait timeout.
 */
void Event::ProcessEvents(millisec timeout)
{
	vector<Event> events;

	assert(Application::IsMainThread());

	Application::GetMutex().unlock();

	{
		boost::mutex::scoped_lock lock(m_Mutex);

		while (m_Events.empty()) {
			if (!m_EventAvailable.timed_wait(lock, timeout))
				return;
		}

		events.swap(m_Events);
	}

	Application::GetMutex().lock();

	BOOST_FOREACH(const Event& ev, events) {
		double st = Utility::GetTime();

		ev.m_Callback();

		double et = Utility::GetTime();

		if (et - st > 1.0) {
			stringstream msgbuf;
			msgbuf << "Event call took " << et - st << " seconds.";
			Logger::Write(LogWarning, "base", msgbuf.str());
		}
	}
}

/**
 * Appends an event to the event queue. Events will be processed in FIFO
 * order on the main thread.
 *
 * @param callback The callback function for the event.
 */
void Event::Post(const Event::Callback& callback)
{
	if (Application::IsMainThread()) {
		callback();
		return;
	}

	Event ev(callback);

	{
		boost::mutex::scoped_lock lock(m_Mutex);
		m_Events.push_back(ev);
		m_EventAvailable.notify_all();
	}
}

