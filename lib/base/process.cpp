/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
#include "base/json.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/thread/once.hpp>
#include <thread>
#include <iostream>

#ifndef _WIN32
#	include <execvpe.h>
#	include <poll.h>
#	include <string.h>

#	ifndef __APPLE__
extern char **environ;
#	else /* __APPLE__ */
#		include <crt_externs.h>
#		define environ (*_NSGetEnviron())
#	endif /* __APPLE__ */
#endif /* _WIN32 */

using namespace icinga;

#define IOTHREADS 4

static boost::mutex l_ProcessMutex[IOTHREADS];
static std::map<Process::ProcessHandle, Process::Ptr> l_Processes[IOTHREADS];
#ifdef _WIN32
static HANDLE l_Events[IOTHREADS];
#else /* _WIN32 */
static int l_EventFDs[IOTHREADS][2];
static std::map<Process::ConsoleHandle, Process::ProcessHandle> l_FDs[IOTHREADS];

static boost::mutex l_ProcessControlMutex;
static int l_ProcessControlFD = -1;
static pid_t l_ProcessControlPID;
#endif /* _WIN32 */
static boost::once_flag l_ProcessOnceFlag = BOOST_ONCE_INIT;
static boost::once_flag l_SpawnHelperOnceFlag = BOOST_ONCE_INIT;

Process::Process(Process::Arguments arguments, Dictionary::Ptr extraEnvironment)
	: m_Arguments(std::move(arguments)), m_ExtraEnvironment(std::move(extraEnvironment)), m_Timeout(600), m_AdjustPriority(false)
#ifdef _WIN32
	, m_ReadPending(false), m_ReadFailed(false), m_Overlapped()
#endif /* _WIN32 */
{
#ifdef _WIN32
	m_Overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
#endif /* _WIN32 */
}

Process::~Process()
{
#ifdef _WIN32
	CloseHandle(m_Overlapped.hEvent);
#endif /* _WIN32 */
}

