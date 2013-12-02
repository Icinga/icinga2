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

#include "base/workqueue.h"
#include "base/utility.h"
#include "base/debug.h"
#include "base/logger_fwd.h"
#include "base/convert.h"
#include <boost/bind.hpp>

using namespace icinga;

int WorkQueue::m_NextID = 1;

WorkQueue::WorkQueue(size_t maxItems)
	: m_ID(m_NextID++), m_MaxItems(maxItems), m_Joined(false),
	  m_Stopped(false), m_ExceptionCallback(WorkQueue::DefaultExceptionCallback)
{
	m_Thread = boost::thread(boost::bind(&WorkQueue::WorkerThreadProc, this));

	m_StatusTimer = make_shared<Timer>();
	m_StatusTimer->SetInterval(10);
	m_StatusTimer->OnTimerExpired.connect(boost::bind(&WorkQueue::StatusTimerHandler, this));
	m_StatusTimer->Start();
}

WorkQueue::~WorkQueue(void)
{
	Join();

	ASSERT(m_Stopped);
}

/**
 * Enqueues a work item. Work items are guaranteed to be executed in the order
 * they were enqueued in.
 */
void WorkQueue::Enqueue(const WorkCallback& callback, bool allowInterleaved)
{
	WorkItem item;
	item.Callback = callback;
	item.AllowInterleaved = allowInterleaved;

	bool wq_thread = (boost::this_thread::get_id() == GetThreadId());

	boost::mutex::scoped_lock lock(m_Mutex);

	ASSERT(!m_Stopped);

	if (!wq_thread) {
		while (m_Items.size() >= m_MaxItems)
			m_CVFull.wait(lock);
	}

	m_Items.push_back(item);

	if (wq_thread)
		ProcessItems(lock, true);
	else
		m_CVEmpty.notify_all();
}

void WorkQueue::Join(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	m_Joined = true;
	m_CVEmpty.notify_all();

	while (!m_Stopped)
		m_CVFull.wait(lock);
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

void WorkQueue::DefaultExceptionCallback(boost::exception_ptr exp)
{
	throw;
}

void WorkQueue::StatusTimerHandler(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	Log(LogInformation, "base", "WQ #" + Convert::ToString(m_ID) + " items: " + Convert::ToString(m_Items.size()));
}

void WorkQueue::ProcessItems(boost::mutex::scoped_lock& lock, bool interleaved)
{
	while (!m_Items.empty()) {
		WorkItem wi = m_Items.front();

		if (interleaved && !wi.AllowInterleaved)
			return;

		m_Items.pop_front();
		m_CVFull.notify_one();

		lock.unlock();

		try {
			wi.Callback();
		} catch (const std::exception& ex) {
			lock.lock();

			ExceptionCallback callback = m_ExceptionCallback;

			lock.unlock();

			callback(boost::current_exception());
		}

		lock.lock();
	}
}

void WorkQueue::WorkerThreadProc(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	std::ostringstream idbuf;
	idbuf << "WQ #" << m_ID;
	Utility::SetThreadName(idbuf.str());

	for (;;) {
		while (m_Items.empty() && !m_Joined)
			m_CVEmpty.wait(lock);

		if (m_Joined)
			break;

		ProcessItems(lock, false);
	}

	m_Stopped = true;
	m_CVFull.notify_all();
}
