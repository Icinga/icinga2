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
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

int WorkQueue::m_NextID = 1;

WorkQueue::WorkQueue(size_t maxItems)
	: m_ID(m_NextID++), m_MaxItems(maxItems), m_Stopped(false),
	  m_Processing(false), m_ExceptionCallback(WorkQueue::DefaultExceptionCallback)
{
	m_StatusTimer = new Timer();
	m_StatusTimer->SetInterval(10);
	m_StatusTimer->OnTimerExpired.connect(boost::bind(&WorkQueue::StatusTimerHandler, this));
	m_StatusTimer->Start();
}

WorkQueue::~WorkQueue(void)
{
	Join(true);
}

/**
 * Enqueues a work item. Work items are guaranteed to be executed in the order
 * they were enqueued in except when allowInterleaved is true in which case
 * the new work item might be run immediately if it's being enqueued from
 * within the WorkQueue thread.
 */
void WorkQueue::Enqueue(const WorkCallback& callback, bool allowInterleaved)
{
	bool wq_thread = (boost::this_thread::get_id() == GetThreadId());

	if (wq_thread && allowInterleaved) {
		callback();

		return;
	}

	WorkItem item;
	item.Callback = callback;
	item.AllowInterleaved = allowInterleaved;

	boost::mutex::scoped_lock lock(m_Mutex);

	if (m_Thread.get_id() == boost::thread::id())
		m_Thread = boost::thread(boost::bind(&WorkQueue::WorkerThreadProc, this));

	if (!wq_thread) {
		while (m_Items.size() >= m_MaxItems)
			m_CVFull.wait(lock);
	}

	m_Items.push_back(item);

	if (m_Items.size() == 1)
		m_CVEmpty.notify_all();
}

void WorkQueue::Join(bool stop)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	while (m_Processing || !m_Items.empty())
		m_CVStarved.wait(lock);

	if (stop) {
		m_Stopped = true;
		m_CVEmpty.notify_all();
		lock.unlock();

		if (m_Thread.joinable())
			m_Thread.join();
	}
}

boost::thread::id WorkQueue::GetThreadId(void) const
{
	return m_Thread.get_id();
}

void WorkQueue::SetExceptionCallback(const ExceptionCallback& callback)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	m_ExceptionCallback = callback;
}

size_t WorkQueue::GetLength(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	return m_Items.size();
}

void WorkQueue::DefaultExceptionCallback(boost::exception_ptr)
{
	throw;
}

void WorkQueue::StatusTimerHandler(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	Log(LogNotice, "WorkQueue")
	    << "#" << m_ID << " items: " << m_Items.size();
}

void WorkQueue::WorkerThreadProc(void)
{
	std::ostringstream idbuf;
	idbuf << "WQ #" << m_ID;
	Utility::SetThreadName(idbuf.str());

	boost::mutex::scoped_lock lock(m_Mutex);

	for (;;) {
		while (m_Items.empty() && !m_Stopped)
			m_CVEmpty.wait(lock);

		if (m_Stopped)
			break;

		std::deque<WorkItem> items;
		m_Items.swap(items);

		if (items.size() >= m_MaxItems)
			m_CVFull.notify_all();

		m_Processing = true;

		lock.unlock();

		BOOST_FOREACH(WorkItem& wi, items) {
			try {
				wi.Callback();
			}
			catch (const std::exception&) {
				lock.lock();

				ExceptionCallback callback = m_ExceptionCallback;

				lock.unlock();

				callback(boost::current_exception());
			}
		}

		lock.lock();

		m_Processing = false;

		m_CVStarved.notify_all();
	}
}

ParallelWorkQueue::ParallelWorkQueue(void)
	: m_QueueCount(Application::GetConcurrency()),
	  m_Queues(new WorkQueue[m_QueueCount]),
	  m_Index(0)
{ }

ParallelWorkQueue::~ParallelWorkQueue(void)
{
	delete[] m_Queues;
}

void ParallelWorkQueue::Enqueue(const boost::function<void(void)>& callback)
{
	m_Index++;
	m_Queues[m_Index % m_QueueCount].Enqueue(callback);
}

void ParallelWorkQueue::Join(void)
{
	for (unsigned int i = 0; i < m_QueueCount; i++)
		m_Queues[i].Join();
}
