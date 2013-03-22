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

#include "base/eventqueue.h"
#include "base/logger_fwd.h"
#include "base/convert.h"
#include "base/utility.h"
#include <sstream>
#include <boost/bind.hpp>
#include <boost/exception/diagnostic_information.hpp>

using namespace icinga;

EventQueue::EventQueue(void)
	: m_Stopped(false)
{
	unsigned int threads = boost::thread::hardware_concurrency();

	if (threads == 0)
		threads = 1;

	threads *= 8;

	for (unsigned int i = 0; i < threads; i++)
		m_Threads.create_thread(boost::bind(&EventQueue::QueueThreadProc, this));

	boost::thread reportThread(boost::bind(&EventQueue::ReportThreadProc, this));
	reportThread.detach();
}

EventQueue::~EventQueue(void)
{
	Stop();
	Join();
}

void EventQueue::Stop(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	m_Stopped = true;
	m_CV.notify_all();
}

/**
 * Waits for all worker threads to finish.
 */
void EventQueue::Join(void)
{
	m_Threads.join_all();
}

/**
 * Waits for events and processes them.
 */
void EventQueue::QueueThreadProc(void)
{
	for (;;) {
		Callback event;

		{
			boost::mutex::scoped_lock lock(m_Mutex);

			while (m_Events.empty() && !m_Stopped)
				m_CV.wait(lock);

			if (m_Events.empty() && m_Stopped)
				break;

			event = m_Events.top();
			m_Events.pop();
		}

#ifdef _DEBUG
		double st = Utility::GetTime();

#	ifdef RUSAGE_THREAD
		struct rusage usage_start, usage_end;

		(void) getrusage(RUSAGE_THREAD, &usage_start);
#	endif /* RUSAGE_THREAD */
#endif /* _DEBUG */

		try {
			event();
		} catch (const std::exception& ex) {
			std::ostringstream msgbuf;
			msgbuf << "Exception thrown in event handler: " << std::endl
			       << boost::diagnostic_information(ex);

			Log(LogCritical, "base", msgbuf.str());
		} catch (...) {
			Log(LogCritical, "base", "Exception of unknown type thrown in event handler.");
		}

#ifdef _DEBUG
		double et = Utility::GetTime();
#	ifdef RUSAGE_THREAD
		(void) getrusage(RUSAGE_THREAD, &usage_end);

		double duser = (usage_end.ru_utime.tv_sec - usage_start.ru_utime.tv_sec) +
		    (usage_end.ru_utime.tv_usec - usage_start.ru_utime.tv_usec) / 1000000.0;

		double dsys = (usage_end.ru_stime.tv_sec - usage_start.ru_stime.tv_sec) +
		    (usage_end.ru_stime.tv_usec - usage_start.ru_stime.tv_usec) / 1000000.0;

		double dwait = (et - st) - (duser + dsys);

		int dminfaults = usage_end.ru_minflt - usage_start.ru_minflt;
		int dmajfaults = usage_end.ru_majflt - usage_start.ru_majflt;

		int dvctx = usage_end.ru_nvcsw - usage_start.ru_nvcsw;
		int divctx = usage_end.ru_nivcsw - usage_start.ru_nivcsw;
#	endif /* RUSAGE_THREAD */
		if (et - st > 0.5) {
			std::ostringstream msgbuf;
#	ifdef RUSAGE_THREAD
			msgbuf << "Event call took user:" << duser << "s, system:" << dsys << "s, wait:" << dwait << "s, minor_faults:" << dminfaults << ", major_faults:" << dmajfaults << ", voluntary_csw:" << dvctx << ", involuntary_csw:" << divctx;
#	else
			msgbuf << "Event call took " << (et - st) << "s";
#	endif /* RUSAGE_THREAD */

			Log(LogWarning, "base", msgbuf.str());
		}
#endif /* _DEBUG */
	}
}

/**
 * Appends an event to the event queue. Events will be processed in FIFO order.
 *
 * @param callback The callback function for the event.
 */
void EventQueue::Post(const EventQueue::Callback& callback)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	m_Events.push(callback);
	m_CV.notify_one();
}

void EventQueue::ReportThreadProc(void)
{
	for (;;) {
		Utility::Sleep(5);

		int pending;

		{
			boost::mutex::scoped_lock lock(m_Mutex);
			pending = m_Events.size();
		}

		Log(LogInformation, "base", "Pending tasks: " + Convert::ToString(pending));
	}
}
