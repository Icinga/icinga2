/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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
#include <atomic>

namespace icinga
{

enum WorkQueuePriority
{
	PriorityLow,
	PriorityNormal,
	PriorityHigh
};

using TaskFunction = std::function<void ()>;

struct Task
{
	Task() = default;

	Task(TaskFunction function, WorkQueuePriority priority, int id)
		: Function(std::move(function)), Priority(priority), ID(id)
	{ }

	TaskFunction Function;
	WorkQueuePriority Priority{PriorityNormal};
	int ID{-1};
};

bool operator<(const Task& a, const Task& b);

/**
 * A workqueue.
 *
 * @ingroup base
 */
class WorkQueue
{
public:
	typedef std::function<void (boost::exception_ptr)> ExceptionCallback;

	WorkQueue(size_t maxItems = 0, int threadCount = 1);
	~WorkQueue();

	void SetName(const String& name);
	String GetName() const;

	boost::mutex::scoped_lock AcquireLock();
	void EnqueueUnlocked(boost::mutex::scoped_lock& lock, TaskFunction&& function, WorkQueuePriority priority = PriorityNormal);
	void Enqueue(TaskFunction&& function, WorkQueuePriority priority = PriorityNormal,
		bool allowInterleaved = false);
	void Join(bool stop = false);

	template<typename VectorType, typename FuncType>
	void ParallelFor(const VectorType& items, const FuncType& func)
	{
		using SizeType = decltype(items.size());

		SizeType totalCount = items.size();

		auto lock = AcquireLock();

		SizeType offset = 0;

		for (int i = 0; i < m_ThreadCount; i++) {
			SizeType count = totalCount / static_cast<SizeType>(m_ThreadCount);
			if (static_cast<SizeType>(i) < totalCount % static_cast<SizeType>(m_ThreadCount))
				count++;

			EnqueueUnlocked(lock, [&items, func, offset, count, this]() {
				for (SizeType j = offset; j < offset + count; j++) {
					RunTaskFunction([&func, &items, j]() {
						func(items[j]);
					});
				}
			});

			offset += count;
		}

		ASSERT(offset == items.size());
	}

	bool IsWorkerThread() const;

	size_t GetLength() const;
	size_t GetTaskCount(RingBuffer::SizeType span);

	void SetExceptionCallback(const ExceptionCallback& callback);

	bool HasExceptions() const;
	std::vector<boost::exception_ptr> GetExceptions() const;
	void ReportExceptions(const String& facility) const;

protected:
	void IncreaseTaskCount();

private:
	int m_ID;
	String m_Name;
	static std::atomic<int> m_NextID;
	int m_ThreadCount;
	bool m_Spawned{false};

	mutable boost::mutex m_Mutex;
	boost::condition_variable m_CVEmpty;
	boost::condition_variable m_CVFull;
	boost::condition_variable m_CVStarved;
	boost::thread_group m_Threads;
	size_t m_MaxItems;
	bool m_Stopped{false};
	int m_Processing{0};
	std::priority_queue<Task, std::deque<Task> > m_Tasks;
	int m_NextTaskID{0};
	ExceptionCallback m_ExceptionCallback;
	std::vector<boost::exception_ptr> m_Exceptions;
	Timer::Ptr m_StatusTimer;
	double m_StatusTimerTimeout;

	RingBuffer m_TaskStats;
	size_t m_PendingTasks{0};
	double m_PendingTasksTimestamp{0};

	void WorkerThreadProc();
	void StatusTimerHandler();

	void RunTaskFunction(const TaskFunction& func);
};

}

#endif /* WORKQUEUE_H */
