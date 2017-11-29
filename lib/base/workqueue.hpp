/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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
#include "base/ringbuffer.hpp"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/exception_ptr.hpp>
#include <queue>
#include <deque>

namespace icinga
{

enum WorkQueuePriority
{
	PriorityLow,
	PriorityNormal,
	PriorityHigh
};

struct Task
{
	Task(void)
	    : Priority(PriorityNormal), ID(-1)
	{ }

	Task(std::function<void (void)>&& function, WorkQueuePriority priority, int id)
	    : Function(std::move(function)), Priority(priority), ID(id)
	{ }

	std::function<void (void)> Function;
	WorkQueuePriority Priority;
	int ID;
};

inline bool operator<(const Task& a, const Task& b)
{
	if (a.Priority < b.Priority)
		return true;

	if (a.Priority == b.Priority) {
		if (a.ID > b.ID)
			return true;
		else
			return false;
	}

	return false;
}

/**
 * A workqueue.
 *
 * @ingroup base
 */
class I2_BASE_API WorkQueue
{
public:
	typedef std::function<void (boost::exception_ptr)> ExceptionCallback;

	WorkQueue(size_t maxItems = 0, int threadCount = 1);
	~WorkQueue(void);

	void SetName(const String& name);
	String GetName(void) const;

	void Enqueue(std::function<void (void)>&& function, WorkQueuePriority priority = PriorityNormal,
	    bool allowInterleaved = false);
	void Join(bool stop = false);

	bool IsWorkerThread(void) const;

	size_t GetLength(void) const;
	int GetTaskCount(RingBuffer::SizeType span) const;

	void SetExceptionCallback(const ExceptionCallback& callback);

	bool HasExceptions(void) const;
	std::vector<boost::exception_ptr> GetExceptions(void) const;
	void ReportExceptions(const String& facility) const;

protected:
	void IncreaseTaskCount(void);

private:
	int m_ID;
	String m_Name;
	static int m_NextID;
	int m_ThreadCount;
	bool m_Spawned;

	mutable boost::mutex m_Mutex;
	boost::condition_variable m_CVEmpty;
	boost::condition_variable m_CVFull;
	boost::condition_variable m_CVStarved;
	boost::thread_group m_Threads;
	size_t m_MaxItems;
	bool m_Stopped;
	int m_Processing;
	std::priority_queue<Task, std::deque<Task> > m_Tasks;
	int m_NextTaskID;
	ExceptionCallback m_ExceptionCallback;
	std::vector<boost::exception_ptr> m_Exceptions;
	Timer::Ptr m_StatusTimer;
	double m_StatusTimerTimeout;

	mutable boost::mutex m_StatsMutex;
	RingBuffer m_TaskStats;
	int m_PendingTasks;
	double m_PendingTasksTimestamp;

	void WorkerThreadProc(void);
	void StatusTimerHandler(void);
};

}

#endif /* WORKQUEUE_H */
