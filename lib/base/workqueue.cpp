/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "base/workqueue.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/application.hpp"
#include "base/exception.hpp"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/tss.hpp>

using namespace icinga;

int WorkQueue::m_NextID = 1;
boost::thread_specific_ptr<WorkQueue *> l_ThreadWorkQueue;

WorkQueue::WorkQueue(size_t maxItems, int threadCount)
	: m_ID(m_NextID++), m_ThreadCount(threadCount), m_Spawned(false), m_MaxItems(maxItems), m_Stopped(false),
	  m_Processing(0)
{
	m_StatusTimer = new Timer();
	m_StatusTimer->SetInterval(10);
	m_StatusTimer->OnTimerExpired.connect(boost::bind(&WorkQueue::StatusTimerHandler, this));
	m_StatusTimer->Start();
}

WorkQueue::~WorkQueue(void)
{
	m_StatusTimer->Stop(true);

	Join(true);
}

/**
 * Enqueues a task. Tasks are guaranteed to be executed in the order
 * they were enqueued in except if there is more than one worker thread or when
 * allowInterleaved is true in which case the new task might be run
 * immediately if it's being enqueued from within the WorkQueue thread.
 */
void WorkQueue::Enqueue(const Task& task, bool allowInterleaved)
{
	bool wq_thread = IsWorkerThread();

	if (wq_thread && allowInterleaved) {
		task();

		return;
	}

	boost::mutex::scoped_lock lock(m_Mutex);

	if (!m_Spawned) {
		for (int i = 0; i < m_ThreadCount; i++) {
			m_Threads.create_thread(boost::bind(&WorkQueue::WorkerThreadProc, this));
		}

		m_Spawned = true;
	}

	if (!wq_thread) {
		while (m_Tasks.size() >= m_MaxItems)
			m_CVFull.wait(lock);
	}

	m_Tasks.push_back(task);

	if (m_Tasks.size() == 1)
		m_CVEmpty.notify_all();
}

/**
 * Waits until all currently enqueued tasks have completed. This only works reliably
 * when no other thread is enqueuing new tasks when this method is called.
 *
 * @param stop Whether to stop the worker threads
 */
void WorkQueue::Join(bool stop)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	while (m_Processing || !m_Tasks.empty())
		m_CVStarved.wait(lock);

	if (stop) {
		m_Stopped = true;
		m_CVEmpty.notify_all();
		lock.unlock();

		m_Threads.join_all();
		m_Spawned = false;
	}
}

/**
 * Checks whether the calling thread is one of the worker threads
 * for this work queue.
 *
 * @returns true if called from one of the worker threads, false otherwise
 */
bool WorkQueue::IsWorkerThread(void) const
{
	WorkQueue **pwq = l_ThreadWorkQueue.get();

	if (!pwq)
		return false;

	return *pwq == this;
}

void WorkQueue::SetExceptionCallback(const ExceptionCallback& callback)
{
	m_ExceptionCallback = callback;
}

/**
 * Checks whether any exceptions have occurred while executing tasks for this
 * work queue. When a custom exception callback is set this method will always
 * return false.
 */
bool WorkQueue::HasExceptions(void) const
{
	boost::mutex::scoped_lock lock(m_Mutex);
 
	return !m_Exceptions.empty();
}

/**
 * Returns all exceptions which have occurred for tasks in this work queue. When a
 * custom exception callback is set this method will always return an empty list.
 */
std::vector<boost::exception_ptr> WorkQueue::GetExceptions(void) const
{
	boost::mutex::scoped_lock lock(m_Mutex);
 
	return m_Exceptions;
}

void WorkQueue::ReportExceptions(const String& facility) const
{
	std::vector<boost::exception_ptr> exceptions = GetExceptions();

	BOOST_FOREACH(const boost::exception_ptr& eptr, exceptions) {
		Log(LogCritical, facility)
		    << DiagnosticInformation(eptr);
	}

	Log(LogCritical, facility)
	    << exceptions.size() << " error" << (exceptions.size() != 1 ? "s" : "");
}

size_t WorkQueue::GetLength(void) const
{
	boost::mutex::scoped_lock lock(m_Mutex);

	return m_Tasks.size();
}

void WorkQueue::StatusTimerHandler(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	Log(LogNotice, "WorkQueue")
	    << "#" << m_ID << " tasks: " << m_Tasks.size();
}

void WorkQueue::WorkerThreadProc(void)
{
	std::ostringstream idbuf;
	idbuf << "WQ #" << m_ID;
	Utility::SetThreadName(idbuf.str());

	l_ThreadWorkQueue.reset(new WorkQueue *(this));

	boost::mutex::scoped_lock lock(m_Mutex);

	for (;;) {
		while (m_Tasks.empty() && !m_Stopped)
			m_CVEmpty.wait(lock);

		if (m_Stopped)
			break;

		if (m_Tasks.size() >= m_MaxItems)
			m_CVFull.notify_all();

		Task task = m_Tasks.front();
		m_Tasks.pop_front();

		m_Processing++;

		lock.unlock();

		try {
			task();
		} catch (const std::exception&) {
			lock.lock();

			if (!m_ExceptionCallback)
				m_Exceptions.push_back(boost::current_exception());

			lock.unlock();

			if (m_ExceptionCallback)
				m_ExceptionCallback(boost::current_exception());
		}

		/* clear the task so whatever other resources it holds are released
		   _before_ we re-acquire the mutex */
		task = Task();

		lock.lock();

		m_Processing--;

		if (m_Tasks.empty())
			m_CVStarved.notify_all();
	}
}

