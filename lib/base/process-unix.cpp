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

#include "base/process.h"
#include "base/exception.h"
#include "base/convert.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include <map>
#include <boost/bind.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp>

#ifndef _WIN32
#include <execvpe.h>

using namespace icinga;

boost::condition_variable Process::m_CV;
int Process::m_TaskFd;
Timer::Ptr Process::m_StatusTimer;

#ifndef __APPLE__
extern char **environ;
#else /* __APPLE__ */
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#endif /* __APPLE__ */

void Process::Initialize(void)
{
	int fds[2];

#if HAVE_PIPE2
	if (pipe2(fds, O_CLOEXEC) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("pipe2")
		    << boost::errinfo_errno(errno));
	}
#else /* HAVE_PIPE2 */
	if (pipe(fds) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("pipe")
		    << boost::errinfo_errno(errno));
	}

	/* Don't bother setting fds[0] to clo-exec as we'll only
	 * use it in the following dup() call. */

	Utility::SetCloExec(fds[1]);
#endif /* HAVE_PIPE2 */

	m_TaskFd = fds[1];

	unsigned int threads = boost::thread::hardware_concurrency();

	if (threads == 0)
		threads = 2;

	for (unsigned int i = 0; i < threads; i++) {
		int childTaskFd = dup(fds[0]);

		if (childTaskFd < 0) {
			BOOST_THROW_EXCEPTION(posix_error()
			    << boost::errinfo_api_function("dup")
			    << boost::errinfo_errno(errno));
		}

		Utility::SetNonBlocking(childTaskFd);
		Utility::SetCloExec(childTaskFd);

		boost::thread t(&Process::WorkerThreadProc, childTaskFd);
		t.detach();
	}

	(void) close(fds[0]);

	m_StatusTimer = boost::make_shared<Timer>();
	m_StatusTimer->OnTimerExpired.connect(boost::bind(&Process::StatusTimerHandler));
	m_StatusTimer->SetInterval(5);
	m_StatusTimer->Start();
}

void Process::WorkerThreadProc(int taskFd)
{
	std::map<int, Process::Ptr> tasks;
	pollfd *pfds = NULL;

	for (;;) {
		std::map<int, Process::Ptr>::iterator it, prev;

		pfds = (pollfd *)realloc(pfds, (1 + tasks.size()) * sizeof(pollfd));

		if (pfds == NULL) {
			BOOST_THROW_EXCEPTION(posix_error()
			    << boost::errinfo_api_function("realloc")
			    << boost::errinfo_errno(errno));
		}

		int idx = 0;

		int fd;
		BOOST_FOREACH(boost::tie(fd, boost::tuples::ignore), tasks) {
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

		if (rc < 0 && errno != EINTR) {
			BOOST_THROW_EXCEPTION(posix_error()
			    << boost::errinfo_api_function("poll")
			    << boost::errinfo_errno(errno));
		}

		if (rc == 0)
			continue;

		for (int i = 0; i < idx; i++) {
			if ((pfds[i].revents & (POLLIN|POLLHUP)) == 0)
				continue;

			if (pfds[i].fd == taskFd) {
				std::vector<Process::Ptr> new_tasks;

				unsigned int want = MaxTasksPerThread - tasks.size();

				if (want > 0) {
					boost::mutex::scoped_lock lock(m_Mutex);

					/* Read one byte for every task we take from the pending tasks list. */
					char buffer[MaxTasksPerThread];

					ASSERT(want <= sizeof(buffer));

					int have = read(taskFd, &buffer, want);

					if (have < 0) {
						if (errno == EAGAIN)
							break; /* Someone else was faster and took our task. */

						BOOST_THROW_EXCEPTION(posix_error()
						    << boost::errinfo_api_function("read")
						    << boost::errinfo_errno(errno));
					}

					while (have > 0) {
						ASSERT(!m_Tasks.empty());

						Process::Ptr task = m_Tasks.front();
						m_Tasks.pop_front();

						new_tasks.push_back(task);

						have--;
					}

					m_CV.notify_all();
				}

				BOOST_FOREACH(const Process::Ptr& task, new_tasks) {
					try {
						task->InitTask();

						int fd = task->m_FD;

						if (fd >= 0)
							tasks[fd] = task;
					} catch (...) {
						task->FinishException(boost::current_exception());
					}
				}

				continue;
			}

			it = tasks.find(pfds[i].fd);

			if (it == tasks.end())
				continue;

			Process::Ptr task = it->second;

			if (!task->RunTask()) {
				prev = it;
				tasks.erase(prev);

				task->FinishResult(task->m_Result);
			}
		}
	}
}

void Process::QueueTask(void)
{
	{
		boost::mutex::scoped_lock lock(m_Mutex);

		while (m_Tasks.size() >= PIPE_BUF)
			m_CV.wait(lock);

		m_Tasks.push_back(GetSelf());

		/**
		 * This little gem which is commonly known as the "self-pipe trick"
		 * takes care of waking up the select() call in the worker thread.
		 */
		if (write(m_TaskFd, "T", 1) < 0) {
			BOOST_THROW_EXCEPTION(posix_error()
			    << boost::errinfo_api_function("write")
			    << boost::errinfo_errno(errno));
		}
	}
}

void Process::InitTask(void)
{
	m_Result.ExecutionStart = Utility::GetTime();

	ASSERT(m_FD == -1);

	int fds[2];

#if HAVE_PIPE2
	if (pipe2(fds, O_NONBLOCK | O_CLOEXEC) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("pipe2")
		    << boost::errinfo_errno(errno));
	}
#else /* HAVE_PIPE2 */
	if (pipe(fds) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("pipe")
		    << boost::errinfo_errno(errno));
	}

	Utility::SetNonBlocking(fds[0]);
	Utility::SetCloExec(fds[0]);

	Utility::SetNonBlocking(fds[1]);
	Utility::SetCloExec(fds[1]);
