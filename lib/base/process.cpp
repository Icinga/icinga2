/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/goprocmgr.hpp"
#include "base/process.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include "base/array.hpp"
#include "base/io-engine.hpp"
#include "base/objectlock.hpp"
#include "base/shared.hpp"
#include "base/utility.hpp"
#include "base/initialize.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include "base/scriptglobal.hpp"
#include "base/json.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/asio.hpp>
#include <boost/thread/once.hpp>
#include <cstdint>
#include <memory>
#include <thread>
#include <utility>
#include <iostream>
#include <vector>

#ifndef _WIN32
#	include <execvpe.h>
#	include <string.h>
#	include <sys/types.h>
#	include <unistd.h>

#	ifndef __APPLE__
extern char **environ;
#	else /* __APPLE__ */
#		include <crt_externs.h>
#		define environ (*_NSGetEnviron())
#	endif /* __APPLE__ */
#endif /* _WIN32 */

using namespace icinga;

#ifdef _WIN32

#define IOTHREADS 4

static boost::mutex l_ProcessMutex[IOTHREADS];
static std::map<Process::ProcessHandle, Process::Ptr> l_Processes[IOTHREADS];
static HANDLE l_Events[IOTHREADS];

#else /* _WIN32 */

struct AsioPipe
{
	inline
	AsioPipe(boost::asio::io_context& io) : Read(io), Write(io)
	{
	}

	boost::asio::posix::stream_descriptor Read, Write;
};

class ProcMgr
{
public:
	inline
	ProcMgr() : m_Pid(0), m_In(m_IO), m_Out(m_IO), m_KeepAlive(m_IO), m_WritesQueued(m_IO)
	{
		namespace asio = boost::asio;

		AsioPipe in (m_IO), out (m_IO);

		for (auto pipeFds : {&in, &out}) {
			int fds[2];

			if (pipe(fds)) {
				BOOST_THROW_EXCEPTION(posix_error()
					<< boost::errinfo_api_function("pipe")
					<< boost::errinfo_errno(errno));
			}

			pipeFds->Read.assign(fds[0]);
			pipeFds->Write.assign(fds[1]);
		}

		m_Pid = fork();

		if (m_Pid < 0) {
			BOOST_THROW_EXCEPTION(posix_error()
				<< boost::errinfo_api_function("fork")
				<< boost::errinfo_errno(errno));
		}

		if (m_Pid) {
			std::swap(in.Write, m_In.next_layer());
			std::swap(out.Read, m_Out.next_layer());

			asio::spawn(m_IO, [this](asio::yield_context yc) { ReadLoop(yc); });
			asio::spawn(m_IO, [this](asio::yield_context yc) { WriteLoop(yc); });

			std::thread([this]() {
				for (;;) {
					try {
						m_IO.run();
					} catch (const std::exception& ex) {
						Log(LogCritical, "Process")
							<< "Exception during I/O operation: " << DiagnosticInformation(ex);
					} catch (...) {
						Log(LogCritical, "Process")
							<< "Exception during I/O operation!";
					}
				}
			}).detach();
		} else {
			{
				sigset_t mask;
				sigemptyset(&mask);
				sigprocmask(SIG_SETMASK, &mask, nullptr);
			}

			if (dup2(in.Read.native_handle(), STDIN_FILENO) < 0 || dup2(out.Write.native_handle(), STDOUT_FILENO) < 0) {
				perror("dup2() failed");
				_exit(128);
			}

			rlimit rl;
			if (getrlimit(RLIMIT_NOFILE, &rl) >= 0) {
				rlim_t maxfds = rl.rlim_max;

				if (maxfds == RLIM_INFINITY)
					maxfds = 65536;

				for (rlim_t i = 3; i < maxfds; i++)
					(void)fcntl(i, F_SETFD, FD_CLOEXEC);
			}

#ifdef HAVE_NICE
			{
				// Cheating the compiler on "warning: ignoring return value of 'int nice(int)', declared with attribute warn_unused_result [-Wunused-result]".
				auto x (nice(5));
				(void)x;
			}
#endif /* HAVE_NICE */

			const char *argv[] = {"icinga2-procmgr", nullptr};
			icinga2_execvpe((char*)(void*)argv[0], (char**)(void*)(const char**)argv, environ);

			char errmsg[512];
			strcpy(errmsg, "execvpe(");
			strncat(errmsg, argv[0], sizeof(errmsg) - strlen(errmsg) - 1);
			strncat(errmsg, ") failed", sizeof(errmsg) - strlen(errmsg) - 1);
			errmsg[sizeof(errmsg) - 1] = '\0';
			perror(errmsg);

			_exit(128);
		}
	}

