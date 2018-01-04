/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/threadpool.hpp"
#include "base/logger.hpp"
#include "base/debug.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/application.hpp"
#include <iostream>

using namespace icinga;

int ThreadPool::m_NextID = 1;

ThreadPool::ThreadPool(size_t max_threads)
	: m_ID(m_NextID++), m_MaxThreads(max_threads), m_Stopped(true)
{
	if (m_MaxThreads != UINT_MAX && m_MaxThreads < sizeof(m_Queues) / sizeof(m_Queues[0]))
		m_MaxThreads = sizeof(m_Queues) / sizeof(m_Queues[0]);

	Start();
}

ThreadPool::~ThreadPool()
{
	Stop();
}

void ThreadPool::Start()
{
	if (!m_Stopped)
		return;

	m_Stopped = false;

	for (size_t i = 0; i < sizeof(m_Queues) / sizeof(m_Queues[0]); i++)
		m_Queues[i].SpawnWorker(m_ThreadGroup);

	m_MgmtThread = std::thread(std::bind(&ThreadPool::ManagerThreadProc, this));
}

void ThreadPool::Stop()
{
	if (m_Stopped)
		return;

	{
		boost::mutex::scoped_lock lock(m_MgmtMutex);
		m_Stopped = true;
		m_MgmtCV.notify_all();
	}

	if (m_MgmtThread.joinable())
		m_MgmtThread.join();

	for (size_t i = 0; i < sizeof(m_Queues) / sizeof(m_Queues[0]); i++) {
		boost::mutex::scoped_lock lock(m_Queues[i].Mutex);
		m_Queues[i].Stopped = true;
		m_Queues[i].CV.notify_all();
	}

	m_ThreadGroup.join_all();
	m_ThreadGroup.~thread_group();
	new (&m_ThreadGroup) boost::thread_group();

	for (size_t i = 0; i < sizeof(m_Queues) / sizeof(m_Queues[0]); i++)
		m_Queues[i].Stopped = false;

	m_Stopped = true;
}

/**
 * Waits for work items and processes them.
 */
void ThreadPool::WorkerThread::ThreadProc(Queue& queue)
{
	std::ostringstream idbuf;
	idbuf << "Q #" << &queue << " W #" << this;
	Utility::SetThreadName(idbuf.str());

	for (;;) {
		WorkItem wi;

		{
			boost::mutex::scoped_lock lock(queue.Mutex);

			UpdateUtilization(ThreadIdle);

			while (queue.Items.empty() && !queue.Stopped && !Zombie) {
				if (queue.Items.empty())
					queue.CVStarved.notify_all();

				queue.CV.wait(lock);
			}

			if (Zombie)
				break;

			if (queue.Items.empty() && queue.Stopped)
				break;

			wi = queue.Items.front();
			queue.Items.pop_front();

			UpdateUtilization(ThreadBusy);
		}

		double st = Utility::GetTime();;

#ifdef I2_DEBUG
#	ifdef RUSAGE_THREAD
		struct rusage usage_start, usage_end;

		(void) getrusage(RUSAGE_THREAD, &usage_start);
#	endif /* RUSAGE_THREAD */
#endif /* I2_DEBUG */

		try {
			if (wi.Callback)
				wi.Callback();
		} catch (const std::exception& ex) {
			Log(LogCritical, "ThreadPool")
				<< "Exception thrown in event handler:\n"
				<< DiagnosticInformation(ex);
		} catch (...) {
			Log(LogCritical, "ThreadPool", "Exception of unknown type thrown in event handler.");
		}

		double et = Utility::GetTime();
		double latency = st - wi.Timestamp;

		{
			boost::mutex::scoped_lock lock(queue.Mutex);

			queue.WaitTime += latency;
			queue.ServiceTime += et - st;
			queue.TaskCount++;
		}

#ifdef I2_DEBUG
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
			Log(LogWarning, "ThreadPool")
#	ifdef RUSAGE_THREAD
				<< "Event call took user:" << duser << "s, system:" << dsys << "s, wait:" << dwait << "s, minor_faults:" << dminfaults << ", major_faults:" << dmajfaults << ", voluntary_csw:" << dvctx << ", involuntary_csw:" << divctx;
#	else
				<< "Event call took " << (et - st) << "s";
#	endif /* RUSAGE_THREAD */
		}
#endif /* I2_DEBUG */
	}

	boost::mutex::scoped_lock lock(queue.Mutex);
	UpdateUtilization(ThreadDead);
	Zombie = false;
}

/**
 * Appends a work item to the work queue. Work items will be processed in FIFO order.
 *
 * @param callback The callback function for the work item.
 * @param policy The scheduling policy
 * @returns true if the item was queued, false otherwise.
 */
bool ThreadPool::Post(const ThreadPool::WorkFunction& callback, SchedulerPolicy policy)
{
	WorkItem wi;
	wi.Callback = callback;
	wi.Timestamp = Utility::GetTime();

	Queue& queue = m_Queues[Utility::Random() % (sizeof(m_Queues) / sizeof(m_Queues[0]))];

	{
		boost::mutex::scoped_lock lock(queue.Mutex);

		if (queue.Stopped)
			return false;

		if (policy == LowLatencyScheduler)
			queue.SpawnWorker(m_ThreadGroup);

		queue.Items.emplace_back(std::move(wi));
		queue.CV.notify_one();
	}

	return true;
}

