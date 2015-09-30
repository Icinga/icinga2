/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "base/process.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include "base/array.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/initialize.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include "base/scriptglobal.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/thread/once.hpp>

#ifndef _WIN32
#	include <execvpe.h>
#	include <poll.h>

#	ifndef __APPLE__
extern char **environ;
#	else /* __APPLE__ */
#		include <crt_externs.h>
#		define environ (*_NSGetEnviron())
#	endif /* __APPLE__ */
#endif /* _WIN32 */

using namespace icinga;

#define IOTHREADS 2

static boost::mutex l_ProcessMutex[IOTHREADS];
static std::map<Process::ProcessHandle, Process::Ptr> l_Processes[IOTHREADS];
#ifdef _WIN32
static HANDLE l_Events[IOTHREADS];
#else /* _WIN32 */
static int l_EventFDs[IOTHREADS][2];
static std::map<Process::ConsoleHandle, Process::ProcessHandle> l_FDs[IOTHREADS];
#endif /* _WIN32 */
static boost::once_flag l_OnceFlag = BOOST_ONCE_INIT;

INITIALIZE_ONCE(&Process::StaticInitialize);

Process::Process(const Process::Arguments& arguments, const Dictionary::Ptr& extraEnvironment)
	: m_Arguments(arguments), m_ExtraEnvironment(extraEnvironment), m_Timeout(600)
#ifdef _WIN32
	, m_ReadPending(false), m_ReadFailed(false), m_Overlapped()
#endif /* _WIN32 */
{
#ifdef _WIN32
	m_Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
#endif /* _WIN32 */
}

Process::~Process(void)
{
#ifdef _WIN32
	CloseHandle(m_Overlapped.hEvent);
#endif /* _WIN32 */
}

void Process::StaticInitialize(void)
{
	for (int tid = 0; tid < IOTHREADS; tid++) {
#ifdef _WIN32
		l_Events[tid] = CreateEvent(NULL, TRUE, FALSE, NULL);
#else /* _WIN32 */
#	ifdef HAVE_PIPE2
		if (pipe2(l_EventFDs[tid], O_CLOEXEC) < 0) {
			if (errno == ENOSYS) {
#	endif /* HAVE_PIPE2 */
				if (pipe(l_EventFDs[tid]) < 0) {
					BOOST_THROW_EXCEPTION(posix_error()
					    << boost::errinfo_api_function("pipe")
					    << boost::errinfo_errno(errno));
				}

				Utility::SetCloExec(l_EventFDs[tid][0]);
				Utility::SetCloExec(l_EventFDs[tid][1]);
#	ifdef HAVE_PIPE2
			} else {
				BOOST_THROW_EXCEPTION(posix_error()
					<< boost::errinfo_api_function("pipe2")
					<< boost::errinfo_errno(errno));
			}
		}
#	endif /* HAVE_PIPE2 */
#endif /* _WIN32 */
	}
}

void Process::ThreadInitialize(void)
{
	/* Note to self: Make sure this runs _after_ we've daemonized. */
	for (int tid = 0; tid < IOTHREADS; tid++) {
		boost::thread t(boost::bind(&Process::IOThreadProc, tid));
		t.detach();
	}
}

Process::Arguments Process::PrepareCommand(const Value& command)
{
#ifdef _WIN32
	String args;
#else /* _WIN32 */
	std::vector<String> args;
#endif /* _WIN32 */

	if (command.IsObjectType<Array>()) {
		Array::Ptr arguments = command;

		ObjectLock olock(arguments);
		BOOST_FOREACH(const Value& argument, arguments) {
#ifdef _WIN32
			if (args != "")
				args += " ";

			args += Utility::EscapeCreateProcessArg(argument);
#else /* _WIN32 */
			args.push_back(argument);
#endif /* _WIN32 */
		}

		return args;
	}

#ifdef _WIN32
	return command;
#else /* _WIN32 */
	args.push_back("sh");
	args.push_back("-c");
	args.push_back(command);
	return args;
#endif
}

void Process::SetTimeout(double timeout)
{
	m_Timeout = timeout;
}

