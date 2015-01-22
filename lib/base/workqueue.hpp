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

#ifndef WORKQUEUE_H
#define WORKQUEUE_H

#include "base/i2-base.hpp"
#include "base/timer.hpp"
#include <deque>
#include <boost/function.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/exception_ptr.hpp>

namespace icinga
{

typedef boost::function<void (void)> WorkCallback;

struct WorkItem
{
	WorkCallback Callback;
	bool AllowInterleaved;
};

/**
 * A workqueue.
 *
 * @ingroup base
 */
class I2_BASE_API WorkQueue
{
public:
	typedef boost::function<void (boost::exception_ptr)> ExceptionCallback;

	WorkQueue(size_t maxItems = 25000);
	~WorkQueue(void);

	void Enqueue(const WorkCallback& callback, bool allowInterleaved = false);
	void Join(bool stop = false);

	boost::thread::id GetThreadId(void) const;

	void SetExceptionCallback(const ExceptionCallback& callback);

	size_t GetLength(void);

private:
	int m_ID;
	static int m_NextID;

	boost::mutex m_Mutex;
	boost::condition_variable m_CVEmpty;
	boost::condition_variable m_CVFull;
	boost::condition_variable m_CVStarved;
	boost::thread m_Thread;
	size_t m_MaxItems;
	bool m_Stopped;
	bool m_Processing;
	std::deque<WorkItem> m_Items;
	ExceptionCallback m_ExceptionCallback;
	Timer::Ptr m_StatusTimer;

	void WorkerThreadProc(void);
	void StatusTimerHandler(void);

	static void DefaultExceptionCallback(boost::exception_ptr exp);
};

class I2_BASE_API ParallelWorkQueue
{
public:
	ParallelWorkQueue(void);
	~ParallelWorkQueue(void);

	void Enqueue(const boost::function<void(void)>& callback);
	void Join(void);

private:
	unsigned int m_QueueCount;
	WorkQueue *m_Queues;
	unsigned int m_Index;
};

}

#endif /* WORKQUEUE_H */
