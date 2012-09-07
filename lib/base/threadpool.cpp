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

ThreadPool::ThreadPool(long numThreads)
	: m_Alive(true)
{
	for (long i = 0; i < numThreads; i++)
		m_Threads.create_thread(boost::bind(&ThreadPool::WorkerThreadProc, this));
}

ThreadPool::~ThreadPool(void)
{
	{
		boost::mutex::scoped_lock lock(m_Lock);

		m_Tasks.clear();

		/* notify worker threads to exit */
		m_Alive = false;
		m_CV.notify_all();
	}

	m_Threads.join_all();
}

void ThreadPool::EnqueueTasks(list<ThreadPoolTask::Ptr>& tasks)
{
	{
		boost::mutex::scoped_lock lock(m_Lock);
		m_Tasks.splice(m_Tasks.end(), tasks, tasks.begin(), tasks.end());
	}

	m_CV.notify_all();
}

void ThreadPool::EnqueueTask(const ThreadPoolTask::Ptr& task)
{
	{
		boost::mutex::scoped_lock lock(m_Lock);
		m_Tasks.push_back(task);
	}

	m_CV.notify_one();
}


ThreadPoolTask::Ptr ThreadPool::DequeueTask(void)
{
	boost::mutex::scoped_lock lock(m_Lock);

	while (m_Tasks.empty()) {
		if (!m_Alive)
			return ThreadPoolTask::Ptr();

		m_CV.wait(lock);
	}

	ThreadPoolTask::Ptr task = m_Tasks.front();
	m_Tasks.pop_front();

	return task;
}

void ThreadPool::WaitForTasks(void)
{
	boost::mutex::scoped_lock lock(m_Lock);

	/* wait for all pending tasks */
	while (!m_Tasks.empty())
		m_CV.wait(lock);
}

void ThreadPool::WorkerThreadProc(void)
{
	while (true) {
		ThreadPoolTask::Ptr task = DequeueTask();

		if (!task)
			break;

		task->Execute();
	}
}

ThreadPool::Ptr ThreadPool::GetDefaultPool(void)
{
	static ThreadPool::Ptr threadPool;

	if (!threadPool)
		threadPool = boost::make_shared<ThreadPool>();

	return threadPool;
}

