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
		mutex::scoped_lock lock(m_Lock);

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
		mutex::scoped_lock lock(m_Lock);
		m_Tasks.splice(m_Tasks.end(), tasks, tasks.begin(), tasks.end());
	}

	m_CV.notify_all();
}

void ThreadPool::EnqueueTask(const ThreadPoolTask::Ptr& task)
{
	{
		mutex::scoped_lock lock(m_Lock);
		m_Tasks.push_back(task);
	}

	m_CV.notify_one();
}


ThreadPoolTask::Ptr ThreadPool::DequeueTask(void)
{
	mutex::scoped_lock lock(m_Lock);

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
	mutex::scoped_lock lock(m_Lock);

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

