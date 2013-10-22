/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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
#include "base/utility.h"
#include <boost/bind.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp>

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

ProcessResult Process::Run(void)
{
	ProcessResult result;

	result.ExecutionStart = Utility::GetTime();

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

	Utility::SetCloExec(fds[0]);
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

	int fd = fds[0];
	(void) close(fds[1]);

		char buffer[512];

	std::ostringstream outputStream;

	pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	for (;;) {
		int rc1, timeout;

		timeout = 0;

		if (m_Timeout != 0) {
			timeout = m_Timeout - (Utility::GetTime() - result.ExecutionStart);

			if (timeout < 0) {
				outputStream << "<Timeout exceeded.>";
				kill(m_Pid, SIGKILL);
				break;
			}
		}

		rc1 = poll(&pfd, 1, timeout * 1000);

		if (rc1 > 0) {
			int rc2 = read(fd, buffer, sizeof(buffer));

			if (rc2 <= 0)
				break;

			outputStream.write(buffer, rc2);
		}
	}

	String output = outputStream.str();

	int status, exitcode;

	(void) close(fd);

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

	result.ExecutionEnd = Utility::GetTime();
	result.ExitStatus = exitcode;
	result.Output = output;

	return result;
}

#endif /* _WIN32 */
