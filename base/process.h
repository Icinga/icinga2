#ifndef PROCESS_H
#define PROCESS_H

namespace icinga
{

class I2_BASE_API Process : public AsyncTask<Process>
{
public:
	typedef shared_ptr<Process> Ptr;
	typedef weak_ptr<Process> WeakPtr;

	static const int MaxTasksPerThread = 128;

	Process(const string& command);

	long GetExitStatus(void) const;
	string GetOutput(void) const;

private:
	static bool m_ThreadsCreated;

	string m_Command;

	FILE *m_FP;
	stringstream m_OutputStream;
	bool m_UsePopen;
#ifndef _MSC_VER
	void *m_PCloseArg;
#endif /* _MSC_VER */

	long m_ExitStatus;
	string m_Output;

	virtual void Run(void);

	static boost::mutex m_Mutex;
	static deque<Process::Ptr> m_Tasks;
	static condition_variable m_TasksCV;

	static void WorkerThreadProc(void);

	bool InitTask(void);
	bool RunTask(void);

	int GetFD(void) const;
};

}

#endif /* PROCESS_H */