#ifndef _WIN32
static Value ProcessSpawnImpl(struct msghdr *msgh, const Dictionary::Ptr& request)
{
	struct cmsghdr *cmsg = CMSG_FIRSTHDR(msgh);

	if (cmsg == nullptr || cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_len != CMSG_LEN(sizeof(int) * 3)) {
		std::cerr << "Invalid 'spawn' request: FDs missing" << std::endl;
		return Empty;
	}

	auto *fds = (int *)CMSG_DATA(cmsg);

	Array::Ptr arguments = request->Get("arguments");
	Dictionary::Ptr extraEnvironment = request->Get("extraEnvironment");
	bool adjustPriority = request->Get("adjustPriority");

	// build argv
	auto **argv = new char *[arguments->GetLength() + 1];

	for (unsigned int i = 0; i < arguments->GetLength(); i++) {
		String arg = arguments->Get(i);
		argv[i] = strdup(arg.CStr());
	}

	argv[arguments->GetLength()] = nullptr;

	// build envp
	int envc = 0;

	/* count existing environment variables */
	while (environ[envc])
		envc++;

	auto **envp = new char *[envc + (extraEnvironment ? extraEnvironment->GetLength() : 0) + 2];

	for (int i = 0; i < envc; i++)
		envp[i] = strdup(environ[i]);

	if (extraEnvironment) {
		ObjectLock olock(extraEnvironment);

		int index = envc;
		for (const Dictionary::Pair& kv : extraEnvironment) {
			String skv = kv.first + "=" + Convert::ToString(kv.second);
			envp[index] = strdup(skv.CStr());
			index++;
		}
	}

	envp[envc + (extraEnvironment ? extraEnvironment->GetLength() : 0)] = strdup("LC_NUMERIC=C");
	envp[envc + (extraEnvironment ? extraEnvironment->GetLength() : 0) + 1] = nullptr;

	extraEnvironment.reset();

	pid_t pid = fork();

	int errorCode = 0;

	if (pid < 0)
		errorCode = errno;

	if (pid == 0) {
		// child process

		(void)close(l_ProcessControlFD);

		if (setsid() < 0) {
			perror("setsid() failed");
			_exit(128);
		}

		if (dup2(fds[0], STDIN_FILENO) < 0 || dup2(fds[1], STDOUT_FILENO) < 0 || dup2(fds[2], STDERR_FILENO) < 0) {
			perror("dup2() failed");
			_exit(128);
		}

		(void)close(fds[0]);
		(void)close(fds[1]);
		(void)close(fds[2]);

#ifdef HAVE_NICE
		if (adjustPriority) {
			// Cheating the compiler on "warning: ignoring return value of 'int nice(int)', declared with attribute warn_unused_result [-Wunused-result]".
			auto x (nice(5));
			(void)x;
		}
#endif /* HAVE_NICE */

		sigset_t mask;
		sigemptyset(&mask);
		sigprocmask(SIG_SETMASK, &mask, nullptr);

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

	(void)close(fds[0]);
	(void)close(fds[1]);
	(void)close(fds[2]);

	// free arguments
	for (int i = 0; argv[i]; i++)
		free(argv[i]);

	delete[] argv;

	// free environment
	for (int i = 0; envp[i]; i++)
		free(envp[i]);

	delete[] envp;

	Dictionary::Ptr response = new Dictionary({
		{ "rc", pid },
		{ "errno", errorCode }
	});

	return response;
}

static Value ProcessKillImpl(struct msghdr *msgh, const Dictionary::Ptr& request)
{
	pid_t pid = request->Get("pid");
	int signum = request->Get("signum");

	errno = 0;
	kill(pid, signum);
	int error = errno;

	Dictionary::Ptr response = new Dictionary({
		{ "errno", error }
	});

	return response;
}

static Value ProcessWaitPIDImpl(struct msghdr *msgh, const Dictionary::Ptr& request)
{
	pid_t pid = request->Get("pid");

	int status;
	int rc = waitpid(pid, &status, 0);

	Dictionary::Ptr response = new Dictionary({
		{ "status", status },
		{ "rc", rc }
	});

	return response;
}

static void ProcessHandler()
{
	sigset_t mask;
	sigfillset(&mask);
	sigprocmask(SIG_SETMASK, &mask, nullptr);

	rlimit rl;
	if (getrlimit(RLIMIT_NOFILE, &rl) >= 0) {
		rlim_t maxfds = rl.rlim_max;

		if (maxfds == RLIM_INFINITY)
			maxfds = 65536;

		for (rlim_t i = 3; i < maxfds; i++)
			if (i != static_cast<rlim_t>(l_ProcessControlFD))
				(void)close(i);
	}

	for (;;) {
		size_t length;

		struct msghdr msg;
		memset(&msg, 0, sizeof(msg));

		struct iovec io;
		io.iov_base = &length;
		io.iov_len = sizeof(length);

		msg.msg_iov = &io;
		msg.msg_iovlen = 1;

		char cbuf[4096];
		msg.msg_control = cbuf;
		msg.msg_controllen = sizeof(cbuf);

		int rc = recvmsg(l_ProcessControlFD, &msg, 0);

		if (rc <= 0) {
			if (rc < 0 && (errno == EINTR || errno == EAGAIN))
				continue;

			break;
		}

		auto *mbuf = new char[length];

		size_t count = 0;
		while (count < length) {
			rc = recv(l_ProcessControlFD, mbuf + count, length - count, 0);

			if (rc <= 0) {
				if (rc < 0 && (errno == EINTR || errno == EAGAIN))
					continue;

				delete [] mbuf;

				_exit(0);
			}

			count += rc;

			if (rc == 0)
				break;
		}

		String jrequest = String(mbuf, mbuf + count);

		delete [] mbuf;

		Dictionary::Ptr request = JsonDecode(jrequest);

		String command = request->Get("command");

		Value response;

		if (command == "spawn")
			response = ProcessSpawnImpl(&msg, request);
		else if (command == "waitpid")
			response = ProcessWaitPIDImpl(&msg, request);
		else if (command == "kill")
			response = ProcessKillImpl(&msg, request);
		else
			response = Empty;

		String jresponse = JsonEncode(response);

		if (send(l_ProcessControlFD, jresponse.CStr(), jresponse.GetLength(), 0) < 0) {
			BOOST_THROW_EXCEPTION(posix_error()
				<< boost::errinfo_api_function("send")
				<< boost::errinfo_errno(errno));
		}
	}

	_exit(0);
}

static void StartSpawnProcessHelper()
{
	if (l_ProcessControlFD != -1) {
		(void)close(l_ProcessControlFD);

		int status;
		(void)waitpid(l_ProcessControlPID, &status, 0);
	}

	int controlFDs[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, controlFDs) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("socketpair")
			<< boost::errinfo_errno(errno));
	}

	pid_t pid = fork();

	if (pid < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("fork")
			<< boost::errinfo_errno(errno));
	}

	if (pid == 0) {
		(void)close(controlFDs[1]);

		l_ProcessControlFD = controlFDs[0];

		ProcessHandler();

		_exit(1);
	}

	(void)close(controlFDs[0]);

	l_ProcessControlFD = controlFDs[1];
	l_ProcessControlPID = pid;
}