double Process::GetTimeout(void) const
{
	return m_Timeout;
}

void Process::IOThreadProc(int tid)
{
#ifdef _WIN32
	HANDLE *handles = NULL;
	HANDLE *fhandles = NULL;
#else /* _WIN32 */
	pollfd *pfds = NULL;
#endif /* _WIN32 */
	int count = 0;
	double now;

	Utility::SetThreadName("ProcessIO");

	for (;;) {
		double timeout = -1;

		now = Utility::GetTime();

		{
			boost::mutex::scoped_lock lock(l_ProcessMutex[tid]);

			count = 1 + l_Processes[tid].size();
#ifdef _WIN32
			handles = reinterpret_cast<HANDLE *>(realloc(handles, sizeof(HANDLE) * count));
			fhandles = reinterpret_cast<HANDLE *>(realloc(fhandles, sizeof(HANDLE) * count));

			fhandles[0] = l_Events[tid];

#else /* _WIN32 */
			pfds = reinterpret_cast<pollfd *>(realloc(pfds, sizeof(pollfd) * count));

			pfds[0].fd = l_EventFDs[tid][0];
			pfds[0].events = POLLIN;
			pfds[0].revents = 0;
#endif /* _WIN32 */

			int i = 1;
			std::pair<ProcessHandle, Process::Ptr> kv;
			BOOST_FOREACH(kv, l_Processes[tid]) {
				const Process::Ptr& process = kv.second;
#ifdef _WIN32
				handles[i] = kv.first;

				if (!process->m_ReadPending) {
					process->m_ReadPending = true;

					BOOL res = ReadFile(process->m_FD, process->m_ReadBuffer, sizeof(process->m_ReadBuffer), 0, &process->m_Overlapped);
					if (res || GetLastError() != ERROR_IO_PENDING) {
						process->m_ReadFailed = !res;
						SetEvent(process->m_Overlapped.hEvent);
					}
				}

				fhandles[i] = process->m_Overlapped.hEvent;
#else /* _WIN32 */
				pfds[i].fd = process->m_FD;
				pfds[i].events = POLLIN;
				pfds[i].revents = 0;
#endif /* _WIN32 */

				if (process->m_Timeout != 0) {
					double delta = process->m_Timeout - (now - process->m_Result.ExecutionStart);

					if (timeout == -1 || delta < timeout)
						timeout = delta;
				}

				i++;
			}
		}

		if (timeout < 0.01)
			timeout = 0.5;

		timeout *= 1000;

#ifdef _WIN32
		DWORD rc = WaitForMultipleObjects(count, fhandles, FALSE, timeout == -1 ? INFINITE : static_cast<DWORD>(timeout));
#else /* _WIN32 */
		int rc = poll(pfds, count, timeout);

		if (rc < 0)
			continue;
#endif /* _WIN32 */

		now = Utility::GetTime();

		{
			boost::mutex::scoped_lock lock(l_ProcessMutex[tid]);

#ifdef _WIN32
			if (rc == WAIT_OBJECT_0)
				ResetEvent(l_Events[tid]);
#else /* _WIN32 */
			if (pfds[0].revents & (POLLIN | POLLHUP | POLLERR)) {
				char buffer[512];
				if (read(l_EventFDs[tid][0], buffer, sizeof(buffer)) < 0)
					Log(LogCritical, "base", "Read from event FD failed.");
			}
#endif /* _WIN32 */

			for (int i = 1; i < count; i++) {
				std::map<ProcessHandle, Process::Ptr>::iterator it;
#ifdef _WIN32
				it = l_Processes[tid].find(handles[i]);
#else /* _WIN32 */
				std::map<ConsoleHandle, ProcessHandle>::iterator it2;
				it2 = l_FDs[tid].find(pfds[i].fd);

				if (it2 == l_FDs[tid].end())
					continue; /* This should never happen. */

				it = l_Processes[tid].find(it2->second);
#endif /* _WIN32 */

				if (it == l_Processes[tid].end())
					continue; /* This should never happen. */

				bool is_timeout = false;

				if (it->second->m_Timeout != 0) {
					double timeout = it->second->m_Result.ExecutionStart + it->second->m_Timeout;

					if (timeout < now)
						is_timeout = true;
				}

#ifdef _WIN32
				if (rc == WAIT_OBJECT_0 + i || is_timeout) {
#else /* _WIN32 */
				if (pfds[i].revents & (POLLIN | POLLHUP | POLLERR) || is_timeout) {
#endif /* _WIN32 */
					if (!it->second->DoEvents()) {
#ifdef _WIN32
						CloseHandle(it->first);
						CloseHandle(it->second->m_FD);
#else /* _WIN32 */
						l_FDs[tid].erase(it->second->m_FD);
						(void)close(it->second->m_FD);
#endif /* _WIN32 */
						l_Processes[tid].erase(it);
					}
				}
			}
		}
	}
}

String Process::PrettyPrintArguments(const Process::Arguments& arguments)
{
#ifdef _WIN32
	return "'" + arguments + "'";
#else /* _WIN32 */
	return "'" + boost::algorithm::join(arguments, "' '") + "'";
#endif /* _WIN32 */
}

#ifdef _WIN32
static BOOL CreatePipeOverlapped(HANDLE *outReadPipe, HANDLE *outWritePipe,
    SECURITY_ATTRIBUTES *securityAttributes, DWORD size, DWORD readMode, DWORD writeMode)
{
	static int pipeIndex = 0;

	if (size == 0)
		size = 8192;

	pipeIndex++;

	char pipeName[128];
	sprintf(pipeName, "\\\\.\\Pipe\\OverlappedPipe.%d.%d", (int)GetCurrentProcessId(), pipeIndex);

	*outReadPipe = CreateNamedPipe(pipeName, PIPE_ACCESS_INBOUND | readMode,
	    PIPE_TYPE_BYTE | PIPE_WAIT, 1, size, size, 60 * 1000, securityAttributes);
	
	if (!*outReadPipe)
		return FALSE;

	*outWritePipe = CreateFile(pipeName, GENERIC_WRITE, 0, securityAttributes, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | writeMode, NULL);

	if (*outWritePipe == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();
		CloseHandle(*outReadPipe);
		SetLastError(error);
		return FALSE;
	}

	return TRUE;
}
#endif /* _WIN32 */

void Process::Run(const boost::function<void(const ProcessResult&)>& callback)
{
	boost::call_once(l_OnceFlag, &Process::ThreadInitialize);

	m_Result.ExecutionStart = Utility::GetTime();

#ifdef _WIN32
	SECURITY_ATTRIBUTES sa = {};
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	HANDLE outReadPipe, outWritePipe;
	if (!CreatePipeOverlapped(&outReadPipe, &outWritePipe, &sa, 0, FILE_FLAG_OVERLAPPED, 0))
		BOOST_THROW_EXCEPTION(win32_error()
			<< boost::errinfo_api_function("CreatePipe")
			<< errinfo_win32_error(GetLastError()));

	if (!SetHandleInformation(outReadPipe, HANDLE_FLAG_INHERIT, 0))
		BOOST_THROW_EXCEPTION(win32_error()
			<< boost::errinfo_api_function("SetHandleInformation")
			<< errinfo_win32_error(GetLastError()));

	HANDLE outWritePipeDup;
	if (!DuplicateHandle(GetCurrentProcess(), outWritePipe, GetCurrentProcess(),
	    &outWritePipeDup, 0, TRUE, DUPLICATE_SAME_ACCESS))
		BOOST_THROW_EXCEPTION(win32_error()
			<< boost::errinfo_api_function("DuplicateHandle")
			<< errinfo_win32_error(GetLastError()));

/*	LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList;
	SIZE_T cbSize;

	if (!InitializeProcThreadAttributeList(NULL, 1, 0, &cbSize) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		BOOST_THROW_EXCEPTION(win32_error()
		<< boost::errinfo_api_function("InitializeProcThreadAttributeList")
		<< errinfo_win32_error(GetLastError()));

	lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(new char[cbSize]);

	if (!InitializeProcThreadAttributeList(lpAttributeList, 1, 0, &cbSize))
		BOOST_THROW_EXCEPTION(win32_error()
		<< boost::errinfo_api_function("InitializeProcThreadAttributeList")
		<< errinfo_win32_error(GetLastError()));

	HANDLE rgHandles[3];
	rgHandles[0] = outWritePipe;
	rgHandles[1] = outWritePipeDup;
	rgHandles[2] = GetStdHandle(STD_INPUT_HANDLE);

	if (!UpdateProcThreadAttribute(lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
	    rgHandles, sizeof(rgHandles), NULL, NULL))
		BOOST_THROW_EXCEPTION(win32_error()
			<< boost::errinfo_api_function("UpdateProcThreadAttribute")
			<< errinfo_win32_error(GetLastError()));
*/

	STARTUPINFOEX si = {};
	si.StartupInfo.cb = sizeof(si);
	si.StartupInfo.hStdError = outWritePipe;
	si.StartupInfo.hStdOutput = outWritePipeDup;
	si.StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
//	si.lpAttributeList = lpAttributeList;

	PROCESS_INFORMATION pi;

	char *args = new char[m_Arguments.GetLength() + 1];
	strncpy(args, m_Arguments.CStr(), m_Arguments.GetLength() + 1);
	args[m_Arguments.GetLength()] = '\0';

	LPCH pEnvironment = GetEnvironmentStrings();
	size_t ioffset = 0, offset = 0;

	char *envp = NULL;

	for (;;) {
		size_t len = strlen(pEnvironment + ioffset);

		if (len == 0)
			break;

		char *eqp = strchr(pEnvironment + ioffset, '=');
		if (eqp && m_ExtraEnvironment && m_ExtraEnvironment->Contains(String(pEnvironment + ioffset, eqp))) {
			ioffset += len + 1;
			continue;
		}

		envp = static_cast<char *>(realloc(envp, offset + len + 1));

		if (envp == NULL)
			BOOST_THROW_EXCEPTION(std::bad_alloc());

		strcpy(envp + offset, pEnvironment + ioffset);
		offset += len + 1;
		ioffset += len + 1;
	}

	FreeEnvironmentStrings(pEnvironment);

	if (m_ExtraEnvironment) {
		ObjectLock olock(m_ExtraEnvironment);

		BOOST_FOREACH(const Dictionary::Pair& kv, m_ExtraEnvironment) {
			String skv = kv.first + "=" + Convert::ToString(kv.second);

			envp = static_cast<char *>(realloc(envp, offset + skv.GetLength() + 1));

			if (envp == NULL)
				BOOST_THROW_EXCEPTION(std::bad_alloc());

			strcpy(envp + offset, skv.CStr());
			offset += skv.GetLength() + 1;
		}
	}

	envp = static_cast<char *>(realloc(envp, offset + 1));

	if (envp == NULL)
		BOOST_THROW_EXCEPTION(std::bad_alloc());

	envp[offset] = '\0';

	if (!CreateProcess(NULL, args, NULL, NULL, TRUE,
	    0 /*EXTENDED_STARTUPINFO_PRESENT*/, envp, NULL, &si.StartupInfo, &pi)) {
		DWORD error = GetLastError();
		CloseHandle(outWritePipe);
		CloseHandle(outWritePipeDup);
		free(envp);
/*		DeleteProcThreadAttributeList(lpAttributeList);
		delete [] reinterpret_cast<char *>(lpAttributeList); */

		m_Result.PID = 0;
		m_Result.ExecutionEnd = Utility::GetTime();
		m_Result.ExitStatus = 127;
		m_Result.Output = "Command " + String(args) + " failed to execute: " + Utility::FormatErrorNumber(error);

		delete [] args;

		if (callback)
			Utility::QueueAsyncCallback(boost::bind(callback, m_Result));

		return;
	}

	delete [] args;
	free(envp);
/*	DeleteProcThreadAttributeList(lpAttributeList);
	delete [] reinterpret_cast<char *>(lpAttributeList); */

	CloseHandle(outWritePipe);
	CloseHandle(outWritePipeDup);
	CloseHandle(pi.hThread);

	m_Process = pi.hProcess;
	m_FD = outReadPipe;
	m_PID = pi.dwProcessId;

	Log(LogNotice, "Process")
	    << "Running command " << PrettyPrintArguments(m_Arguments) << ": PID " << m_PID;

#else /* _WIN32 */
	int fds[2];

#ifdef HAVE_PIPE2
	if (pipe2(fds, O_CLOEXEC) < 0) {
		if (errno == ENOSYS) {
#endif /* HAVE_PIPE2 */
			if (pipe(fds) < 0) {
				BOOST_THROW_EXCEPTION(posix_error()
				    << boost::errinfo_api_function("pipe")
				    << boost::errinfo_errno(errno));
			}

			Utility::SetCloExec(fds[0]);
			Utility::SetCloExec(fds[1]);
#ifdef HAVE_PIPE2
		} else {
			BOOST_THROW_EXCEPTION(posix_error()
				<< boost::errinfo_api_function("pipe2")
				<< boost::errinfo_errno(errno));
		}
	}
#endif /* HAVE_PIPE2 */

	// build argv
	char **argv = new char *[m_Arguments.size() + 1];

	for (unsigned int i = 0; i < m_Arguments.size(); i++)
		argv[i] = strdup(m_Arguments[i].CStr());

	argv[m_Arguments.size()] = NULL;

	// build envp
	int envc = 0;

	/* count existing environment variables */
	while (environ[envc] != NULL)
		envc++;

	char **envp = new char *[envc + (m_ExtraEnvironment ? m_ExtraEnvironment->GetLength() : 0) + 2];

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

	envp[envc + (m_ExtraEnvironment ? m_ExtraEnvironment->GetLength() : 0)] = strdup("LC_NUMERIC=C");
	envp[envc + (m_ExtraEnvironment ? m_ExtraEnvironment->GetLength() : 0) + 1] = NULL;

	m_ExtraEnvironment.reset();

#ifdef HAVE_VFORK
	Value use_vfork = ScriptGlobal::Get("UseVfork");

	if (use_vfork.IsEmpty() || static_cast<bool>(use_vfork))
		m_Process = vfork();
	else
		m_Process = fork();
#else /* HAVE_VFORK */
	m_Process = fork();
#endif /* HAVE_VFORK */

	if (m_Process < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("fork")
			<< boost::errinfo_errno(errno));
	}

	if (m_Process == 0) {
		// child process

		if (setsid() < 0) {
			perror("setsid() failed");
			_exit(128);
		}

		if (dup2(fds[1], STDOUT_FILENO) < 0 || dup2(fds[1], STDERR_FILENO) < 0) {
			perror("dup2() failed");
			_exit(128);
		}

		(void)close(fds[0]);
		(void)close(fds[1]);

#ifdef HAVE_NICE
		if (nice(5) < 0)
			Log(LogWarning, "base", "Failed to renice child process.");
#endif /* HAVE_NICE */

		if (icinga2_execvpe(argv[0], argv, envp) < 0) {
			char errmsg[512];
			strcpy(errmsg, "execvpe(");
			strncat(errmsg, argv[0], sizeof(errmsg) - strlen(errmsg) - 1);
			strncat(errmsg, ") failed", sizeof(errmsg) - strlen(errmsg) - 1);
			errmsg[sizeof(errmsg) - 1] = '\0';
			perror(errmsg);
			_exit(128);
		}

		_exit(128);
	}

	// parent process

	m_PID = m_Process;

	Log(LogNotice, "Process")
	    << "Running command " << PrettyPrintArguments(m_Arguments) <<": PID " << m_PID;

	// free arguments
	for (int i = 0; argv[i] != NULL; i++)
		free(argv[i]);

	delete[] argv;

	// free environment
	for (int i = 0; envp[i] != NULL; i++)
		free(envp[i]);

	delete[] envp;

	(void)close(fds[1]);

	Utility::SetNonBlocking(fds[0]);

	m_FD = fds[0];
#endif /* _WIN32 */

	m_Callback = callback;

	int tid = GetTID();

	{
		boost::mutex::scoped_lock lock(l_ProcessMutex[tid]);
		l_Processes[tid][m_Process] = this;
#ifndef _WIN32
		l_FDs[tid][m_FD] = m_Process;
#endif /* _WIN32 */
	}

#ifdef _WIN32
	SetEvent(l_Events[tid]);
#else /* _WIN32 */
	if (write(l_EventFDs[tid][1], "T", 1) < 0 && errno != EINTR && errno != EAGAIN)
		Log(LogCritical, "base", "Write to event FD failed.");
#endif /* _WIN32 */
}

