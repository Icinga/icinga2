/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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
#include "base/debug.h"
#include "base/utility.h"
#include "base/scriptvariable.h"
#include <sstream>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

int ThreadPool::m_NextID = 1;

ThreadPool::ThreadPool(void)
	: m_ID(m_NextID++), m_WaitTime(0), m_ServiceTime(0),
	  m_TaskCount(0), m_Stopped(false)
{
	for (int i = 0; i < 2; i++)
		SpawnWorker();

	m_ManagerThread = boost::thread(boost::bind(&ThreadPool::ManagerThreadProc, this));
	m_StatsThread = boost::thread(boost::bind(&ThreadPool::StatsThreadProc, this));
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
	m_WorkCV.notify_all();
	m_MgmtCV.notify_all();
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

	int alive;

	do {
		alive = 0;
		for (size_t i = 0; i < sizeof(m_ThreadStats) / sizeof(m_ThreadStats[0]); i++) {
			if (m_ThreadStats[i].State != ThreadDead) {
				alive++;
				KillWorker();
			}
		}

		if (alive > 0) {
			lock.unlock();
			Utility::Sleep(0.5);
			lock.lock();
		}
	} while (alive > 0);

	m_ManagerThread.join();
	m_StatsThread.join();
}

/**
 * Waits for work items and processes them.
 */
void ThreadPool::QueueThreadProc(int tid)
{
	std::ostringstream idbuf;
	idbuf << "TP #" << m_ID << " W #" << tid;
	Utility::SetThreadName(idbuf.str());

	for (;;) {
		WorkItem wi;

		{
			boost::mutex::scoped_lock lock(m_Mutex);

			UpdateThreadUtilization(tid, ThreadIdle);

			while (m_WorkItems.empty() && !m_Stopped && !m_ThreadStats[tid].Zombie)
				m_WorkCV.wait(lock);

			if (m_ThreadStats[tid].Zombie)
				break;

			if (m_WorkItems.empty() && m_Stopped)
				break;

			wi = m_WorkItems.front();
			m_WorkItems.pop_front();

			UpdateThreadUtilization(tid, ThreadBusy);
		}

		double st = Utility::GetTime();;

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

	boost::mutex::scoped_lock lock(m_Mutex);
	UpdateThreadUtilization(tid, ThreadDead);
	m_ThreadStats[tid].Zombie = false;
}

/**
 * Appends a work item to the work queue. Work items will be processed in FIFO order.
 *
 * @param callback The callback function for the work item.
 * @returns true if the item was queued, false otherwise.
 */
bool ThreadPool::Post(const ThreadPool::WorkFunction& callback)
{
	WorkItem wi;
	wi.Callback = callback;
	wi.Timestamp = Utility::GetTime();

	{
		boost::mutex::scoped_lock lock(m_Mutex);

		if (m_Stopped)
			return false;

		m_WorkItems.push_back(wi);
		m_WorkCV.notify_one();
	}

	return true;
}

void ThreadPool::ManagerThreadProc(void)
{
	std::ostringstream idbuf;
	idbuf << "TP #" << m_ID << " Manager";
	Utility::SetThreadName(idbuf.str());

	for (;;) {
		size_t pending, alive;
		double avg_latency, max_latency;
		double utilization = 0;

		{
			boost::mutex::scoped_lock lock(m_Mutex);

			m_MgmtCV.timed_wait(lock, boost::posix_time::seconds(5));

			if (m_Stopped)
				break;

			pending = m_WorkItems.size();

			alive = 0;

			for (size_t i = 0; i < sizeof(m_ThreadStats) / sizeof(m_ThreadStats[0]); i++) {
				if (m_ThreadStats[i].State != ThreadDead && !m_ThreadStats[i].Zombie) {
					alive++;
					utilization += m_ThreadStats[i].Utilization * 100;
				}
			}

			utilization /= alive;

			if (m_TaskCount > 0)
				avg_latency = m_WaitTime / (m_TaskCount * 1.0);
			else
				avg_latency = 0;

			if (utilization < 60 || utilization > 80 || alive < 8) {
				double wthreads = ceil((utilization * alive) / 80.0);

				int tthreads = wthreads - alive;

				/* Don't ever kill the last 8 threads. */
				if (alive + tthreads < 8)
					tthreads = 8 - alive;

				/* Spawn more workers if there are outstanding work items. */
				if (tthreads > 0 && pending > 0)
					tthreads = 8;

				std::ostringstream msgbuf;
				msgbuf << "Thread pool; current: " << alive << "; adjustment: " << tthreads;
				Log(LogDebug, "base", msgbuf.str());

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
	for (size_t i = 0; i < sizeof(m_ThreadStats) / sizeof(m_ThreadStats[0]); i++) {
		if (m_ThreadStats[i].State == ThreadDead) {
			Log(LogDebug, "debug", "Spawning worker thread.");

			m_ThreadStats[i] = ThreadStats(ThreadIdle);
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
	for (size_t i = 0; i < sizeof(m_ThreadStats) / sizeof(m_ThreadStats[0]); i++) {
		if (m_ThreadStats[i].State == ThreadIdle && !m_ThreadStats[i].Zombie) {
			Log(LogDebug, "base", "Killing worker thread.");

			m_ThreadStats[i].Zombie = true;
			m_WorkCV.notify_all();

			break;
		}
	}
}

void ThreadPool::StatsThreadProc(void)
{
	std::ostringstream idbuf;
	idbuf << "TP #" << m_ID << " Stats";
	Utility::SetThreadName(idbuf.str());

	for (;;) {
		boost::mutex::scoped_lock lock(m_Mutex);

		m_MgmtCV.timed_wait(lock, boost::posix_time::milliseconds(250));

		if (m_Stopped)
			break;

		for (size_t i = 0; i < sizeof(m_ThreadStats) / sizeof(m_ThreadStats[0]); i++)
			UpdateThreadUtilization(i);
	}
}

/**
 * Note: Caller must hold m_Mutex.
 */
void ThreadPool::UpdateThreadUtilization(int tid, ThreadState state)
{
	double utilization;

	switch (m_ThreadStats[tid].State) {
		case ThreadDead:
			return;
		case ThreadIdle:
			utilization = 0;
			break;
		case ThreadBusy:
			utilization = 1;
			break;
		default:
			ASSERT(0);
	}

	double now = Utility::GetTime();
	double time = now - m_ThreadStats[tid].LastUpdate;

	const double avg_time = 5.0;

	if (time > avg_time)
		time = avg_time;

	m_ThreadStats[tid].Utilization = (m_ThreadStats[tid].Utilization * (avg_time - time) + utilization * time) / avg_time;
	m_ThreadStats[tid].LastUpdate = now;

	if (state != ThreadUnspecified)
		m_ThreadStats[tid].State = state;
}