static pid_t ProcessSpawn(const std::vector<String>& arguments, const Dictionary::Ptr& extraEnvironment, bool adjustPriority, int fds[3])
{
	Dictionary::Ptr request = new Dictionary({
		{ "command", "spawn" },
		{ "arguments", Array::FromVector(arguments) },
		{ "extraEnvironment", extraEnvironment },
		{ "adjustPriority", adjustPriority }
	});

	String jrequest = JsonEncode(request);
	size_t length = jrequest.GetLength();

	boost::mutex::scoped_lock lock(l_ProcessControlMutex);

	struct msghdr msg;
	memset(&msg, 0, sizeof(msg));

	struct iovec io;
	io.iov_base = &length;
	io.iov_len = sizeof(length);

	msg.msg_iov = &io;
	msg.msg_iovlen = 1;

	char cbuf[CMSG_SPACE(sizeof(int) * 3)];
	msg.msg_control = cbuf;
	msg.msg_controllen = sizeof(cbuf);

	struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(int) * 3);

	memcpy(CMSG_DATA(cmsg), fds, sizeof(int) * 3);

	msg.msg_controllen = cmsg->cmsg_len;

	do {
		while (sendmsg(l_ProcessControlFD, &msg, 0) < 0) {
			StartSpawnProcessHelper();
		}
	} while (send(l_ProcessControlFD, jrequest.CStr(), jrequest.GetLength(), 0) < 0);

	char buf[4096];

	ssize_t rc = recv(l_ProcessControlFD, buf, sizeof(buf), 0);

	if (rc <= 0)
		return -1;

	String jresponse = String(buf, buf + rc);

	Dictionary::Ptr response = JsonDecode(jresponse);

	if (response->Get("rc") == -1)
		errno = response->Get("errno");

	return response->Get("rc");
}

static int ProcessKill(pid_t pid, int signum)
{
	Dictionary::Ptr request = new Dictionary({
		{ "command", "kill" },
		{ "pid", pid },
		{ "signum", signum }
	});

	String jrequest = JsonEncode(request);
	size_t length = jrequest.GetLength();

	boost::mutex::scoped_lock lock(l_ProcessControlMutex);

	do {
		while (send(l_ProcessControlFD, &length, sizeof(length), 0) < 0) {
			StartSpawnProcessHelper();
		}
	} while (send(l_ProcessControlFD, jrequest.CStr(), jrequest.GetLength(), 0) < 0);

	char buf[4096];

	ssize_t rc = recv(l_ProcessControlFD, buf, sizeof(buf), 0);

	if (rc <= 0)
		return -1;

	String jresponse = String(buf, buf + rc);

	Dictionary::Ptr response = JsonDecode(jresponse);
	return response->Get("errno");
}

static int ProcessWaitPID(pid_t pid, int *status)
{
	Dictionary::Ptr request = new Dictionary({
		{ "command", "waitpid" },
		{ "pid", pid }
	});

	String jrequest = JsonEncode(request);
	size_t length = jrequest.GetLength();

	boost::mutex::scoped_lock lock(l_ProcessControlMutex);

	do {
		while (send(l_ProcessControlFD, &length, sizeof(length), 0) < 0) {
			StartSpawnProcessHelper();
		}
	} while (send(l_ProcessControlFD, jrequest.CStr(), jrequest.GetLength(), 0) < 0);

	char buf[4096];

	ssize_t rc = recv(l_ProcessControlFD, buf, sizeof(buf), 0);

	if (rc <= 0)
		return -1;

	String jresponse = String(buf, buf + rc);

	Dictionary::Ptr response = JsonDecode(jresponse);
	*status = response->Get("status");
	return response->Get("rc");
}

