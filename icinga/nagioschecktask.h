#ifndef NAGIOSCHECKTASK_H
#define NAGIOSCHECKTASK_H

namespace icinga
{

class I2_ICINGA_API NagiosCheckTask : public CheckTask
{
public:
	NagiosCheckTask(const Service& service);

	virtual void Execute(void);
	virtual bool IsFinished(void) const;
	virtual CheckResult GetResult(void);

	static CheckTask::Ptr CreateTask(const Service& service);

private:
	string m_Command;
	packaged_task<CheckResult> m_Task;
	unique_future<CheckResult> m_Result;

	void InternalExecute(void);
	CheckResult RunCheck(void) const;
};

}

#endif /* NAGIOSCHECKTASK_H */
