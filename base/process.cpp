/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "i2-base.h"

#ifndef _MSC_VER
#	include "popen_noshell.h"
#endif /* _MSC_VER */

using namespace icinga;

bool Process::m_ThreadCreated = false;
boost::mutex Process::m_Mutex;
deque<Process::Ptr> Process::m_Tasks;
condition_variable Process::m_TasksCV;

Process::Process(const string& command)
	: AsyncTask<Process, ProcessResult>(), m_Command(command), m_UsePopen(false)
{
	if (!m_ThreadCreated) {
		thread t(&Process::WorkerThreadProc);
		t.detach();

		m_ThreadCreated = true;
	}
}

void Process::Run(void)
{
	mutex::scoped_lock lock(m_Mutex);
	m_Tasks.push_back(GetSelf());
	m_TasksCV.notify_all();
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

					task->Finish(task->m_Result);
				} else {
					it++;
				}
			}

			lock.lock();
		}

		while (!m_Tasks.empty() && tasks.size() < MaxTasksPerThread) {
			Process::Ptr task = m_Tasks.front();
			m_Tasks.pop_front();

			lock.unlock();

			if (task->CallWithExceptionGuard(boost::bind(&Process::InitTask, task))) {
				int fd = task->GetFD();
				if (fd >= 0)
					tasks[fd] = task;
			}

			lock.lock();
		}
	}
}

void Process::InitTask(void)
{
	time(&m_Result.ExecutionStart);

#ifdef _MSC_VER
	m_FP = _popen(m_Command.c_str(), "r");
#else /* _MSC_VER */
	if (!m_UsePopen) {
		m_PCloseArg = new popen_noshell_pass_to_pclose;

		m_FP = popen_noshell_compat(m_Command.c_str(), "r",
		    (popen_noshell_pass_to_pclose *)m_PCloseArg);

		if (m_FP == NULL)
			m_UsePopen = true;
	}

	if (m_UsePopen)
		m_FP = popen(m_Command.c_str(), "r");
#endif /* _MSC_VER */

	if (m_FP == NULL)
		throw runtime_error("Could not create process.");
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

	time(&m_Result.ExecutionEnd);

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

	m_Result.ExitStatus = exitcode;
	m_Result.Output = output;

	return false;
}

int Process::GetFD(void) const
{
	return fileno(m_FP);
}

