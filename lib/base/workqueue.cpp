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
#include <boost/bind.hpp>

using namespace icinga;

WorkQueue::WorkQueue(size_t maxItems)
	: m_MaxItems(maxItems)
{ }

WorkQueue::~WorkQueue(void)
{
	Join();
}

/**
 * Enqueues a work item. Work items are guaranteed to be executed in the order
 * they were enqueued in.
 */
void WorkQueue::Enqueue(const WorkCallback& item)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	while (m_Items.size() >= m_MaxItems)
		m_CV.wait(lock);

	m_Items.push_back(item);
	m_CV.notify_all();

	if (!m_Executing) {
		m_Executing = true;
		Utility::QueueAsyncCallback(boost::bind(&WorkQueue::ExecuteItem, this));
	}
}

void WorkQueue::Join(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	while (m_Executing || !m_Items.empty())
		m_CV.wait(lock);
}

void WorkQueue::Clear(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	m_Items.clear();
	m_CV.notify_all();
}

void WorkQueue::ExecuteItem(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	while (!m_Items.empty()) {
		try {
			WorkCallback wi = m_Items.front();
			m_Items.pop_front();
			m_CV.notify_all();

			lock.unlock();
			wi();
			lock.lock();
		} catch (...) {
			lock.lock();
			m_Executing = false;
			throw;
		}
	}

	m_Executing = false;
}
