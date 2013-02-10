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
#include <execvpe.h>
#endif /* _MSC_VER */

using namespace icinga;

bool Process::m_ThreadCreated = false;
boost::mutex Process::m_Mutex;
deque<Process::Ptr> Process::m_Tasks;
#ifndef _MSC_VER
int Process::m_TaskFd;
extern char **environ;
#endif /* _MSC_VER */

Process::Process(const vector<String>& arguments, const Dictionary::Ptr& extraEnvironment)
	: AsyncTask<Process, ProcessResult>(), m_FP(NULL)
{
	assert(Application::IsMainThread());

	if (!m_ThreadCreated) {
#ifndef _MSC_VER
		int fds[2];

		if (pipe(fds) < 0)
			BOOST_THROW_EXCEPTION(PosixException("pipe() failed.", errno));

		m_TaskFd = fds[1];
#endif /* _MSC_VER */

		for (int i = 0; i < thread::hardware_concurrency(); i++) {
			int childTaskFd;

#ifdef _MSC_VER
			childTaskFd = 0;
#else /* _MSC_VER */
			childTaskFd = dup(fds[0]);

			if (childTaskFd < 0)
				BOOST_THROW_EXCEPTION(PosixException("dup() failed.", errno));

			int flags;
			flags = fcntl(childTaskFd, F_GETFL, 0);
			if (flags < 0)
				BOOST_THROW_EXCEPTION(PosixException("fcntl failed", errno));

			if (fcntl(childTaskFd, F_SETFL, flags | O_NONBLOCK) < 0)
				BOOST_THROW_EXCEPTION(PosixException("fcntl failed", errno));
#endif /* _MSC_VER */

			thread t(&Process::WorkerThreadProc, childTaskFd);
			t.detach();
		}

		m_ThreadCreated = true;
	}

	// build argv
	m_Arguments = new char *[arguments.size() + 1];

	for (int i = 0; i < arguments.size(); i++)
		m_Arguments[i] = strdup(arguments[i].CStr());

	m_Arguments[arguments.size()] = NULL;

	// build envp
	int envc = 0;

	/* count existing environment variables */
	while (environ[envc] != NULL)
		envc++;

	m_Environment = new char *[envc + (extraEnvironment ? extraEnvironment->GetLength() : 0) + 1];

	for (int i = 0; i < envc; i++)
		m_Environment[i] = strdup(environ[i]);

	if (extraEnvironment) {
		String key;
		Value value;
		int index = envc;
		BOOST_FOREACH(tie(key, value), extraEnvironment) {
			String kv = key + "=" + Convert::ToString(value);
			m_Environment[index] = strdup(kv.CStr());
			index++;
		}
	}

	m_Environment[envc + (extraEnvironment ? extraEnvironment->GetLength() : 0)] = NULL;
}

vector<String> Process::ParseCommand(const String& command)
{
	// TODO: implement
	vector<String> args;
#ifdef _MSC_VER
	args.push_back(command);
#else /* MSC_VER */
	args.push_back("sh");
	args.push_back("-c");
	args.push_back(command);
#endif
	return args;
}

void Process::Run(void)
{
	{
		boost::mutex::scoped_lock lock(m_Mutex);
		m_Tasks.push_back(GetSelf());
	}

#ifndef _MSC_VER
	/**
	 * This little gem which is commonly known as the "self-pipe trick"
	 * takes care of waking up the select() call in the worker thread.
	 */
	if (write(m_TaskFd, "T", 1) < 0)
		BOOST_THROW_EXCEPTION(PosixException("write() failed.", errno));
#endif /* _MSC_VER */
}

void Process::WorkerThreadProc(int taskFd)
{
	map<int, Process::Ptr> tasks;

	for (;;) {
		map<int, Process::Ptr>::iterator it, prev;

#ifndef _MSC_VER
		fd_set readfds;
		int nfds = 0;

		FD_ZERO(&readfds);

		if (tasks.size() < MaxTasksPerThread)
			FD_SET(taskFd, &readfds);

		if (taskFd > nfds)
			nfds = taskFd;

		int fd;
		BOOST_FOREACH(tie(fd, tuples::ignore), tasks) {
			if (fd > nfds)
				nfds = fd;

			FD_SET(fd, &readfds);
		}

		int rc = select(nfds + 1, &readfds, NULL, NULL, NULL);

		if (rc < 0 && errno != EINTR)
			BOOST_THROW_EXCEPTION(PosixException("select() failed.", errno));

		if (rc == 0)
			continue;

#else /* _MSC_VER */
		Utility::Sleep(1);
#endif /* _MSC_VER */

#ifndef _MSC_VER
		if (FD_ISSET(taskFd, &readfds)) {
#endif /* _MSC_VER */

			while (tasks.size() < MaxTasksPerThread) {
				Process::Ptr task;

				{
					boost::mutex::scoped_lock lock(m_Mutex);

					/* Read one byte for every task we take from the pending tasks list. */
					char buffer;
					int rc = read(taskFd, &buffer, sizeof(buffer));

					if (rc < 0) {
						if (errno == EAGAIN)
							break; /* Someone else was faster and took our task. */

						BOOST_THROW_EXCEPTION(PosixException("read() failed.", errno));
					}

					assert(!m_Tasks.empty());

					task = m_Tasks.front();
					m_Tasks.pop_front();
				}

				try {
					task->InitTask();

					int fd = fileno(task->m_FP);
					if (fd >= 0)
						tasks[fd] = task;
				} catch (...) {
					Event::Post(boost::bind(&Process::FinishException, task, boost::current_exception()));
				}
			}
#ifndef _MSC_VER
		}
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

				Event::Post(boost::bind(&Process::FinishResult, task, task->m_Result));
			} else {
				it++;
			}
		}
	}
}