	ProcMgr(const ProcMgr&) = delete;
	ProcMgr(ProcMgr&&) = delete;
	ProcMgr& operator=(const ProcMgr&) = delete;
	ProcMgr& operator=(ProcMgr&&) = delete;

	void Spawn(std::vector<String> args, std::vector<String> extraEnv, double timeout, Process::Callback callback)
	{
		std::unique_ptr<Process::Callback> ucb (new Process::Callback(std::move(callback)));
		auto ssr (Shared<SpawnRequest>::Make(SpawnRequest{(uintptr_t)ucb.get(), std::move(args), std::move(extraEnv), timeout}));

		boost::asio::post(m_IO, [this, ssr]() {
			m_WritesQueue.emplace_back(std::move((SpawnRequest&)*ssr));
			m_WritesQueued.Set();
		});

		ucb.release();
	}

private:
	pid_t m_Pid;
	boost::asio::io_context m_IO;
	boost::asio::buffered_write_stream<boost::asio::posix::stream_descriptor> m_In;
	boost::asio::buffered_read_stream<boost::asio::posix::stream_descriptor> m_Out;
	boost::asio::io_context::work m_KeepAlive;
	std::vector<SpawnRequest> m_WritesQueue;
	AsioConditionVariable m_WritesQueued;

	inline
	void ReadLoop(boost::asio::yield_context& yc)
	{
		for (;;) {
			auto ses (Shared<ExitStatus>::Make());
			ses->ReadFrom(m_Out, yc);

			Utility::QueueAsyncCallback([ses]() {
				std::unique_ptr<Process::Callback> ucb ((Process::Callback*)ses->IPid);
				(*ucb)(ProcessResult{ses->Pid, ses->ExecStart, ses->ExecEnd, ses->ExitCode, std::move(ses->Output)});
			});
		}
	}

	inline
	void WriteLoop(boost::asio::yield_context& yc)
	{
		for (;;) {
			m_WritesQueued.Wait(yc);

			auto items (std::move(m_WritesQueue));
			m_WritesQueue = decltype(m_WritesQueue)();

			m_WritesQueued.Clear();

			for (auto& item : items) {
				item.WriteTo(m_In, yc);
			}

			m_In.async_flush(yc);
		}
	}
};

static std::unique_ptr<ProcMgr> l_ProcMgr;

#endif /* _WIN32 */

static boost::once_flag l_ProcessOnceFlag = BOOST_ONCE_INIT;

Process::Process(Process::Arguments arguments, Dictionary::Ptr extraEnvironment)
	: m_Arguments(std::move(arguments)), m_ExtraEnvironment(std::move(extraEnvironment)), m_Timeout(600)
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

#ifdef _WIN32

