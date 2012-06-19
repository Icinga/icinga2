#ifndef THREADPOOL_H
#define THREADPOOL_H

namespace icinga
{

class I2_BASE_API ThreadPool : public Object
{
public:
	typedef shared_ptr<ThreadPool> Ptr;
	typedef weak_ptr<ThreadPool> WeakPtr;

	typedef function<void()> Task;

	ThreadPool(long numThreads = 16);
	~ThreadPool(void);

	static ThreadPool::Ptr GetDefaultPool(void);

	void EnqueueTasks(const vector<Task>& tasks);
	void EnqueueTask(Task task);
	void WaitForTasks(void);

private:
	mutex m_Lock;
	condition_variable m_CV;

	deque<Task> m_Tasks;

	thread_group m_Threads;
	bool m_Alive;

	void WorkerThreadProc(void);
};

}

#endif /* THREADPOOL_H */
