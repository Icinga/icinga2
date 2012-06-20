#include "i2-icinga.h"
#include "popen_noshell.h"

using namespace icinga;

list<ThreadPoolTask::Ptr> NagiosCheckTask::m_QueuedTasks;

boost::mutex NagiosCheckTask::m_FinishedTasksMutex;
vector<CheckTask::Ptr> NagiosCheckTask::m_FinishedTasks;

NagiosCheckTask::NagiosCheckTask(const Service& service)
	: CheckTask(service)
{
	string checkCommand = service.GetCheckCommand();
	m_Command = MacroProcessor::ResolveMacros(checkCommand, service.GetMacros()); // + " 2>&1";
}

void NagiosCheckTask::Enqueue(void)
{
	time(&m_Result.StartTime);
	m_QueuedTasks.push_back(static_pointer_cast<ThreadPoolTask>(static_cast<NagiosCheckTask::Ptr>(GetSelf())));
//	m_QueuedTasks.push_back(new ThreadPool:Task(bind(&NagiosCheckTask::Execute, static_cast<NagiosCheckTask::Ptr>(GetSelf()))));
}

void NagiosCheckTask::FlushQueue(void)
{
	ThreadPool::GetDefaultPool()->EnqueueTasks(m_QueuedTasks);
	m_QueuedTasks.clear();
}

void NagiosCheckTask::GetFinishedTasks(vector<CheckTask::Ptr>& tasks)
{
	mutex::scoped_lock lock(m_FinishedTasksMutex);
	std::copy(m_FinishedTasks.begin(), m_FinishedTasks.end(), back_inserter(tasks));
	m_FinishedTasks.clear();
}

CheckResult NagiosCheckTask::GetResult(void)
{
	return m_Result;
}

void NagiosCheckTask::Execute(void)
{
	RunCheck();

	{
		mutex::scoped_lock lock(m_FinishedTasksMutex);
		m_FinishedTasks.push_back(GetSelf());
	}
}

void NagiosCheckTask::RunCheck(void)
{
	FILE *fp;

#ifdef _MSC_VER
	fp = _popen(m_Command.c_str(), "r");
#else /* _MSC_VER */
	bool use_libc_popen = false;

	popen_noshell_pass_to_pclose pclose_arg;

	if (!use_libc_popen) {
		fp = popen_noshell_compat(m_Command.c_str(), "r", &pclose_arg);

		if (fp == NULL) // TODO: add check for valgrind
			use_libc_popen = true;
	}

	if (use_libc_popen)
		fp = popen(m_Command.c_str(), "r");
#endif /* _MSC_VER */

	stringstream outputbuf;

	while (!feof(fp)) {
		char buffer[512];
		size_t read = fread(buffer, 1, sizeof(buffer), fp);

		if (read == 0)
			break;

		outputbuf.write(buffer, read);
	}

	m_Result.Output = outputbuf.str();
	boost::algorithm::trim(m_Result.Output);

	int status, exitcode;
#ifdef _MSC_VER
	status = _pclose(fp);
#else /* _MSC_VER */
	if (use_libc_popen)
		status = pclose(fp);
	else
		status = pclose_noshell(&pclose_arg);
#endif /* _MSC_VER */

#ifndef _MSC_VER
	if (WIFEXITED(status)) {
		exitcode = WEXITSTATUS(status);
#else /* _MSC_VER */
		exitcode = status;
#endif /* _MSC_VER */

		switch (exitcode) {
			case 0:
				m_Result.State = StateOK;
				break;
			case 1:
				m_Result.State = StateWarning;
				break;
			case 2:
				m_Result.State = StateCritical;
				break;
			default:
				m_Result.State = StateUnknown;
				break;
		}
#ifndef _MSC_VER
	} else if (WIFSIGNALED(status)) {
		m_Result.Output = "Process was terminated by signal " + WTERMSIG(status);
		m_Result.State = StateUnknown;
	}
#endif /* _MSC_VER */

	time(&m_Result.EndTime);
}

CheckTask::Ptr NagiosCheckTask::CreateTask(const Service& service)
{
	return boost::make_shared<NagiosCheckTask>(service);
}
