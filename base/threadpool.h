#ifndef THREADPOOL_H
#define THREADPOOL_H

namespace icinga
{

struct ThreadPoolTask
{
	typedef shared_ptr<ThreadPoolTask> Ptr;
	typedef weak_ptr<ThreadPoolTask> WeakPtr;

	virtual void Execute(void) = 0;
};

class I2_BASE_API ThreadPool : public Object
{
public:
	typedef shared_ptr<ThreadPool> Ptr;
	typedef weak_ptr<ThreadPool> WeakPtr;

	ThreadPool(long numThreads = 128);
	~ThreadPool(void);

	static ThreadPool::Ptr GetDefaultPool(void);

	void EnqueueTasks(list<ThreadPoolTask::Ptr>& tasks);
	void EnqueueTask(const ThreadPoolTask::Ptr& task);
	void WaitForTasks(void);

private:
	mutable mutex m_Lock;
	condition_variable m_CV;

	list<ThreadPoolTask::Ptr> m_Tasks;

	thread_group m_Threads;
	bool m_Alive;

	ThreadPoolTask::Ptr DequeueTask(void);
	void WorkerThreadProc(void);
};

}

#endif /* THREADPOOL_H */
