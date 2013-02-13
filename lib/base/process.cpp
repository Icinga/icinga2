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
	: AsyncTask<Process, ProcessResult>(),
#ifndef _MSC_VER
	m_FD(-1)
#else /* _MSC_VER */
	m_FP(NULL)
#endif /* _MSC_VER */
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

			if (fcntl(childTaskFd, F_SETFL, flags | O_NONBLOCK | O_CLOEXEC) < 0)
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
	pollfd *pfds = NULL;

	for (;;) {
		map<int, Process::Ptr>::iterator it, prev;

#ifndef _MSC_VER
		pfds = (pollfd *)realloc(pfds, (1 + tasks.size()) * sizeof(pollfd));

		if (pfds == NULL)
			BOOST_THROW_EXCEPTION(PosixException("realloc() failed.", errno));

		int idx = 0;

		int fd;
		BOOST_FOREACH(tie(fd, tuples::ignore), tasks) {
			pfds[idx].fd = fd;
			pfds[idx].events = POLLIN | POLLHUP;
			idx++;
		}

		if (tasks.size() < MaxTasksPerThread) {
			pfds[idx].fd = taskFd;
			pfds[idx].events = POLLIN;
			idx++;
		}

		int rc = poll(pfds, idx, -1);

		if (rc < 0 && errno != EINTR)
			BOOST_THROW_EXCEPTION(PosixException("poll() failed.", errno));

		if (rc == 0)
			continue;

#else /* _MSC_VER */
		Utility::Sleep(1);
#endif /* _MSC_VER */

#ifndef _MSC_VER
		for (int i = 0; i < idx; i++) {
			if ((pfds[i].revents & (POLLIN|POLLHUP)) == 0)
				continue;

			if (pfds[i].fd == taskFd) {
#endif /* _MSC_VER */

				while (tasks.size() < MaxTasksPerThread) {
					Process::Ptr task;

					{
						boost::mutex::scoped_lock lock(m_Mutex);

#ifndef _MSC_VER
						/* Read one byte for every task we take from the pending tasks list. */
						char buffer;
						int rc = read(taskFd, &buffer, sizeof(buffer));

						if (rc < 0) {
							if (errno == EAGAIN)
								break; /* Someone else was faster and took our task. */

							BOOST_THROW_EXCEPTION(PosixException("read() failed.", errno));
						}

						assert(!m_Tasks.empty());
#else /* _MSC_VER */
						if (m_Tasks.empty())
							break;
#endif /* _MSC_VER */

						task = m_Tasks.front();
						m_Tasks.pop_front();
					}

					try {
						task->InitTask();

#ifdef _MSC_VER
						int fd = fileno(task->m_FP);
#else /* _MSC_VER */
						int fd = task->m_FD;
#endif /* _MSC_VER */

						if (fd >= 0)
							tasks[fd] = task;
					} catch (...) {
						Event::Post(boost::bind(&Process::FinishException, task, boost::current_exception()));
					}
				}
#ifndef _MSC_VER

				continue;
			}

			it = tasks.find(pfds[i].fd);

			if (it == tasks.end())
				continue;
#else /* _MSC_VER */
			for (it = tasks.begin(); it != tasks.end(); ) {
				int fd = it->first;
#endif /* _MSC_VER */
				Process::Ptr task = it->second;

				if (!task->RunTask()) {
					prev = it;
#ifdef _MSC_VER
					it++;
#endif /* _MSC_VER */
					tasks.erase(prev);

					Event::Post(boost::bind(&Process::FinishResult, task, task->m_Result));
#ifdef _MSC_VER
				} else {
					it++;
#endif /* _MSC_VER */
				}
#ifdef _MSC_VER
			}
#else /* _MSC_VER */
		}
#endif /* _MSC_VER */
	}
}

void Process::InitTask(void)
{
	m_Result.ExecutionStart = Utility::GetTime();

#ifdef _MSC_VER
	assert(m_FP == NULL);
#else /* _MSC_VER */
	assert(m_FD == -1);
#endif /* _MSC_VER */

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

#ifdef HAVE_PIPE2
	if (pipe2(fds, O_NONBLOCK | O_CLOEXEC) < 0)
#else /* HAVE_PIPE2 */
	if (pipe(fds) < 0)
#endif /* HAVE_PIPE2 */
		BOOST_THROW_EXCEPTION(PosixException("pipe() failed.", errno));

#ifndef HAVE_PIPE2
	int flags;
	flags = fcntl(childTaskFd, F_GETFL, 0);
	if (flags < 0)
		BOOST_THROW_EXCEPTION(PosixException("fcntl failed", errno));

	if (fcntl(childTaskFd, F_SETFL, flags | O_NONBLOCK | O_CLOEXEC) < 0)
		BOOST_THROW_EXCEPTION(PosixException("fcntl failed", errno));
#endif /* HAVE_PIPE2 */

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

		(void) close(fds[0]);
		(void) close(fds[1]);

		if (execvpe(m_Arguments[0], m_Arguments, m_Environment) < 0) {
			perror("execvpe() failed.");
			_exit(128);
		}

		_exit(128);
	}

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

	m_FD = fds[0];
#endif /* _MSC_VER */
}

bool Process::RunTask(void)
{
	char buffer[512];
	int rc;

#ifndef _MSC_VER
	rc = read(m_FD, buffer, sizeof(buffer));
#else /* _MSC_VER */
	if (!feof(m_FP))
		rc =  fread(buffer, 1, sizeof(buffer), m_FP);
	else
		rc = 0;
#endif /* _MSC_VER */

	if (rc > 0) {
		m_OutputStream.write(buffer, rc);

		return true;
	}

	String output = m_OutputStream.str();

	int status, exitcode;

#ifdef _MSC_VER
	status = _pclose(m_FP);
#else /* _MSC_VER */
	(void) close(m_FD);

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
