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
	unique_lock<mutex> lock(m_Lock);
	
	/* wait for all pending tasks */
	while (m_Tasks.size() > 0)
		m_CV.wait(lock);

	/* notify worker threads to exit */
	m_Alive = false;
	m_CV.notify_all();
}

void ThreadPool::EnqueueTask(Task task)
{
	unique_lock<mutex> lock(m_Lock);
	m_Tasks.push_back(task);
	m_CV.notify_one();
}

void ThreadPool::WorkerThreadProc(void)
{
	while (true) {
		Task task;

		{
			unique_lock<mutex> lock(m_Lock);

			while (m_Tasks.size() == 0) {
				m_CV.wait(lock);

				if (!m_Alive)
					return;
			}

			task = m_Tasks.front();
			m_Tasks.pop_front();
		}

		task();
	}
}

ThreadPool::Ptr ThreadPool::GetDefaultPool(void)
{
	static ThreadPool::Ptr threadPool;

	if (!threadPool)
		threadPool = boost::make_shared<ThreadPool>();

	return threadPool;
}