#endif /* HAVE_PIPE2 */

	// build argv
	char **argv = new char *[m_Arguments.size() + 1];

	for (unsigned int i = 0; i < m_Arguments.size(); i++)
		argv[i] = strdup(m_Arguments[i].CStr());

	argv[m_Arguments.size()] = NULL;

	m_Arguments.clear();

	// build envp
	int envc = 0;

	/* count existing environment variables */
	while (environ[envc] != NULL)
		envc++;

	char **envp = new char *[envc + (m_ExtraEnvironment ? m_ExtraEnvironment->GetLength() : 0) + 1];

	for (int i = 0; i < envc; i++)
		envp[i] = strdup(environ[i]);

	if (m_ExtraEnvironment) {
		ObjectLock olock(m_ExtraEnvironment);

		String key;
		Value value;
		int index = envc;
		BOOST_FOREACH(boost::tie(key, value), m_ExtraEnvironment) {
			String kv = key + "=" + Convert::ToString(value);
			envp[index] = strdup(kv.CStr());
			index++;
		}
	}

	envp[envc + (m_ExtraEnvironment ? m_ExtraEnvironment->GetLength() : 0)] = NULL;

	m_ExtraEnvironment.reset();

#if HAVE_WORKING_VFORK
	m_Pid = vfork();
#else /* HAVE_WORKING_VFORK */
	m_Pid = fork();
#endif /* HAVE_WORKING_VFORK */

	if (m_Pid < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("fork")
		    << boost::errinfo_errno(errno));
	}

	if (m_Pid == 0) {
		// child process

		if (dup2(fds[1], STDOUT_FILENO) < 0 || dup2(fds[1], STDERR_FILENO) < 0) {
			perror("dup2() failed.");
			_exit(128);
		}

		(void) close(fds[0]);
		(void) close(fds[1]);

		if (execvpe(argv[0], argv, envp) < 0) {
			perror("execvpe() failed.");
			_exit(128);
		}

		_exit(128);
	}

	// parent process

	// free arguments
	for (int i = 0; argv[i] != NULL; i++)
		free(argv[i]);

	delete [] argv;

	// free environment
	for (int i = 0; envp[i] != NULL; i++)
		free(envp[i]);

	delete [] envp;

	m_FD = fds[0];
	(void) close(fds[1]);
}

bool Process::RunTask(void)
{
	char buffer[512];
	int rc;

	do {
		rc = read(m_FD, buffer, sizeof(buffer));

		if (rc > 0) {
			m_OutputStream.write(buffer, rc);
		}
	} while (rc > 0);

	if (rc < 0 && errno == EAGAIN)
		return true;

	String output = m_OutputStream.str();

	int status, exitcode;

	(void) close(m_FD);

	if (waitpid(m_Pid, &status, 0) != m_Pid) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("waitpid")
		    << boost::errinfo_errno(errno));
	}

	if (WIFEXITED(status)) {
		exitcode = WEXITSTATUS(status);
	} else if (WIFSIGNALED(status)) {
		std::ostringstream outputbuf;
		outputbuf << "Process was terminated by signal " << WTERMSIG(status);
		output = outputbuf.str();
		exitcode = 128;
	} else {
		exitcode = 128;
	}

	m_Result.ExecutionEnd = Utility::GetTime();
	m_Result.ExitStatus = exitcode;
	m_Result.Output = output;

	return false;
}

void Process::StatusTimerHandler(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	if (m_Tasks.size() > 50)
		Log(LogCritical, "base", "More than 50 waiting Process tasks: " +
		    Convert::ToString(m_Tasks.size()));
}

#endif /* _WIN32 */