static void InitializeProcess()
{
	for (auto& event : l_Events) {
		event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	}
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

void Process::IOThreadProc(int tid)
{
	HANDLE *handles = nullptr;
	HANDLE *fhandles = nullptr;
	int count = 0;
	double now;

	Utility::SetThreadName("ProcessIO");

	for (;;) {
		double timeout = -1;

		now = Utility::GetTime();

		{
			boost::mutex::scoped_lock lock(l_ProcessMutex[tid]);

			count = 1 + l_Processes[tid].size();
			handles = reinterpret_cast<HANDLE *>(realloc(handles, sizeof(HANDLE) * count));
			fhandles = reinterpret_cast<HANDLE *>(realloc(fhandles, sizeof(HANDLE) * count));

			fhandles[0] = l_Events[tid];


			int i = 1;
			typedef std::pair<ProcessHandle, Process::Ptr> kvProcesses[tid]) {
				const Process::Ptr& process = kv.second;
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

		DWORD rc = WaitForMultipleObjects(count, fhandles, FALSE, timeout == -1 ? INFINITE : static_cast<DWORD>(timeout));

		now = Utility::GetTime();

		{
			boost::mutex::scoped_lock lock(l_ProcessMutex[tid]);

			if (rc == WAIT_OBJECT_0)
				ResetEvent(l_Events[tid]);

			for (int i = 1; i < count; i++) {
				auto it = l_Processes[tid].find(handles[i]);

				if (it == l_Processes[tid].end())
					continue; /* This should never happen. */

				bool is_timeout = false;

				if (it->second->m_Timeout != 0) {
					double timeout = it->second->m_Result.ExecutionStart + it->second->m_Timeout;

					if (timeout < now)
						is_timeout = true;
				}

				if (rc == WAIT_OBJECT_0 + i || is_timeout) {
					if (!it->second->DoEvents()) {
						CloseHandle(it->first);
						CloseHandle(it->second->m_FD);
						l_Processes[tid].erase(it);
					}
				}
			}
		}
	}
}

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

void Process::Run(Process::Callback callback)
{
	boost::call_once(l_ProcessOnceFlag, &Process::ThreadInitialize);

	m_Result.ExecutionStart = Utility::GetTime();

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
			Utility::QueueAsyncCallback(std::bind(std::move(callback), m_Result));

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

	m_Callback = callback;

	int tid = GetTID();

	{
		boost::mutex::scoped_lock lock(l_ProcessMutex[tid]);
		l_Processes[tid][m_Process] = this;
	}

	SetEvent(l_Events[tid]);
}

bool Process::DoEvents()
{
	bool is_timeout = false;

	if (m_Timeout != 0) {
		double timeout = m_Result.ExecutionStart + m_Timeout;

		if (timeout < Utility::GetTime()) {
			Log(LogWarning, "Process")
				<< "Killing process group " << m_PID << " (" << PrettyPrintArguments(m_Arguments)
				<< ") after timeout of " << m_Timeout << " seconds";

			m_OutputStream << "<Timeout exceeded.>";
			TerminateProcess(m_Process, 3);

			is_timeout = true;
		}
	}

	if (!is_timeout) {
		m_ReadPending = false;

		DWORD rc;
		if (!m_ReadFailed && GetOverlappedResult(m_FD, &m_Overlapped, &rc, TRUE) && rc > 0) {
			m_OutputStream.wrreturn true;
		}
	}

	String output = m_OutputStream.str();

	WaitForSingleObject(m_Process, INFINITE);

	DWORD exitcode;
	GetExitCodeProcess(m_Process, &exitcode);

	Log(LogNotice, "Process")
		<< "PID " << m_PID << " (" << PrettyPrintArguments(m_Arguments) << ") terminated with exit code " << exitcode;

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

#else /* _WIN32 */

void Process::InitializeSpawnHelper()
{
	if (!l_ProcMgr) {
		l_ProcMgr.reset(new ProcMgr());
	}
}

void Process::Run(Process::Callback callback)
{
	boost::call_once(l_ProcessOnceFlag, &Process::InitializeSpawnHelper);

	{
		std::vector<String> extraEnvironment;

		if (m_ExtraEnvironment) {
			extraEnvironment.reserve(m_ExtraEnvironment->GetLength() + 1u);

			ObjectLock oLock (m_ExtraEnvironment);
			for (auto& kv : m_ExtraEnvironment) {
				extraEnvironment.emplace_back(kv.first + "=" + kv.second);
			}
		}

		extraEnvironment.emplace_back("LC_NUMERIC=C");

		l_ProcMgr->Spawn(m_Arguments, std::move(extraEnvironment), m_Timeout, std::move(callback));
	}

	Log(LogNotice, "Process")
		<< "Running command " << PrettyPrintArguments(m_Arguments);
}

#endif /* _WIN32 */

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

String Process::PrettyPrintArguments(const Process::Arguments& arguments)
{
#ifdef _WIN32
	return "'" + arguments + "'";
#else /* _WIN32 */
	return "'" + boost::algorithm::join(arguments, "' '") + "'";
#endif /* _WIN32 */
}
