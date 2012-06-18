#include "i2-base.h"

using namespace icinga;

ThreadPool::ThreadPool(long numThreads)
	: m_Alive(true)
{
	for (long i = 0; i < numThreads; i++) {
		thread *thr = m_Threads.create_thread(boost::bind(&ThreadPool::WorkerThreadProc, this));
#ifdef _WIN32
		HANDLE handle = thr->native_handle();
		SetPriorityClass(handle, BELOW_NORMAL_PRIORITY_CLASS);
#else /* _WIN32 */
		pthread_t handle = thr->native_handle();

		int policy;
		sched_param param;

		if (pthread_getschedparam(handle, &policy, &param) < 0)
			throw PosixException("pthread_getschedparam failed", errno);

		param.sched_priority = 0;

		if (pthread_setschedparam(handle, SCHED_IDLE, &param) < 0)
			throw PosixException("pthread_setschedparam failed", errno);
#endif /* _WIN32 */
	}
}

ThreadPool::~ThreadPool(void)
{
	{
		unique_lock<mutex> lock(m_Lock);

		m_Tasks.clear();

		/* notify worker threads to exit */
		m_Alive = false;
		m_CV.notify_all();
	}

	m_Threads.join_all();
}

void ThreadPool::EnqueueTasks(const vector<Task>& tasks)
{
	unique_lock<mutex> lock(m_Lock);

	std::copy(tasks.begin(), tasks.end(), std::back_inserter(m_Tasks));
	m_CV.notify_all();
}

void ThreadPool::EnqueueTask(Task task)
{
	unique_lock<mutex> lock(m_Lock);
	m_Tasks.push_back(task);
	m_CV.notify_one();
}

void ThreadPool::WaitForTasks(void)
{
	unique_lock<mutex> lock(m_Lock);

	/* wait for all pending tasks */
	while (m_Tasks.size() > 0)
		m_CV.wait(lock);
}

void ThreadPool::WorkerThreadProc(void)
{
	while (true) {
		Task task;

		{
			unique_lock<mutex> lock(m_Lock);

			while (m_Tasks.size() == 0) {
				if (!m_Alive)
					return;

				m_CV.wait(lock);
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
