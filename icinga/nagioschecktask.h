#ifndef NAGIOSCHECKTASK_H
#define NAGIOSCHECKTASK_H

namespace icinga
{

class I2_ICINGA_API NagiosCheckTask : public CheckTask
{
public:
	typedef shared_ptr<NagiosCheckTask> Ptr;
	typedef weak_ptr<NagiosCheckTask> WeakPtr;

	NagiosCheckTask(const Service& service);

	virtual void Enqueue(void);
	virtual CheckResult GetResult(void);

	static CheckTask::Ptr CreateTask(const Service& service);
	static void FlushQueue(void);
	static void GetFinishedTasks(vector<CheckTask::Ptr>& tasks);

private:
	string m_Command;
	CheckResult m_Result;

	static vector<ThreadPool::Task> m_QueuedTasks;

	static boost::mutex m_FinishedTasksMutex;
	static vector<CheckTask::Ptr> m_FinishedTasks;

	void Execute(void);
	CheckResult RunCheck(void) const;
};

}

#endif /* NAGIOSCHECKTASK_H */
