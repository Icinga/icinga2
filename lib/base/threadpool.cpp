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

#include "base/threadpool.h"
#include "base/logger_fwd.h"
#include "base/convert.h"
#include "base/utility.h"
#include <sstream>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

ThreadPool::ThreadPool(void)
	: m_Stopped(false), m_ThreadDeaths(0), m_WaitTime(0), m_ServiceTime(0), m_TaskCount(0)
{
	for (int i = 0; i < sizeof(m_ThreadStates) / sizeof(m_ThreadStates[0]); i++)
		m_ThreadStates[i] = ThreadDead;

	for (int i = 0; i < 2; i++)
		SpawnWorker();

	boost::thread managerThread(boost::bind(&ThreadPool::ManagerThreadProc, this));
	managerThread.detach();
}

ThreadPool::~ThreadPool(void)
{
	Stop();
	Join();
}

void ThreadPool::Stop(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	m_Stopped = true;
	m_CV.notify_all();
}

/**
 * Waits for all worker threads to finish.
 */
void ThreadPool::Join(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	while (!m_Stopped || !m_WorkItems.empty()) {
		lock.unlock();
		Utility::Sleep(0.5);
		lock.lock();
	}
}

/**
 * Waits for work items and processes them.
 */
void ThreadPool::QueueThreadProc(int tid)
{
	std::ostringstream idbuf;
	idbuf << "TP " << this << " Worker #" << tid;
	Utility::SetThreadName(idbuf.str());

	for (;;) {
		WorkItem wi;

		double ws = Utility::GetTime();
		double st;

		{
			boost::mutex::scoped_lock lock(m_Mutex);

			m_ThreadStates[tid] = ThreadIdle;

			while (m_WorkItems.empty() && !m_Stopped && m_ThreadDeaths == 0)
				m_CV.wait(lock);

			if (m_ThreadDeaths > 0) {
				m_ThreadDeaths--;
				break;
			}

			if (m_WorkItems.empty() && m_Stopped)
				break;

			wi = m_WorkItems.front();
			m_WorkItems.pop_front();

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
			wi.Callback();
		} catch (const std::exception& ex) {
			std::ostringstream msgbuf;
			msgbuf << "Exception thrown in event handler: " << std::endl
			       << boost::diagnostic_information(ex);

			Log(LogCritical, "base", msgbuf.str());
		} catch (...) {
			Log(LogCritical, "base", "Exception of unknown type thrown in event handler.");
		}

		double et = Utility::GetTime();
		double latency = st - wi.Timestamp;

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
 * Appends a work item to the work queue. Work items will be processed in FIFO order.
 *
 * @param callback The callback function for the work item.
 */
void ThreadPool::Post(const ThreadPool::WorkFunction& callback)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	if (m_Stopped)
		BOOST_THROW_EXCEPTION(std::runtime_error("ThreadPool has been stopped."));

	WorkItem wi;
	wi.Callback = callback;
	wi.Timestamp = Utility::GetTime();

	m_WorkItems.push_back(wi);
	m_CV.notify_one();
}

void ThreadPool::ManagerThreadProc(void)
{
	std::ostringstream idbuf;
	idbuf << "TP " << this << " Manager";
	Utility::SetThreadName(idbuf.str());

	for (;;) {
		Utility::Sleep(5);

		double now = Utility::GetTime();

		int pending, alive;
		double avg_latency, max_latency;
		double utilization = 0;

		{
			boost::mutex::scoped_lock lock(m_Mutex);
			pending = m_WorkItems.size();

			alive = 0;

			for (int i = 0; i < sizeof(m_ThreadStates) / sizeof(m_ThreadStates[0]); i++) {
				if (m_ThreadStates[i] != ThreadDead) {
					alive++;
					utilization += m_ThreadUtilization[i] * 100;
				}
			}

			utilization /= alive;

			if (m_TaskCount > 0)
				avg_latency = m_WaitTime / (m_TaskCount * 1.0);
			else
				avg_latency = 0;

			if (utilization < 60 || utilization > 80) {
				int tthreads = ceil((utilization * alive) / 80.0) - alive;

				/* Don't ever kill the last 2 threads. */
				if (alive + tthreads < 2)
					tthreads = 2 - alive;

				/* Spawn more workers if there are outstanding work items. */
				if (tthreads > 0 && pending > 0)
					tthreads = 8;

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
		msgbuf << "Pending tasks: " << pending << "; Average latency: "
		    << (long)(avg_latency * 1000) << "ms"
		    << "; Max latency: " << (long)(max_latency * 1000) << "ms"
		    << "; Threads: " << alive
		    << "; Pool utilization: " << utilization << "%";
		Log(LogInformation, "base", msgbuf.str());
	}
}

/**
 * Note: Caller must hold m_Mutex
 */
void ThreadPool::SpawnWorker(void)
{
	for (int i = 0; i < sizeof(m_ThreadStates) / sizeof(m_ThreadStates[0]); i++) {
		if (m_ThreadStates[i] == ThreadDead) {
			Log(LogDebug, "debug", "Spawning worker thread.");

			m_ThreadStates[i] = ThreadIdle;
			m_ThreadUtilization[i] = 0;
			boost::thread worker(boost::bind(&ThreadPool::QueueThreadProc, this, i));
			worker.detach();

			break;
		}
	}
}

/**
 * Note: Caller must hold m_Mutex.
 */
void ThreadPool::KillWorker(void)
{
	Log(LogDebug, "base", "Killing worker thread.");

	m_ThreadDeaths++;
}

/**
 * Note: Caller must hold m_Mutex.
 */
void ThreadPool::UpdateThreadUtilization(int tid, double time, double utilization)
{
	const double avg_time = 5.0;

	if (time > avg_time)
		time = avg_time;

	m_ThreadUtilization[tid] = (m_ThreadUtilization[tid] * (avg_time - time) + utilization * time) / avg_time;
}
