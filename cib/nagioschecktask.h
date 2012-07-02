#ifndef NAGIOSCHECKTASK_H
#define NAGIOSCHECKTASK_H

namespace icinga
{

class I2_CIB_API NagiosCheckTask : public CheckTask
{
public:
	typedef shared_ptr<NagiosCheckTask> Ptr;
	typedef weak_ptr<NagiosCheckTask> WeakPtr;

	static const int MaxChecksPerThread = 128;

	NagiosCheckTask(const Service& service);

	virtual void Enqueue(void);

	static CheckTask::Ptr CreateTask(const Service& service);
	static void FlushQueue(void);

	static void Register(void);

private:
	string m_Command;

	FILE *m_FP;
	stringstream m_OutputStream;
	bool m_UsePopen;
#ifndef _MSC_VER
	void *m_PCloseArg;
#endif /* _MSC_VER */

	static boost::mutex m_Mutex;
	static deque<NagiosCheckTask::Ptr> m_Tasks;
	static vector<NagiosCheckTask::Ptr> m_PendingTasks;
	static condition_variable m_TasksCV;

	static void CheckThreadProc(void);

	bool InitTask(void);
	void ProcessCheckOutput(const string& output);
	bool RunTask(void);
	int GetFD(void) const;
};

}

#endif /* NAGIOSCHECKTASK_H */