void ThreadPool::ManagerThreadProc()
{
	std::ostringstream idbuf;
	idbuf << "TP #" << m_ID << " Manager";
	Utility::SetThreadName(idbuf.str());

	double lastStats = 0;

	for (;;) {
		size_t total_pending = 0, total_alive = 0;
		double total_avg_latency = 0;
		double total_utilization = 0;

		{
			boost::mutex::scoped_lock lock(m_MgmtMutex);

			if (!m_Stopped)
				m_MgmtCV.timed_wait(lock, boost::posix_time::milliseconds(500));

			if (m_Stopped)
				break;
		}

		for (size_t i = 0; i < sizeof(m_Queues) / sizeof(m_Queues[0]); i++) {
			size_t pending, alive = 0;
			double avg_latency;
			double utilization = 0;

			Queue& queue = m_Queues[i];

			boost::mutex::scoped_lock lock(queue.Mutex);

			for (size_t i = 0; i < sizeof(queue.Threads) / sizeof(queue.Threads[0]); i++)
				queue.Threads[i].UpdateUtilization();

			pending = queue.Items.size();

			for (size_t i = 0; i < sizeof(queue.Threads) / sizeof(queue.Threads[0]); i++) {
				if (queue.Threads[i].State != ThreadDead && !queue.Threads[i].Zombie) {
					alive++;
					utilization += queue.Threads[i].Utilization * 100;
				}
			}

			utilization /= alive;

			if (queue.TaskCount > 0)
				avg_latency = queue.WaitTime / (queue.TaskCount * 1.0);
			else
				avg_latency = 0;

			if (utilization < 60 || utilization > 80 || alive < 8) {
				double wthreads = std::ceil((utilization * alive) / 80.0);

				int tthreads = wthreads - alive;

				/* Make sure there is at least one thread per queue */
				if (alive + tthreads < 1)
					tthreads = 1 - alive;

				/* Don't kill more than 2 threads at once. */
				if (tthreads < -2)
					tthreads = -2;

				/* Spawn more workers if there are outstanding work items. */
				if (tthreads > 0 && pending > 0)
					tthreads = 2;

				if (m_MaxThreads != UINT_MAX && (alive + tthreads) * (sizeof(m_Queues) / sizeof(m_Queues[0])) > m_MaxThreads)
					tthreads = m_MaxThreads / (sizeof(m_Queues) / sizeof(m_Queues[0])) - alive;

				if (tthreads != 0) {
					Log(LogNotice, "ThreadPool")
						<< "Thread pool; current: " << alive << "; adjustment: " << tthreads;
				}

				for (int i = 0; i < -tthreads; i++)
					queue.KillWorker(m_ThreadGroup);

				for (int i = 0; i < tthreads; i++)
					queue.SpawnWorker(m_ThreadGroup);
			}

			queue.WaitTime = 0;
			queue.ServiceTime = 0;
			queue.TaskCount = 0;

			total_pending += pending;
			total_alive += alive;
			total_avg_latency += avg_latency;
			total_utilization += utilization;
		}

		double now = Utility::GetTime();

		if (lastStats < now - 15) {
			lastStats = now;

			Log(LogNotice, "ThreadPool")
				<< "Pool #" << m_ID << ": Pending tasks: " << total_pending << "; Average latency: "
				<< (long)(total_avg_latency * 1000 / (sizeof(m_Queues) / sizeof(m_Queues[0]))) << "ms"
				<< "; Threads: " << total_alive
				<< "; Pool utilization: " << (total_utilization / (sizeof(m_Queues) / sizeof(m_Queues[0]))) << "%";
		}
	}
}

/**
 * Note: Caller must hold m_Mutex
 */
void ThreadPool::Queue::SpawnWorker(boost::thread_group& group)
{
	for (size_t i = 0; i < sizeof(Threads) / sizeof(Threads[0]); i++) {
		if (Threads[i].State == ThreadDead) {
			Log(LogDebug, "ThreadPool", "Spawning worker thread.");

			Threads[i] = WorkerThread(ThreadIdle);
			Threads[i].Thread = group.create_thread(std::bind(&ThreadPool::WorkerThread::ThreadProc, std::ref(Threads[i]), std::ref(*this)));

			break;
		}
	}
}

/**
 * Note: Caller must hold Mutex.
 */
void ThreadPool::Queue::KillWorker(boost::thread_group& group)
{
	for (size_t i = 0; i < sizeof(Threads) / sizeof(Threads[0]); i++) {
		if (Threads[i].State == ThreadIdle && !Threads[i].Zombie) {
			Log(LogDebug, "ThreadPool", "Killing worker thread.");

			group.remove_thread(Threads[i].Thread);
			Threads[i].Thread->detach();
			delete Threads[i].Thread;

			Threads[i].Zombie = true;
			CV.notify_all();

			break;
		}
	}
}

/**
 * Note: Caller must hold queue Mutex.
 */
void ThreadPool::WorkerThread::UpdateUtilization(ThreadState state)
{
	double utilization;

	switch (State) {
		case ThreadDead:
			return;
		case ThreadIdle:
			utilization = 0;
			break;
		case ThreadBusy:
			utilization = 1;
			break;
		default:
			VERIFY(0);
	}

	double now = Utility::GetTime();
	double time = now - LastUpdate;

	const double avg_time = 5.0;

	if (time > avg_time)
		time = avg_time;

	Utilization = (Utilization * (avg_time - time) + utilization * time) / avg_time;
	LastUpdate = now;

	if (state != ThreadUnspecified)
		State = state;
}
