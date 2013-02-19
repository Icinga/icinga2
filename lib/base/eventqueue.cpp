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
{
	int thread_count = thread::hardware_concurrency();

	if (thread_count < 4)
		thread_count = 4;

	thread_count *= 4;

	for (int i = 0; i < thread_count; i++)
		m_Threads.create_thread(boost::bind(&EventQueue::QueueThreadProc, this));
}

/**
 * @threadsafety Always.
 */
EventQueue::~EventQueue(void)
{
	Stop();
	Join();
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
 * Waits for all worker threads to finish.
 *
 * @threadsafety Always.
 */
void EventQueue::Join(void)
{
	m_Threads.join_all();
}

/**
 * Waits for events and processes them.
 *
 * @threadsafety Always.
 */
void EventQueue::QueueThreadProc(void)
{
	for (;;) {
		vector<Callback> events;

		{
			boost::mutex::scoped_lock lock(m_Mutex);

			while (m_Events.empty() && !m_Stopped)
				m_CV.wait(lock);

			if (m_Stopped)
				break;

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
	m_CV.notify_one();
}
