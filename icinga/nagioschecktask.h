#ifndef NAGIOSCHECKTASK_H
#define NAGIOSCHECKTASK_H

namespace icinga
{

class I2_ICINGA_API NagiosCheckTask : public CheckTask
{
public:
	NagiosCheckTask(const Service& service);

	virtual void Enqueue(void);
	virtual bool IsFinished(void) const;
	virtual CheckResult GetResult(void);

	static CheckTask::Ptr CreateTask(const Service& service);
	static void FlushQueue(void);

private:
	string m_Command;
	packaged_task<CheckResult> m_Task;
	unique_future<CheckResult> m_Result;

	static vector<ThreadPool::Task> m_QueuedTasks;

	void Execute(void);
	CheckResult RunCheck(void) const;
};

}

#endif /* NAGIOSCHECKTASK_H */
