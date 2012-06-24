#include "i2-icinga.h"
#ifndef _MSC_VER
#	include "popen_noshell.h"
#endif /* _MSC_VER */

using namespace icinga;

boost::mutex NagiosCheckTask::m_Mutex;
deque<NagiosCheckTask::Ptr> NagiosCheckTask::m_Tasks;
condition_variable NagiosCheckTask::m_TasksCV;

NagiosCheckTask::NagiosCheckTask(const Service& service)
	: CheckTask(service), m_FP(NULL)
{
	string checkCommand = service.GetCheckCommand();
	m_Command = MacroProcessor::ResolveMacros(checkCommand, service.GetMacros()); // + " 2>&1";
}

void NagiosCheckTask::Enqueue(void)
{
	time(&m_Result.StartTime);

	{
			mutex::scoped_lock lock(m_Mutex);
			m_Tasks.push_back(GetSelf());
	}
}

void NagiosCheckTask::FlushQueue(void)
{
	m_TasksCV.notify_all();
}

CheckResult NagiosCheckTask::GetResult(void)
{
	return m_Result;
}

void NagiosCheckTask::CheckThreadProc(void)
{
	mutex::scoped_lock lock(m_Mutex);

	map<int, NagiosCheckTask::Ptr> tasks;
	const int maxTasks = 16;

	for (;;) {
		while (m_Tasks.empty() || tasks.size() >= maxTasks) {
			lock.unlock();

			map<int, NagiosCheckTask::Ptr>::iterator it, prev;

#ifndef _MSC_VER
			fd_set readfds;
			int nfds = 0;
			
			FD_ZERO(&readfds);

			for (it = tasks.begin(); it != tasks.end(); it++) {
				if (it->first > nfds)
					nfds = it->first;

				FD_SET(it->first, &readfds);
			}

			timeval tv;
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			select(nfds + 1, &readfds, NULL, NULL, &tv);
#else /* _MSC_VER */
			Sleep(1000);
#endif /* _MSC_VER */

			for (it = tasks.begin(); it != tasks.end(); ) {
#ifndef _MSC_VER
				if (!FD_ISSET(it->first, &readfds)) {
					it++;
					continue;
				}
#endif /* _MSC_VER */

				if (!it->second->RunTask()) {
					CheckTask::FinishTask(it->second);
					it = tasks.erase(it);
				} else {
					it++;
				}
			}

			lock.lock();
		}

		while (!m_Tasks.empty() && tasks.size() < maxTasks) {
			NagiosCheckTask::Ptr task = m_Tasks.front();
			m_Tasks.pop_front();
			if (!task->InitTask()) {
				CheckTask::FinishTask(task);
			} else {
				int fd = task->GetFD();
				if (fd >= 0)
					tasks[fd] = task;
			}
		}
	}
}

bool NagiosCheckTask::InitTask(void)
{
#ifdef _MSC_VER
	m_FP = _popen(m_Command.c_str(), "r");
#else /* _MSC_VER */
	bool use_libc_popen = false;

	popen_noshell_pass_to_pclose pclose_arg;

	if (!use_libc_popen) {
		m_FP = popen_noshell_compat(m_Command.c_str(), "r", &pclose_arg);

		if (m_FP == NULL) // TODO: add check for valgrind
			use_libc_popen = true;
	}

	if (use_libc_popen)
		m_FP = popen(m_Command.c_str(), "r");
#endif /* _MSC_VER */

	return (m_FP != NULL);
}

bool NagiosCheckTask::RunTask(void)
{
	char buffer[512];
	size_t read = fread(buffer, 1, sizeof(buffer), m_FP);

	if (read > 0)
		m_OutputStream.write(buffer, read);

	if (!feof(m_FP))
		return true;

	m_Result.Output = m_OutputStream.str();
	boost::algorithm::trim(m_Result.Output);

	int status, exitcode;
#ifdef _MSC_VER
	status = _pclose(m_FP);
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

	return false;
}

int NagiosCheckTask::GetFD(void) const
{
	return fileno(m_FP);
}

CheckTask::Ptr NagiosCheckTask::CreateTask(const Service& service)
{
	return boost::make_shared<NagiosCheckTask>(service);
}

void NagiosCheckTask::Register(void)
{
	CheckTask::RegisterType("nagios", NagiosCheckTask::CreateTask, NagiosCheckTask::FlushQueue);

	for (int i = 0; i < 1; i++) {
		thread t(&NagiosCheckTask::CheckThreadProc);
		t.detach();
	}
}