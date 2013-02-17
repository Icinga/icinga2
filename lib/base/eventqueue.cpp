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

/**
 * @threadsafety Always.
 */
EventQueue::EventQueue(void)
	: m_Stopped(false)
{ }

/**
 * @threadsafety Always.
 */
EventQueue::~EventQueue(void)
{
	Stop();
}

/**
 * @threadsafety Always.
 */
void EventQueue::Stop(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	m_Stopped = true;
	m_CV.notify_all();
}

/**
 * Spawns worker threads and waits for them to complete.
 *
 * @threadsafety Always.
 */
void EventQueue::Run(void)
{
	thread_group threads;

	int cpus = thread::hardware_concurrency();

	if (cpus == 0)
		cpus = 4;

	for (int i = 0; i < cpus * 4; i++)
		threads.create_thread(boost::bind(&EventQueue::QueueThreadProc, this));

	threads.join_all();
}

/**
 * Waits for events and processes them.
 *
 * @threadsafety Always.
 */
void EventQueue::QueueThreadProc(void)
{
	while (!m_Stopped) {
		vector<Callback> events;

		{
			boost::mutex::scoped_lock lock(m_Mutex);

			while (m_Events.empty() && !m_Stopped)
				m_CV.wait(lock);

			events.swap(m_Events);
		}

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
	}
}

/**
 * Appends an event to the event queue. Events will be processed in FIFO order.
 *
 * @param callback The callback function for the event.
 * @threadsafety Always.
 */
void EventQueue::Post(const EventQueue::Callback& callback)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	m_Events.push_back(callback);
	m_CV.notify_all();
}
