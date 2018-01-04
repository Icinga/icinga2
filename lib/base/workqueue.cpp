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

#include "base/workqueue.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/application.hpp"
#include "base/exception.hpp"
#include <boost/thread/tss.hpp>

using namespace icinga;

std::atomic<int> WorkQueue::m_NextID(1);
boost::thread_specific_ptr<WorkQueue *> l_ThreadWorkQueue;

WorkQueue::WorkQueue(size_t maxItems, int threadCount)
	: m_ID(m_NextID++), m_ThreadCount(threadCount), m_MaxItems(maxItems),
	m_TaskStats(15 * 60)
{
	/* Initialize logger. */
	m_StatusTimerTimeout = Utility::GetTime();

	m_StatusTimer = new Timer();
	m_StatusTimer->SetInterval(10);
	m_StatusTimer->OnTimerExpired.connect(std::bind(&WorkQueue::StatusTimerHandler, this));
	m_StatusTimer->Start();
}

WorkQueue::~WorkQueue()
{
	m_StatusTimer->Stop(true);

	Join(true);
}

void WorkQueue::SetName(const String& name)
{
	m_Name = name;
}

String WorkQueue::GetName() const
{
	return m_Name;
}

/**
 * Enqueues a task. Tasks are guaranteed to be executed in the order
 * they were enqueued in except if there is more than one worker thread or when
 * allowInterleaved is true in which case the new task might be run
 * immediately if it's being enqueued from within the WorkQueue thread.
 */
void WorkQueue::Enqueue(std::function<void ()>&& function, WorkQueuePriority priority,
	bool allowInterleaved)
{
	bool wq_thread = IsWorkerThread();

	if (wq_thread && allowInterleaved) {
		function();

		return;
	}

	boost::mutex::scoped_lock lock(m_Mutex);

	if (!m_Spawned) {
		Log(LogNotice, "WorkQueue")
			<< "Spawning WorkQueue threads for '" << m_Name << "'";

		for (int i = 0; i < m_ThreadCount; i++) {
			m_Threads.create_thread(std::bind(&WorkQueue::WorkerThreadProc, this));
		}

		m_Spawned = true;
	}

	if (!wq_thread) {
		while (m_Tasks.size() >= m_MaxItems && m_MaxItems != 0)
			m_CVFull.wait(lock);
	}

	m_Tasks.emplace(std::move(function), priority, ++m_NextTaskID);

	m_CVEmpty.notify_one();
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

		Log(LogNotice, "WorkQueue")
			<< "Stopped WorkQueue threads for '" << m_Name << "'";
	}
}

/**
 * Checks whether the calling thread is one of the worker threads
 * for this work queue.
 *
 * @returns true if called from one of the worker threads, false otherwise
 */
bool WorkQueue::IsWorkerThread() const
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
bool WorkQueue::HasExceptions() const
{
	boost::mutex::scoped_lock lock(m_Mutex);

	return !m_Exceptions.empty();
}

/**
 * Returns all exceptions which have occurred for tasks in this work queue. When a
 * custom exception callback is set this method will always return an empty list.
 */
std::vector<boost::exception_ptr> WorkQueue::GetExceptions() const
{
	boost::mutex::scoped_lock lock(m_Mutex);

	return m_Exceptions;
}

void WorkQueue::ReportExceptions(const String& facility) const
{
	std::vector<boost::exception_ptr> exceptions = GetExceptions();

	for (const auto& eptr : exceptions) {
		Log(LogCritical, facility)
			<< DiagnosticInformation(eptr);
	}

	Log(LogCritical, facility)
		<< exceptions.size() << " error" << (exceptions.size() != 1 ? "s" : "");
}

size_t WorkQueue::GetLength() const
{
	boost::mutex::scoped_lock lock(m_Mutex);

	return m_Tasks.size();
}

void WorkQueue::StatusTimerHandler()
{
	boost::mutex::scoped_lock lock(m_Mutex);

	ASSERT(!m_Name.IsEmpty());

	size_t pending = m_Tasks.size();

	double now = Utility::GetTime();
	double gradient = (pending - m_PendingTasks) / (now - m_PendingTasksTimestamp);
	double timeToZero = pending / gradient;

	String timeInfo;

	if (pending > GetTaskCount(5)) {
		timeInfo = " empty in ";
		if (timeToZero < 0)
			timeInfo += "infinite time, your task handler isn't able to keep up";
		else
			timeInfo += Utility::FormatDuration(timeToZero);
	}

	m_PendingTasks = pending;
	m_PendingTasksTimestamp = now;

	/* Log if there are pending items, or 5 minute timeout is reached. */
	if (pending > 0 || m_StatusTimerTimeout < now) {
		Log(LogInformation, "WorkQueue")
			<< "#" << m_ID << " (" << m_Name << ") "
			<< "items: " << pending << ", "
			<< "rate: " << std::setw(2) << GetTaskCount(60) / 60.0 << "/s "
			<< "(" << GetTaskCount(60) << "/min " << GetTaskCount(60 * 5) << "/5min " << GetTaskCount(60 * 15) << "/15min);"
			<< timeInfo;
	}

	/* Reschedule next log entry in 5 minutes. */
	if (m_StatusTimerTimeout < now) {
		m_StatusTimerTimeout = now + 60 * 5;
	}
}

void WorkQueue::WorkerThreadProc()
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

		if (m_Tasks.size() >= m_MaxItems && m_MaxItems != 0)
			m_CVFull.notify_all();

		Task task = m_Tasks.top();
		m_Tasks.pop();

		m_Processing++;

		lock.unlock();

		try {
			task.Function();
		} catch (const std::exception&) {
			lock.lock();

			if (!m_ExceptionCallback)
				m_Exceptions.push_back(boost::current_exception());

			lock.unlock();

			if (m_ExceptionCallback)
				m_ExceptionCallback(boost::current_exception());
		}

		/* clear the task so whatever other resources it holds are released _before_ we re-acquire the mutex */
		task = Task();

		IncreaseTaskCount();

		lock.lock();

		m_Processing--;

		if (m_Tasks.empty())
			m_CVStarved.notify_all();
	}
}

void WorkQueue::IncreaseTaskCount()
{
	double now = Utility::GetTime();

	boost::mutex::scoped_lock lock(m_StatsMutex);
	m_TaskStats.InsertValue(now, 1);
}

size_t WorkQueue::GetTaskCount(RingBuffer::SizeType span)
{
	boost::mutex::scoped_lock lock(m_StatsMutex);
	return m_TaskStats.UpdateAndGetValues(Utility::GetTime(), span);
}