void Process::InitializeSpawnHelper()
{
	if (l_ProcessControlFD == -1)
		StartSpawnProcessHelper();
}
#endif /* _WIN32 */

static void InitializeProcess()
{
#ifdef _WIN32
	for (auto& event : l_Events) {
		event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	}
#else /* _WIN32 */
	for (auto& eventFD : l_EventFDs) {
#	ifdef HAVE_PIPE2
		if (pipe2(eventFD, O_CLOEXEC) < 0) {
			if (errno == ENOSYS) {
#	endif /* HAVE_PIPE2 */
				if (pipe(eventFD) < 0) {
					BOOST_THROW_EXCEPTION(posix_error()
						<< boost::errinfo_api_function("pipe")
						<< boost::errinfo_errno(errno));
				}

				Utility::SetCloExec(eventFD[0]);
				Utility::SetCloExec(eventFD[1]);
#	ifdef HAVE_PIPE2
			} else {
				BOOST_THROW_EXCEPTION(posix_error()
					<< boost::errinfo_api_function("pipe2")
					<< boost::errinfo_errno(errno));
			}
		}
#	endif /* HAVE_PIPE2 */
	}
#endif /* _WIN32 */
}

INITIALIZE_ONCE(InitializeProcess);

void Process::ThreadInitialize()
{
	/* Note to self: Make sure this runs _after_ we've daemonized. */
	for (int tid = 0; tid < IOTHREADS; tid++) {
		std::thread t(std::bind(&Process::IOThreadProc, tid));
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
		for (const Value& argument : arguments) {
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
	return { "sh", "-c", command };
#endif
}

void Process::SetTimeout(double timeout)
{
	m_Timeout = timeout;
}

double Process::GetTimeout() const
{
	return m_Timeout;
}

void Process::SetAdjustPriority(bool adjust)
{
	m_AdjustPriority = adjust;
}

bool Process::GetAdjustPriority() const
{
	return m_AdjustPriority;
}

void Process::IOThreadProc(int tid)
{
#ifdef _WIN32
	HANDLE *handles = nullptr;
	HANDLE *fhandles = nullptr;
#else /* _WIN32 */
	pollfd *pfds = nullptr;
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
			typedef std::pair<ProcessHandle, Process::Ptr> kv_pair;
			for (const kv_pair& kv : l_Processes[tid]) {
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
#ifdef _WIN32
				auto it = l_Processes[tid].find(handles[i]);
#else /* _WIN32 */
				auto it2 = l_FDs[tid].find(pfds[i].fd);

				if (it2 == l_FDs[tid].end())
					continue; /* This should never happen. */

				auto it = l_Processes[tid].find(it2->second);
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
	static LONG pipeIndex = 0;

	if (size == 0)
		size = 8192;

	LONG currentIndex = InterlockedIncrement(&pipeIndex);

	char pipeName[128];
	sprintf(pipeName, "\\\\.\\Pipe\\OverlappedPipe.%d.%d", (int)GetCurrentProcessId(), (int)currentIndex);

	*outReadPipe = CreateNamedPipe(pipeName, PIPE_ACCESS_INBOUND | readMode,
		PIPE_TYPE_BYTE | PIPE_WAIT, 1, size, size, 60 * 1000, securityAttributes);

	if (*outReadPipe == INVALID_HANDLE_VALUE)
		return FALSE;

	*outWritePipe = CreateFile(pipeName, GENERIC_WRITE, 0, securityAttributes, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | writeMode, nullptr);

	if (*outWritePipe == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();
		CloseHandle(*outReadPipe);
		SetLastError(error);
		return FALSE;
	}

	return TRUE;
}
#endif /* _WIN32 */

void Process::Run(const std::function<void(const ProcessResult&)>& callback)
{
#ifndef _WIN32
	boost::call_once(l_SpawnHelperOnceFlag, &Process::InitializeSpawnHelper);
#endif /* _WIN32 */
	boost::call_once(l_ProcessOnceFlag, &Process::ThreadInitialize);

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

	if (!InitializeProcThreadAttributeList(nullptr, 1, 0, &cbSize) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
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
		rgHandles, sizeof(rgHandles), nullptr, nullptr))
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

	char *envp = nullptr;

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

		if (!envp)
			BOOST_THROW_EXCEPTION(std::bad_alloc());

		strcpy(envp + offset, pEnvironment + ioffset);
		offset += len + 1;
		ioffset += len + 1;
	}

	FreeEnvironmentStrings(pEnvironment);

	if (m_ExtraEnvironment) {
		ObjectLock olock(m_ExtraEnvironment);

		for (const Dictionary::Pair& kv : m_ExtraEnvironment) {
			String skv = kv.first + "=" + Convert::ToString(kv.second);

			envp = static_cast<char *>(realloc(envp, offset + skv.GetLength() + 1));

			if (!envp)
				BOOST_THROW_EXCEPTION(std::bad_alloc());

			strcpy(envp + offset, skv.CStr());
			offset += skv.GetLength() + 1;
		}
	}

	envp = static_cast<char *>(realloc(envp, offset + 1));

	if (!envp)
		BOOST_THROW_EXCEPTION(std::bad_alloc());

	envp[offset] = '\0';

	if (!CreateProcess(nullptr, args, nullptr, nullptr, TRUE,
		0 /*EXTENDED_STARTUPINFO_PRESENT*/, envp, nullptr, &si.StartupInfo, &pi)) {
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
			Utility::QueueAsyncCallback(std::bind(callback, m_Result));

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
	int outfds[2];

#ifdef HAVE_PIPE2
	if (pipe2(outfds, O_CLOEXEC) < 0) {
		if (errno == ENOSYS) {
#endif /* HAVE_PIPE2 */
			if (pipe(outfds) < 0) {
				BOOST_THROW_EXCEPTION(posix_error()
					<< boost::errinfo_api_function("pipe")
					<< boost::errinfo_errno(errno));
			}

			Utility::SetCloExec(outfds[0]);
			Utility::SetCloExec(outfds[1]);
#ifdef HAVE_PIPE2
		} else {
			BOOST_THROW_EXCEPTION(posix_error()
				<< boost::errinfo_api_function("pipe2")
				<< boost::errinfo_errno(errno));
		}
	}
#endif /* HAVE_PIPE2 */

	int fds[3];
	fds[0] = STDIN_FILENO;
	fds[1] = outfds[1];
	fds[2] = outfds[1];

	m_Process = ProcessSpawn(m_Arguments, m_ExtraEnvironment, m_AdjustPriority, fds);
	m_PID = m_Process;

	if (m_PID == -1) {
		m_OutputStream << "Fork failed with error code " << errno << " (" << Utility::FormatErrorNumber(errno) << ")";
		Log(LogCritical, "Process", m_OutputStream.str());
	}

	Log(LogNotice, "Process")
		<< "Running command " << PrettyPrintArguments(m_Arguments) << ": PID " << m_PID;

	(void)close(outfds[1]);

	Utility::SetNonBlocking(outfds[0]);

	m_FD = outfds[0];
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

bool Process::DoEvents()
{
	bool is_timeout = false;
#ifndef _WIN32
	bool could_not_kill = false;
#endif /* _WIN32 */

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
			int error = ProcessKill(-m_Process, SIGKILL);
			if (error) {
				Log(LogWarning, "Process")
					<< "Couldn't kill the process group " << m_PID << " (" << PrettyPrintArguments(m_Arguments)
					<< "): [errno " << error << "] " << strerror(error);
				could_not_kill = true;
			}
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
	if (could_not_kill || m_PID == -1) {
		exitcode = 128;
	} else if (ProcessWaitPID(m_Process, &status) != m_Process) {
		exitcode = 128;

		Log(LogWarning, "Process")
			<< "PID " << m_PID << " (" << PrettyPrintArguments(m_Arguments) << ") died mysteriously: waitpid failed";
	} else if (WIFEXITED(status)) {
		exitcode = WEXITSTATUS(status);

		Log(LogNotice, "Process")
			<< "PID " << m_PID << " (" << PrettyPrintArguments(m_Arguments) << ") terminated with exit code " << exitcode;
	} else if (WIFSIGNALED(status)) {
		int signum = WTERMSIG(status);
		const char *zsigname = strsignal(signum);

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
		Utility::QueueAsyncCallback(std::bind(m_Callback, m_Result));

	return false;
}

pid_t Process::GetPID() const
{
	return m_PID;
}


int Process::GetTID() const
{
	return (reinterpret_cast<uintptr_t>(this) / sizeof(void *)) % IOTHREADS;
}

