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
#include <iostream>
#include <boost/bind.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

EventQueue::EventQueue(void)
	: m_Stopped(false), m_ThreadDeaths(0), m_WaitTime(0), m_ServiceTime(0), m_TaskCount(0)
{
	for (int i = 0; i < sizeof(m_ThreadStates) / sizeof(m_ThreadStates[0]); i++)
		m_ThreadStates[i] = ThreadDead;

	for (int i = 0; i < 2; i++)
		SpawnWorker();

	boost::thread managerThread(boost::bind(&EventQueue::ManagerThreadProc, this));
	managerThread.detach();
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
	boost::mutex::scoped_lock lock(m_Mutex);

	while (!m_Stopped || !m_Events.empty()) {
		lock.unlock();
		Utility::Sleep(0.5);
		lock.lock();
	}
}

/**
 * Waits for events and processes them.
 */
void EventQueue::QueueThreadProc(int tid)
{
	for (;;) {
		EventQueueWorkItem event;

		double ws = Utility::GetTime();
		double st;

		{
			boost::mutex::scoped_lock lock(m_Mutex);

			m_ThreadStates[tid] = ThreadIdle;

			while (m_Events.empty() && !m_Stopped && m_ThreadDeaths == 0)
				m_CV.wait(lock);

			if (m_ThreadDeaths > 0) {
				m_ThreadDeaths--;
				break;
			}

			if (m_Events.empty() && m_Stopped)
				break;

			event = m_Events.front();
			m_Events.pop_front();

			m_ThreadStates[tid] = ThreadBusy;
			st = Utility::GetTime();
			UpdateThreadUtilization(tid, st - ws, 0);
		}

#ifdef _DEBUG
#	ifdef RUSAGE_THREAD
		struct rusage usage_start, usage_end;

		(void) getrusage(RUSAGE_THREAD, &usage_start);
#	endif /* RUSAGE_THREAD */
#endif /* _DEBUG */

		try {
			event.Callback();
		} catch (const std::exception& ex) {
			std::ostringstream msgbuf;
			msgbuf << "Exception thrown in event handler: " << std::endl
			       << boost::diagnostic_information(ex);

			Log(LogCritical, "base", msgbuf.str());
		} catch (...) {
			Log(LogCritical, "base", "Exception of unknown type thrown in event handler.");
		}

		double et = Utility::GetTime();
		double latency = st - event.Timestamp;

		{
			boost::mutex::scoped_lock lock(m_Mutex);

			m_WaitTime += latency;
			m_ServiceTime += et - st;
			m_TaskCount++;

			if (latency > m_MaxLatency)
				m_MaxLatency = latency;

			UpdateThreadUtilization(tid, et - st, 1);
		}

#ifdef _DEBUG
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

	m_ThreadStates[tid] = ThreadDead;
}

/**
 * Appends an event to the event queue. Events will be processed in FIFO order.
 *
 * @param callback The callback function for the event.
 */
void EventQueue::Post(const EventQueueCallback& callback)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	if (m_Stopped)
		BOOST_THROW_EXCEPTION(std::runtime_error("EventQueue has been stopped."));

	EventQueueWorkItem event;
	event.Callback = callback;
	event.Timestamp = Utility::GetTime();

	m_Events.push_back(event);
	m_CV.notify_one();
}

void EventQueue::ManagerThreadProc(void)
{
	for (;;) {
		Utility::Sleep(5);

		double now = Utility::GetTime();

		int pending, alive;
		double avg_latency, max_latency;

		{
			boost::mutex::scoped_lock lock(m_Mutex);
			pending = m_Events.size();

			alive = 0;

			double util = 0;
			int hg = 0;

			for (int i = 0; i < sizeof(m_ThreadStates) / sizeof(m_ThreadStates[0]); i++) {
				if (m_ThreadStates[i] != ThreadDead) {
					alive++;
					util += m_ThreadUtilization[i] * 100;
					std::cout << (int)(m_ThreadUtilization[i] * 100) << "\t";
					hg++;
					if (hg % 25 == 0)
						std::cout << std::endl;
				}
			}

			util /= alive;

			std::cout << std::endl;

			if (m_TaskCount > 0)
				avg_latency = m_WaitTime / (m_TaskCount * 1.0);
			else
				avg_latency = 0;

			std::cout << "Wait time: " << m_WaitTime << "; Service time: " << m_ServiceTime << "; tasks: " << m_TaskCount << std::endl;
			std::cout << "Thread util: " << util << std::endl;

			if (util < 60 || util > 80) {
				int tthreads = ceil((util * alive) / 80.0) - alive;

				if (alive + tthreads < 2)
					tthreads = 2 - alive;

				std::cout << "Target threads: " << tthreads << "; Alive: " << alive << std::endl;

				for (int i = 0; i < -tthreads; i++)
					KillWorker();

				for (int i = 0; i < tthreads; i++)
					SpawnWorker();
			}

			m_WaitTime = 0;
			m_ServiceTime = 0;
			m_TaskCount = 0;

			max_latency = m_MaxLatency;
			m_MaxLatency = 0;
		}

		std::ostringstream msgbuf;
		msgbuf << "Pending tasks: " << pending << "; Average latency: " << (long)(avg_latency * 1000) << "ms"
		    << "; Max latency: " << (long)(max_latency * 1000) << "ms";
		Log(LogInformation, "base", msgbuf.str());
	}
}

/**
 * Note: Caller must hold m_Mutex
 */
void EventQueue::SpawnWorker(void)
{
	for (int i = 0; i < sizeof(m_ThreadStates) / sizeof(m_ThreadStates[0]); i++) {
		if (m_ThreadStates[i] == ThreadDead) {
			Log(LogDebug, "debug", "Spawning worker thread.");

			m_ThreadStates[i] = ThreadIdle;
			m_ThreadUtilization[i] = 0;
			boost::thread worker(boost::bind(&EventQueue::QueueThreadProc, this, i));
			worker.detach();

			break;
		}
	}
}

/**
 * Note: Caller must hold m_Mutex.
 */
void EventQueue::KillWorker(void)
{
	Log(LogDebug, "base", "Killing worker thread.");

	m_ThreadDeaths++;
}

/**
 * Note: Caller must hold m_Mutex.
 */
void EventQueue::UpdateThreadUtilization(int tid, double time, double utilization)
{
	const double avg_time = 5.0;

	if (time > avg_time)
		time = avg_time;

	m_ThreadUtilization[tid] = (m_ThreadUtilization[tid] * (avg_time - time) + utilization * time) / avg_time;
}