void Process::InitTask(void)
{
	m_Result.ExecutionStart = Utility::GetTime();

	assert(m_FP == NULL);

#ifdef _MSC_VER
	String cmdLine;

	// This is almost certainly wrong, but will have to do for now. (#3684)
	for (int i = 0; m_Arguments[i] != NULL ; i++) {
		cmdLine += "\"";
		cmdLine += m_Arguments[i];
		cmdLine += "\" ";
	}

	// free arguments
	for (int i = 0; m_Arguments[i] != NULL; i++)
		free(m_Arguments[i]);

	delete [] m_Arguments;

	// free environment
	for (int i = 0; m_Environment[i] != NULL; i++)
		free(m_Environment[i]);

	delete [] m_Environment;

	m_FP = _popen(cmdLine.CStr(), "r");
#else /* _MSC_VER */
	int fds[2];

	if (pipe(fds) < 0)
		BOOST_THROW_EXCEPTION(PosixException("pipe() failed.", errno));

#ifdef HAVE_VFORK
	m_Pid = vfork();
#else /* HAVE_VFORK */
	m_Pid = fork();
#endif /* HAVE_VFORK */

	if (m_Pid < 0)
		BOOST_THROW_EXCEPTION(PosixException("fork() failed.", errno));

	if (m_Pid == 0) {
		// child process

		if (dup2(fds[1], STDOUT_FILENO) < 0 || dup2(fds[1], STDERR_FILENO) < 0) {
			perror("dup2() failed.");
			_exit(128);
		}

		(void) close(fds[1]);

		if (execvpe(m_Arguments[0], m_Arguments, m_Environment) < 0) {
			perror("execvpe() failed.");
			_exit(128);
		}

		_exit(128);
	} else {
		// parent process

		// free arguments
		for (int i = 0; m_Arguments[i] != NULL; i++)
			free(m_Arguments[i]);

		delete [] m_Arguments;

		// free environment
		for (int i = 0; m_Environment[i] != NULL; i++)
			free(m_Environment[i]);

		delete [] m_Environment;

		(void) close(fds[1]);

		m_FP = fdopen(fds[0], "r");

		if (m_FP == NULL)
			BOOST_THROW_EXCEPTION(PosixException("fdopen() failed.", errno));
	}
#endif /* _MSC_VER */
}

bool Process::RunTask(void)
{
	char buffer[512];
	size_t read = fread(buffer, 1, sizeof(buffer), m_FP);

	if (read > 0)
		m_OutputStream.write(buffer, read);

	if (!feof(m_FP))
		return true;

	String output = m_OutputStream.str();

	int status, exitcode;

#ifdef _MSC_VER
	status = _pclose(m_FP);
#else /* _MSC_VER */
	fclose(m_FP);

	if (waitpid(m_Pid, &status, 0) != m_Pid)
		BOOST_THROW_EXCEPTION(PosixException("waitpid() failed.", errno));
#endif /* _MSC_VER */

#ifndef _MSC_VER
	if (WIFEXITED(status)) {
		exitcode = WEXITSTATUS(status);
#else /* _MSC_VER */
		exitcode = status;

		/* cmd.exe returns error code 1 (warning) when the plugin
		 * could not be executed - change the exit status to "unknown"
		 * when we have no plugin output. */
		if (output.IsEmpty())
			exitcode = 128;
#endif /* _MSC_VER */

#ifndef _MSC_VER
	} else if (WIFSIGNALED(status)) {
		stringstream outputbuf;
		outputbuf << "Process was terminated by signal " << WTERMSIG(status);
		output = outputbuf.str();
		exitcode = 128;
	} else {
		exitcode = 128;
	}
#endif /* _MSC_VER */

	m_Result.ExecutionEnd = Utility::GetTime();
	m_Result.ExitStatus = exitcode;
	m_Result.Output = output;

	return false;
}
