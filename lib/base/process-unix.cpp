/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-present Icinga Development Team (http://www.icinga.org) *
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
#include "base/initialize.h"
#include "base/logger_fwd.h"
#include "base/utility.h"
#include "base/scriptvariable.h"
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp>
#include <boost/algorithm/string/join.hpp>

#ifndef _WIN32
#include <execvpe.h>
#include <poll.h>

using namespace icinga;

#ifndef __APPLE__
extern char **environ;
#else /* __APPLE__ */
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#endif /* __APPLE__ */

static boost::mutex l_ProcessMutex;
static std::map<int, Process::Ptr> l_Processes;
static int l_EventFDs[2];
static boost::once_flag l_OnceFlag = BOOST_ONCE_INIT;

INITIALIZE_ONCE(&Process::StaticInitialize);

void Process::StaticInitialize(void)
{
#ifdef HAVE_PIPE2
	if (pipe2(l_EventFDs, O_CLOEXEC) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("pipe2")
		    << boost::errinfo_errno(errno));
	}
#else /* HAVE_PIPE2 */
	if (pipe(l_EventFDs) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("pipe")
		    << boost::errinfo_errno(errno));
	}

	Utility::SetCloExec(l_EventFDs[0]);
	Utility::SetCloExec(l_EventFDs[1]);
#endif /* HAVE_PIPE2 */

	Utility::SetNonBlocking(l_EventFDs[0]);
	Utility::SetNonBlocking(l_EventFDs[1]);
}

void Process::ThreadInitialize(void)
{
	/* Note to self: Make sure this runs _after_ we've daemonized. */
	boost::thread t(&Process::IOThreadProc);
	t.detach();
}

void Process::IOThreadProc(void)
{
	pollfd *pfds = NULL;
	int count = 0;

	for (;;) {
		double timeout = 1;

		{
			boost::mutex::scoped_lock lock(l_ProcessMutex);

			count = 1 + l_Processes.size();
			pfds = reinterpret_cast<pollfd *>(realloc(pfds, sizeof(pollfd) * count));

			pfds[0].fd = l_EventFDs[0];
			pfds[0].events = POLLIN;
			pfds[0].revents = 0;

			int i = 1;
			std::pair<int, Process::Ptr> kv;
			BOOST_FOREACH(kv, l_Processes) {
				pfds[i].fd = kv.second->m_FD;
				pfds[i].events = POLLIN;
				pfds[i].revents = 0;

				if (kv.second->GetTimeout() != 0 && kv.second->GetTimeout() < timeout)
				    timeout = kv.second->GetTimeout();

				i++;
			}
		}

		int rc = poll(pfds, count, timeout * 1000);

		if (rc < 0)
			continue;

		{
			boost::mutex::scoped_lock lock(l_ProcessMutex);

			if (pfds[0].revents & (POLLIN|POLLHUP|POLLERR)) {
				char buffer[512];
				(void) read(l_EventFDs[0], buffer, sizeof(buffer));
			}

			for (int i = 1; i < count; i++) {
				if (pfds[i].revents & (POLLIN|POLLHUP|POLLERR)) {
					std::map<int, Process::Ptr>::iterator it;
					it = l_Processes.find(pfds[i].fd);

					if (it == l_Processes.end())
						continue; /* This should never happen. */

					if (!it->second->DoEvents()) {
						(void) close(it->first);
						l_Processes.erase(it);
					}
				}
			}
		}
	}
}

void Process::Run(const boost::function<void (const ProcessResult&)>& callback)
{
	boost::call_once(l_OnceFlag, &Process::ThreadInitialize);

	m_Result.ExecutionStart = Utility::GetTime();

	int fds[2];

#ifdef HAVE_PIPE2
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

	Utility::SetCloExec(fds[0]);
	Utility::SetCloExec(fds[1]);
#endif /* HAVE_PIPE2 */

	// build argv
	Log(LogDebug, "base", "Running command '" + boost::algorithm::join(m_Arguments, " ") + "'.");

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

		int index = envc;
		BOOST_FOREACH(const Dictionary::Pair& kv, m_ExtraEnvironment) {
			String skv = kv.first + "=" + Convert::ToString(kv.second);
			envp[index] = strdup(skv.CStr());
			index++;
		}
	}

	envp[envc + (m_ExtraEnvironment ? m_ExtraEnvironment->GetLength() : 0)] = NULL;

	m_ExtraEnvironment.reset();

#ifdef HAVE_VFORK
	Value use_vfork = ScriptVariable::Get("IcingaUseVfork");

	if (use_vfork.IsEmpty() || static_cast<bool>(use_vfork))
		m_Pid = vfork();
	else
		m_Pid = fork();
#else /* HAVE_VFORK */
	m_Pid = fork();
#endif /* HAVE_VFORK */

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

		(void) nice(5);

		if (icinga2_execvpe(argv[0], argv, envp) < 0) {
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

	(void) close(fds[1]);

	Utility::SetNonBlocking(fds[0]);

	m_FD = fds[0];
	m_Callback = callback;

	{
		boost::mutex::scoped_lock lock(l_ProcessMutex);
		l_Processes[m_FD] = GetSelf();
	}

	(void) write(l_EventFDs[1], "T", 1);
}

bool Process::DoEvents(void)
{
	if (m_Timeout != 0) {
		double timeout = m_Timeout - (Utility::GetTime() - m_Result.ExecutionStart);

		if (timeout < 0) {
			m_OutputStream << "<Timeout exceeded.>";
			kill(m_Pid, SIGKILL);
		}
	}

	char buffer[512];
	for (;;) {
		int rc = read(m_FD, buffer, sizeof(buffer));

		if (rc < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
			return true;

		if (rc > 0) {
			m_OutputStream.write(buffer, rc);
			return true;
		}

		break;
	}

	String output = m_OutputStream.str();

	int status, exitcode;

	if (waitpid(m_Pid, &status, 0) != m_Pid) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("waitpid")
		    << boost::errinfo_errno(errno));
	}

	if (WIFEXITED(status)) {
		exitcode = WEXITSTATUS(status);
	} else if (WIFSIGNALED(status)) {
		std::ostringstream outputbuf;
		outputbuf << "<Terminated by signal " << WTERMSIG(status) << ".>";
		output = output + outputbuf.str();
		exitcode = 128;
	} else {
		exitcode = 128;
	}

	m_Result.ExecutionEnd = Utility::GetTime();
	m_Result.ExitStatus = exitcode;
	m_Result.Output = output;

	if (m_Callback)
		Utility::QueueAsyncCallback(boost::bind(m_Callback, m_Result));

	return false;
}

#endif /* _WIN32 */