bool Process::DoEvents(void)
{
	bool is_timeout = false;

	if (m_Timeout != 0) {
		double timeout = m_Result.ExecutionStart + m_Timeout;

		if (timeout < Utility::GetTime()) {
			Log(LogWarning, "Process")
			    << "Killing process group " << m_PID << " (" << PrettyPrintArguments(m_Arguments)
			    << ") after timeout of " << m_Timeout << " seconds";

			m_OutputStream << "<Timeout exceeded.>";
#ifdef _WIN32
			TerminateProcess(m_Process, 1);
#else /* _WIN32 */
			kill(-m_Process, SIGKILL);
#endif /* _WIN32 */

			is_timeout = true;
		}
	}

	if (!is_timeout) {
#ifdef _WIN32
		m_ReadPending = false;

		DWORD rc;
		if (!m_ReadFailed && GetOverlappedResult(m_FD, &m_Overlapped, &rc, TRUE) && rc > 0) {
			m_OutputStream.write(m_ReadBuffer, rc);
			return true;
		}
#else /* _WIN32 */
		char buffer[512];
		for (;;) {
			int rc = read(m_FD, buffer, sizeof(buffer));

			if (rc < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
				return true;

			if (rc > 0) {
				m_OutputStream.write(buffer, rc);
				continue;
			}

			break;
		}
#endif /* _WIN32 */
	}

	String output = m_OutputStream.str();

#ifdef _WIN32
	WaitForSingleObject(m_Process, INFINITE);

	DWORD exitcode;
	GetExitCodeProcess(m_Process, &exitcode);

	Log(LogNotice, "Process")
	    << "PID " << m_PID << " (" << PrettyPrintArguments(m_Arguments) << ") terminated with exit code " << exitcode;
#else /* _WIN32 */
	int status, exitcode;
	if (waitpid(m_Process, &status, 0) != m_Process) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("waitpid")
			<< boost::errinfo_errno(errno));
	}

	if (WIFEXITED(status)) {
		exitcode = WEXITSTATUS(status);

		Log(LogNotice, "Process")
		    << "PID " << m_PID << " (" << PrettyPrintArguments(m_Arguments) << ") terminated with exit code " << exitcode;
	} else if (WIFSIGNALED(status)) {
		int signum = WTERMSIG(status);
		char *zsigname = strsignal(signum);

		String signame = Convert::ToString(signum);

		if (zsigname) {
			signame += " (";
			signame += zsigname;
			signame += ")";
		}

		Log(LogWarning, "Process")
		    << "PID " << m_PID << " was terminated by signal " << signame;

		std::ostringstream outputbuf;
		outputbuf << "<Terminated by signal " << signame << ".>";
		output = output + outputbuf.str();
		exitcode = 128;
	} else {
		exitcode = 128;
	}
#endif /* _WIN32 */

	m_Result.PID = m_PID;
	m_Result.ExecutionEnd = Utility::GetTime();
	m_Result.ExitStatus = exitcode;
	m_Result.Output = output;

	if (m_Callback)
		Utility::QueueAsyncCallback(boost::bind(m_Callback, m_Result));

	return false;
}

pid_t Process::GetPID(void) const
{
	return m_PID;
}


int Process::GetTID(void) const
{
	return (reinterpret_cast<uintptr_t>(this) / sizeof(void *)) % IOTHREADS;
}

