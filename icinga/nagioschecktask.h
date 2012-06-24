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

	static void Register(void);

private:
	string m_Command;
	CheckResult m_Result;

	FILE *m_FP;
	stringstream m_OutputStream;

	static boost::mutex m_Mutex;
	static deque<NagiosCheckTask::Ptr> m_Tasks;
	static condition_variable m_TasksCV;

	static void CheckThreadProc(void);

	bool InitTask(void);
	bool RunTask(void);
	int GetFD(void) const;
};

}

#endif /* NAGIOSCHECKTASK_H */
