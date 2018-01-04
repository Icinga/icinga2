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

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "base/i2-base.hpp"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <deque>
#include <thread>

namespace icinga
{

#define QUEUECOUNT 4U

enum SchedulerPolicy
{
	DefaultScheduler,
	LowLatencyScheduler
};

/**
 * A thread pool.
 *
 * @ingroup base
 */
class ThreadPool
{
public:
	typedef std::function<void ()> WorkFunction;

	ThreadPool(size_t max_threads = UINT_MAX);
	~ThreadPool();

	void Start();
	void Stop();

	bool Post(const WorkFunction& callback, SchedulerPolicy policy = DefaultScheduler);

private:
	enum ThreadState
	{
		ThreadUnspecified,
		ThreadDead,
		ThreadIdle,
		ThreadBusy
	};

	struct WorkItem
	{
		WorkFunction Callback;
		double Timestamp;
	};

	struct Queue;

	struct WorkerThread
	{
		ThreadState State{ThreadDead};
		bool Zombie{false};
		double Utilization{0};
		double LastUpdate{0};
		boost::thread *Thread{nullptr};

		WorkerThread(ThreadState state = ThreadDead)
			: State(state)
		{ }

		void UpdateUtilization(ThreadState state = ThreadUnspecified);

		void ThreadProc(Queue& queue);
	};

	struct Queue
	{
		boost::mutex Mutex;
		boost::condition_variable CV;
		boost::condition_variable CVStarved;

		std::deque<WorkItem> Items;

		double WaitTime{0};
		double ServiceTime{0};
		int TaskCount{0};

		bool Stopped{false};

		WorkerThread Threads[16];

		void SpawnWorker(boost::thread_group& group);
		void KillWorker(boost::thread_group& group);
	};

	int m_ID;
	static int m_NextID;

	size_t m_MaxThreads;

	boost::thread_group m_ThreadGroup;

	std::thread m_MgmtThread;
	boost::mutex m_MgmtMutex;
	boost::condition_variable m_MgmtCV;
	bool m_Stopped{true};

	Queue m_Queues[QUEUECOUNT];

	void ManagerThreadProc();
};

}

#endif /* EVENTQUEUE_H */
