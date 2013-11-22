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
#include <boost/bind.hpp>

using namespace icinga;

int WorkQueue::m_NextID = 1;

WorkQueue::WorkQueue(size_t maxItems)
	: m_ID(m_NextID++), m_MaxItems(maxItems), m_Joined(false),
	  m_Stopped(false), m_ExceptionCallback(WorkQueue::DefaultExceptionCallback)
{
	m_Thread = boost::thread(boost::bind(&WorkQueue::WorkerThreadProc, this));
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
void WorkQueue::Enqueue(const WorkCallback& item)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	ASSERT(!m_Stopped);

	bool wq_thread = (boost::this_thread::get_id() == GetThreadId());

	if (!wq_thread) {
		while (m_Items.size() >= m_MaxItems)
			m_CV.wait(lock);
	}

	m_Items.push_back(item);

	if (wq_thread)
		ProcessItems(lock);
	else
		m_CV.notify_all();
}

void WorkQueue::Join(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	m_Joined = true;
	m_CV.notify_all();

	while (!m_Stopped)
		m_CV.wait(lock);
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

void WorkQueue::ProcessItems(boost::mutex::scoped_lock& lock)
{
	while (!m_Items.empty()) {
		try {
			WorkCallback wi = m_Items.front();
			m_Items.pop_front();
			m_CV.notify_all();

			lock.unlock();
			wi();
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
			m_CV.wait(lock);

		if (m_Joined)
			break;

		ProcessItems(lock);
	}

	m_Stopped = true;
	m_CV.notify_all();
}
