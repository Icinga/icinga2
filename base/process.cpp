#include "i2-base.h"

using namespace icinga;

bool Process::m_ThreadsCreated = false;
boost::mutex Process::m_Mutex;
deque<Process::Ptr> Process::m_Tasks;
condition_variable Process::m_TasksCV;

Process::Process(const string& command)
	: m_Command(command)
{
	if (!m_ThreadsCreated) {
		int numThreads = boost::thread::hardware_concurrency();

		if (numThreads < 4)
			numThreads = 4;

		for (int i = 0; i < numThreads; i++) {
			thread t(&Process::WorkerThreadProc);
			t.detach();
		}

		m_ThreadsCreated = true;
	}
}

void Process::Run(void)
{
	mutex::scoped_lock lock(m_Mutex);
	m_Tasks.push_back(GetSelf());
	m_TasksCV.notify_one();
}

void Process::WorkerThreadProc(void)
{
	mutex::scoped_lock lock(m_Mutex);

	map<int, Process::Ptr> tasks;

	for (;;) {
		while (m_Tasks.empty() || tasks.size() >= MaxTasksPerThread) {
			lock.unlock();

			map<int, Process::Ptr>::iterator it, prev;

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
				int fd = it->first;
				Process::Ptr task = it->second;

#ifndef _MSC_VER
				if (!FD_ISSET(fd, &readfds)) {
					it++;
					continue;
				}
#endif /* _MSC_VER */

				if (!task->RunTask()) {
					prev = it;
					it++;
					tasks.erase(prev);

					task->Finish();
				} else {
					it++;
				}
			}

			lock.lock();
		}

		while (!m_Tasks.empty() && tasks.size() < MaxTasksPerThread) {
			Process::Ptr task = m_Tasks.front();
			m_Tasks.pop_front();
			if (!task->InitTask()) {
				task->Finish();
			} else {
				int fd = task->GetFD();
				if (fd >= 0)
					tasks[fd] = task;
			}
		}
	}
}

bool Process::InitTask(void)
{
#ifdef _MSC_VER
	m_FP = _popen(m_Command.c_str(), "r");
#else /* _MSC_VER */
	if (!m_UsePopen) {
		m_PCloseArg = new popen_noshell_pass_to_pclose;

		m_FP = popen_noshell_compat(m_Command.c_str(), "r",
		    (popen_noshell_pass_to_pclose *)m_PCloseArg);

		if (m_FP == NULL) // TODO: add check for valgrind
			m_UsePopen = true;
	}

	if (m_UsePopen)
		m_FP = popen(m_Command.c_str(), "r");
#endif /* _MSC_VER */

	if (m_FP == NULL) {
		return false;
	}

	return true;
}

bool Process::RunTask(void)
{
	char buffer[512];
	size_t read = fread(buffer, 1, sizeof(buffer), m_FP);

	if (read > 0)
		m_OutputStream.write(buffer, read);

	if (!feof(m_FP))
		return true;

	string output = m_OutputStream.str();

	int status, exitcode;
#ifdef _MSC_VER
	status = _pclose(m_FP);
#else /* _MSC_VER */
	if (m_UsePopen) {
		status = pclose(m_FP);
	} else {
		status = pclose_noshell((popen_noshell_pass_to_pclose *)m_PCloseArg);
		delete (popen_noshell_pass_to_pclose *)m_PCloseArg;
	}
#endif /* _MSC_VER */

#ifndef _MSC_VER
	if (WIFEXITED(status)) {
		exitcode = WEXITSTATUS(status);
#else /* _MSC_VER */
		exitcode = status;

		/* cmd.exe returns error code 1 (warning) when the plugin
		 * could not be executed - change the exit status to "unknown"
		 * when we have no plugin output. */
		if (output.empty())
			exitcode = 128;
#endif /* _MSC_VER */

#ifndef _MSC_VER
	} else if (WIFSIGNALED(status)) {
		stringstream outputbuf;
		outputbuf << "Process was terminated by signal " << WTERMSIG(status);
		output = outputbuf.str();
		exitcode = 128;
	}
#endif /* _MSC_VER */

	m_ExitStatus = exitcode;
	m_Output = output;

	return false;
}

int Process::GetFD(void) const
{
	return fileno(m_FP);
}

long Process::GetExitStatus(void) const
{
	return m_ExitStatus;
}

string Process::GetOutput(void) const
{
	return m_Output;
}
