#ifndef CHECKTASK_H
#define CHECKTASK_H

namespace icinga
{

enum CheckState
{
	StateOK,
	StateWarning,
	StateCritical,
	StateUnreachable,
	StateUncheckable,
	StateUnknown
};

struct CheckResult
{
	time_t StartTime;
	time_t EndTime;

	CheckState State;
	string Output;
	Dictionary::Ptr PerformanceData;
};

class I2_ICINGA_API CheckTask : public Object
{
public:
	typedef shared_ptr<CheckTask> Ptr;
	typedef weak_ptr<CheckTask> WeakPtr;

	typedef function<CheckTask::Ptr(const Service&)> Factory;

	Service GetService(void) const;

	virtual void Execute(void) = 0;
	virtual bool IsFinished(void) const = 0;
	virtual CheckResult GetResult(void) = 0;

	static void RegisterType(string type, Factory factory);
	static CheckTask::Ptr CreateTask(const Service& service);

protected:
	CheckTask(const Service& service);

private:
	Service m_Service;

	static map<string, Factory> m_Types;
};

}

#endif /* CHECKTASK_H */
