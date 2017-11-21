/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/application.hpp"
#include "base/application.tcpp"
#include "base/stacktrace.hpp"
#include "base/timer.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/loader.hpp"
#include "base/debug.hpp"
#include "base/type.hpp"
#include "base/convert.hpp"
#include "base/scriptglobal.hpp"
#include "base/process.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <sstream>
#include <iostream>
#include <fstream>
#include <thread>
#ifdef __linux__
#include <sys/prctl.h>
#endif /* __linux__ */

#ifdef _WIN32
#include <windows.h>
#endif /* _win32 */

using namespace icinga;

REGISTER_TYPE(Application);

boost::signals2::signal<void (void)> Application::OnReopenLogs;
Application::Ptr Application::m_Instance = NULL;
bool Application::m_ShuttingDown = false;
bool Application::m_RequestRestart = false;
bool Application::m_RequestReopenLogs = false;
pid_t Application::m_ReloadProcess = 0;
static bool l_Restarting = false;
static bool l_InExceptionHandler = false;
int Application::m_ArgC;
char **Application::m_ArgV;
double Application::m_StartTime;
double Application::m_MainTime;
bool Application::m_ScriptDebuggerEnabled = false;
double Application::m_LastReloadFailed;

/**
 * Constructor for the Application class.
 */
void Application::OnConfigLoaded(void)
{
	m_PidFile = NULL;

	ASSERT(m_Instance == NULL);
	m_Instance = this;
}

/**
 * Destructor for the application class.
 */
void Application::Stop(bool runtimeRemoved)
{
	m_ShuttingDown = true;

#ifdef _WIN32
	WSACleanup();
#endif /* _WIN32 */

	// Getting a shutdown-signal when a restart is in progress usually
	// means that the restart succeeded and the new process wants to take
	// over. Write the PID of the new process to the pidfile before this
	// process exits to keep systemd happy.
	if (l_Restarting) {
		try {
			UpdatePidFile(GetPidPath(), m_ReloadProcess);
		} catch (const std::exception&) {
			/* abort restart */
			Log(LogCritical, "Application", "Cannot update PID file. Aborting restart operation.");
			return;
		}

		Log(LogDebug, "Application")
			<< "Keeping pid  '" << m_ReloadProcess << "' open.";

		ClosePidFile(false);
	} else
		ClosePidFile(true);

	ObjectImpl<Application>::Stop(runtimeRemoved);
}

Application::~Application(void)
{
	m_Instance = NULL;
}

void Application::Exit(int rc)
{
	std::cout.flush();
	std::cerr.flush();

	for (const Logger::Ptr& logger : Logger::GetLoggers()) {
		logger->Flush();
	}

	UninitializeBase();
#ifdef I2_DEBUG
	exit(rc);
#else /* I2_DEBUG */
	_exit(rc); // Yay, our static destructors are pretty much beyond repair at this point.
#endif /* I2_DEBUG */
}

void Application::InitializeBase(void)
{
#ifdef _WIN32
	/* disable GUI-based error messages for LoadLibrary() */
	SetErrorMode(SEM_FAILCRITICALERRORS);

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) {
		BOOST_THROW_EXCEPTION(win32_error()
			<< boost::errinfo_api_function("WSAStartup")
			<< errinfo_win32_error(WSAGetLastError()));
	}
#else /* _WIN32 */
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL);
#endif /* _WIN32 */

	Loader::ExecuteDeferredInitializers();

	/* make sure the thread pool gets initialized */
	GetTP().Start();

	Timer::Initialize();
}

void Application::UninitializeBase(void)
{
	Timer::Uninitialize();

	GetTP().Stop();
}

/**
 * Retrieves a pointer to the application singleton object.
 *
 * @returns The application object.
 */
Application::Ptr Application::GetInstance(void)
{
	return m_Instance;
}

void Application::SetResourceLimits(void)
{
#ifdef __linux__
	rlimit rl;

#	ifdef RLIMIT_NOFILE
	rlim_t fileLimit = GetRLimitFiles();

	if (fileLimit != 0) {
		if (fileLimit < GetDefaultRLimitFiles()) {
			Log(LogWarning, "Application")
			    << "The user-specified value for RLimitFiles cannot be smaller than the default value (" << GetDefaultRLimitFiles() << "). Using the default value instead.";
			fileLimit = GetDefaultRLimitFiles();
		}

		rl.rlim_cur = fileLimit;
		rl.rlim_max = rl.rlim_cur;

		if (setrlimit(RLIMIT_NOFILE, &rl) < 0)
			Log(LogNotice, "Application", "Could not adjust resource limit for open file handles (RLIMIT_NOFILE)");
#	else /* RLIMIT_NOFILE */
		Log(LogNotice, "Application", "System does not support adjusting the resource limit for open file handles (RLIMIT_NOFILE)");
#	endif /* RLIMIT_NOFILE */
	}

#	ifdef RLIMIT_NPROC
	rlim_t processLimit = GetRLimitProcesses();

	if (processLimit != 0) {
		if (processLimit < GetDefaultRLimitProcesses()) {
			Log(LogWarning, "Application")
			    << "The user-specified value for RLimitProcesses cannot be smaller than the default value (" << GetDefaultRLimitProcesses() << "). Using the default value instead.";
			processLimit = GetDefaultRLimitProcesses();
		}

		rl.rlim_cur = processLimit;
		rl.rlim_max = rl.rlim_cur;

		if (setrlimit(RLIMIT_NPROC, &rl) < 0)
			Log(LogNotice, "Application", "Could not adjust resource limit for number of processes (RLIMIT_NPROC)");
#	else /* RLIMIT_NPROC */
		Log(LogNotice, "Application", "System does not support adjusting the resource limit for number of processes (RLIMIT_NPROC)");
#	endif /* RLIMIT_NPROC */
	}

#	ifdef RLIMIT_STACK
	int argc = Application::GetArgC();
	char **argv = Application::GetArgV();
	bool set_stack_rlimit = true;

	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "--no-stack-rlimit") == 0) {
			set_stack_rlimit = false;
			break;
		}
	}

	if (getrlimit(RLIMIT_STACK, &rl) < 0) {
		Log(LogWarning, "Application", "Could not determine resource limit for stack size (RLIMIT_STACK)");
		rl.rlim_max = RLIM_INFINITY;
	}

	rlim_t stackLimit;

	stackLimit = GetRLimitStack();

	if (stackLimit != 0) {
		if (stackLimit < GetDefaultRLimitStack()) {
			Log(LogWarning, "Application")
			    << "The user-specified value for RLimitStack cannot be smaller than the default value (" << GetDefaultRLimitStack() << "). Using the default value instead.";
			stackLimit = GetDefaultRLimitStack();
		}

		if (set_stack_rlimit)
			rl.rlim_cur = stackLimit;
		else
			rl.rlim_cur = rl.rlim_max;

		if (setrlimit(RLIMIT_STACK, &rl) < 0)
			Log(LogNotice, "Application", "Could not adjust resource limit for stack size (RLIMIT_STACK)");
		else if (set_stack_rlimit) {
			char **new_argv = static_cast<char **>(malloc(sizeof(char *) * (argc + 2)));

			if (!new_argv) {
				perror("malloc");
				Exit(EXIT_FAILURE);
			}

			new_argv[0] = argv[0];
			new_argv[1] = strdup("--no-stack-rlimit");

			if (!new_argv[1]) {
				perror("strdup");
				exit(1);
			}

			for (int i = 1; i < argc; i++)
				new_argv[i + 1] = argv[i];

			new_argv[argc + 1] = NULL;

			(void) execvp(new_argv[0], new_argv);
			perror("execvp");
			_exit(EXIT_FAILURE);
		}
#	else /* RLIMIT_STACK */
		Log(LogNotice, "Application", "System does not support adjusting the resource limit for stack size (RLIMIT_STACK)");
#	endif /* RLIMIT_STACK */
	}
#endif /* __linux__ */
}

int Application::GetArgC(void)
{
	return m_ArgC;
}

void Application::SetArgC(int argc)
{
	m_ArgC = argc;
}

char **Application::GetArgV(void)
{
	return m_ArgV;
}

void Application::SetArgV(char **argv)
{
	m_ArgV = argv;
}

/**
 * Processes events for registered sockets and timers and calls whatever
 * handlers have been set up for these events.
 */
void Application::RunEventLoop(void)
{
	Timer::Initialize();

	double lastLoop = Utility::GetTime();

mainloop:
	while (!m_ShuttingDown && !m_RequestRestart) {
		/* Watches for changes to the system time. Adjusts timers if necessary. */
		Utility::Sleep(2.5);

		if (m_RequestReopenLogs) {
			Log(LogNotice, "Application", "Reopening log files");
			m_RequestReopenLogs = false;
			OnReopenLogs();
		}

		double now = Utility::GetTime();
		double timeDiff = lastLoop - now;

		if (std::fabs(timeDiff) > 15) {
			/* We made a significant jump in time. */
			Log(LogInformation, "Application")
			    << "We jumped "
			    << (timeDiff < 0 ? "forward" : "backward")
			    << " in time: " << std::fabs(timeDiff) << " seconds";

			Timer::AdjustTimers(-timeDiff);
		}

		lastLoop = now;
	}

	if (m_RequestRestart) {
		m_RequestRestart = false;         // we are now handling the request, once is enough

		// are we already restarting? ignore request if we already are
		if (l_Restarting)
			goto mainloop;

		l_Restarting = true;
		m_ReloadProcess = StartReloadProcess();

		goto mainloop;
	}

	Log(LogInformation, "Application", "Shutting down...");

	ConfigObject::StopObjects();
	Application::GetInstance()->OnShutdown();

	UninitializeBase();
}

bool Application::IsShuttingDown(void)
{
	return m_ShuttingDown;
}

void Application::OnShutdown(void)
{
	/* Nothing to do here. */
}

static void ReloadProcessCallbackInternal(const ProcessResult& pr)
{
	if (pr.ExitStatus != 0) {
		Application::SetLastReloadFailed(Utility::GetTime());
		Log(LogCritical, "Application", "Found error in config: reloading aborted");
	}
#ifdef _WIN32
	else
		Application::Exit(7); /* keep this exit code in sync with icinga-app */
#endif /* _WIN32 */
}

static void ReloadProcessCallback(const ProcessResult& pr)
{
	l_Restarting = false;

	std::thread t(std::bind(&ReloadProcessCallbackInternal, pr));
	t.detach();
}

pid_t Application::StartReloadProcess(void)
{
	Log(LogInformation, "Application", "Got reload command: Starting new instance.");

	// prepare arguments
	Array::Ptr args = new Array();
	args->Add(GetExePath(m_ArgV[0]));

	for (int i=1; i < Application::GetArgC(); i++) {
		if (std::string(Application::GetArgV()[i]) != "--reload-internal")
			args->Add(Application::GetArgV()[i]);
		else
			i++;     // the next parameter after --reload-internal is the pid, remove that too
	}

#ifndef _WIN32
	args->Add("--reload-internal");
	args->Add(Convert::ToString(Utility::GetPid()));
#else /* _WIN32 */
	args->Add("--validate");
#endif /* _WIN32 */

	Process::Ptr process = new Process(Process::PrepareCommand(args));
	process->SetTimeout(300);
	process->Run(&ReloadProcessCallback);

	return process->GetPID();
}

/**
 * Signals the application to shut down during the next
 * execution of the event loop.
 */
void Application::RequestShutdown(void)
{
	Log(LogInformation, "Application", "Received request to shut down.");

	m_ShuttingDown = true;
}

/**
 * Signals the application to restart during the next
 * execution of the event loop.
 */
void Application::RequestRestart(void)
{
	m_RequestRestart = true;
}

/**
 * Signals the application to reopen log files during the
 * next execution of the event loop.
 */
void Application::RequestReopenLogs(void)
{
	m_RequestReopenLogs = true;
}

/**
 * Retrieves the full path of the executable.
 *
 * @param argv0 The first command-line argument.
 * @returns The path.
 */
String Application::GetExePath(const String& argv0)
{
	String executablePath;

#ifndef _WIN32
	char buffer[MAXPATHLEN];
	if (getcwd(buffer, sizeof(buffer)) == NULL) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("getcwd")
		    << boost::errinfo_errno(errno));
	}

	String workingDirectory = buffer;

	if (argv0[0] != '/')
		executablePath = workingDirectory + "/" + argv0;
	else
		executablePath = argv0;

	bool foundSlash = false;
	for (size_t i = 0; i < argv0.GetLength(); i++) {
		if (argv0[i] == '/') {
			foundSlash = true;
			break;
		}
	}

	if (!foundSlash) {
		const char *pathEnv = getenv("PATH");
		if (pathEnv != NULL) {
			std::vector<String> paths;
			boost::algorithm::split(paths, pathEnv, boost::is_any_of(":"));

			bool foundPath = false;
			for (const String& path : paths) {
				String pathTest = path + "/" + argv0;

				if (access(pathTest.CStr(), X_OK) == 0) {
					executablePath = pathTest;
					foundPath = true;
					break;
				}
			}

			if (!foundPath) {
				executablePath.Clear();
				BOOST_THROW_EXCEPTION(std::runtime_error("Could not determine executable path."));
			}
		}
	}

	if (realpath(executablePath.CStr(), buffer) == NULL) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("realpath")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(executablePath));
	}

	return buffer;
#else /* _WIN32 */
	char FullExePath[MAXPATHLEN];

	if (!GetModuleFileName(NULL, FullExePath, sizeof(FullExePath)))
		BOOST_THROW_EXCEPTION(win32_error()
		    << boost::errinfo_api_function("GetModuleFileName")
		    << errinfo_win32_error(GetLastError()));

	return FullExePath;
#endif /* _WIN32 */
}

/**
 * Display version and path information.
 */
void Application::DisplayInfoMessage(std::ostream& os, bool skipVersion)
{
	os << "Application information:" << "\n";

	if (!skipVersion)
		os << "  Application version: " << GetAppVersion() << "\n";

	os << "  Installation root: " << GetPrefixDir() << "\n"
	   << "  Sysconf directory: " << GetSysconfDir() << "\n"
	   << "  Run directory: " << GetRunDir() << "\n"
	   << "  Local state directory: " << GetLocalStateDir() << "\n"
	   << "  Package data directory: " << GetPkgDataDir() << "\n"
	   << "  State path: " << GetStatePath() << "\n"
	   << "  Modified attributes path: " << GetModAttrPath() << "\n"
	   << "  Objects path: " << GetObjectsPath() << "\n"
	   << "  Vars path: " << GetVarsPath() << "\n"
	   << "  PID path: " << GetPidPath() << "\n";

	os << "\n"
	   << "System information:" << "\n"
	   << "  Platform: " << Utility::GetPlatformName() << "\n"
	   << "  Platform version: " << Utility::GetPlatformVersion() << "\n"
	   << "  Kernel: " << Utility::GetPlatformKernel() << "\n"
	   << "  Kernel version: " << Utility::GetPlatformKernelVersion() << "\n"
	   << "  Architecture: " << Utility::GetPlatformArchitecture() << "\n";

	os << "\n"
	   << "Build information:" << "\n"
	   << "  Compiler: " << ScriptGlobal::Get("BuildCompilerName") << " " << ScriptGlobal::Get("BuildCompilerVersion") << "\n"
	   << "  Build host: " << ScriptGlobal::Get("BuildHostName") << "\n";
}

/**
 * Displays a message that tells users what to do when they encounter a bug.
 */
void Application::DisplayBugMessage(std::ostream& os)
{
	os << "***" << "\n"
	   << "* This would indicate a runtime problem or configuration error. If you believe this is a bug in Icinga 2" << "\n"
	   << "* please submit a bug report at https://github.com/Icinga/icinga2 and include this stack trace as well as any other" << "\n"
	   << "* information that might be useful in order to reproduce this problem." << "\n"
	   << "***" << "\n";
}

String Application::GetCrashReportFilename(void)
{
	return GetLocalStateDir() + "/log/icinga2/crash/report." + Convert::ToString(Utility::GetTime());
}


void Application::AttachDebugger(const String& filename, bool interactive)
{
#ifndef _WIN32
#ifdef __linux__
	prctl(PR_SET_DUMPABLE, 1);
#endif /* __linux __ */

	String my_pid = Convert::ToString(Utility::GetPid());

	pid_t pid = fork();

	if (pid < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("fork")
		    << boost::errinfo_errno(errno));
	}

	if (pid == 0) {
		if (!interactive) {
			int fd = open(filename.CStr(), O_CREAT | O_RDWR | O_APPEND, 0600);

			if (fd < 0) {
				BOOST_THROW_EXCEPTION(posix_error()
				    << boost::errinfo_api_function("open")
				    << boost::errinfo_errno(errno)
				    << boost::errinfo_file_name(filename));
			}

			if (fd != 1) {
				/* redirect stdout to the file */
				dup2(fd, 1);
				close(fd);
			}

			/* redirect stderr to stdout */
			if (fd != 2)
				close(2);

			dup2(1, 2);
		}

		char **argv;
		char *my_pid_str = strdup(my_pid.CStr());

		if (interactive) {
			const char *uargv[] = {
				"gdb",
				"-p",
				my_pid_str,
				NULL
			};
			argv = const_cast<char **>(uargv);
		} else {
			const char *uargv[] = {
				"gdb",
				"--batch",
				"-p",
				my_pid_str,
				"-ex",
				"thread apply all bt full",
				"-ex",
				"detach",
				"-ex",
				"quit",
				NULL
			};
			argv = const_cast<char **>(uargv);
		}

		(void)execvp(argv[0], argv);
		perror("Failed to launch GDB");
		free(my_pid_str);
		_exit(0);
	}

	int status;
	if (waitpid(pid, &status, 0) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("waitpid")
		    << boost::errinfo_errno(errno));
	}

#ifdef __linux__
	prctl(PR_SET_DUMPABLE, 0);
#endif /* __linux __ */
#else /* _WIN32 */
	DebugBreak();
#endif /* _WIN32 */
}

#ifndef _WIN32
/**
 * Signal handler for SIGINT and SIGTERM. Prepares the application for cleanly
 * shutting down during the next execution of the event loop.
 *
 * @param - The signal number.
 */
void Application::SigIntTermHandler(int signum)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_DFL;
	sigaction(signum, &sa, NULL);

	Application::Ptr instance = Application::GetInstance();

	if (!instance)
		return;

	instance->RequestShutdown();
}
#endif /* _WIN32 */

/**
 * Signal handler for SIGUSR1. This signal causes Icinga to re-open
 * its log files and is mainly for use by logrotate.
 *
 * @param - The signal number.
 */
void Application::SigUsr1Handler(int)
{
	RequestReopenLogs();
}

/**
 * Signal handler for SIGABRT. Helps with debugging ASSERT()s.
 *
 * @param - The signal number.
 */
void Application::SigAbrtHandler(int)
{
#ifndef _WIN32
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_DFL;
	sigaction(SIGABRT, &sa, NULL);
#endif /* _WIN32 */

	std::cerr << "Caught SIGABRT." << std::endl
		  << "Current time: " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", Utility::GetTime()) << std::endl
		  << std::endl;

	String fname = GetCrashReportFilename();
	String dirName = Utility::DirName(fname);

	if (!Utility::PathExists(dirName)) {
#ifndef _WIN32
		if (mkdir(dirName.CStr(), 0700) < 0 && errno != EEXIST) {
#else /*_ WIN32 */
		if (mkdir(dirName.CStr()) < 0 && errno != EEXIST) {
#endif /* _WIN32 */
			std::cerr << "Could not create directory '" << dirName << "': Error " << errno << ", " << strerror(errno) << "\n";
		}
	}

	bool interactive_debugger = Convert::ToBool(ScriptGlobal::Get("AttachDebugger"));

	if (!interactive_debugger) {
		std::ofstream ofs;
		ofs.open(fname.CStr());

		Log(LogCritical, "Application")
		    << "Icinga 2 has terminated unexpectedly. Additional information can be found in '" << fname << "'" << "\n";

		DisplayInfoMessage(ofs);

		StackTrace trace;
		ofs << "Stacktrace:" << "\n";
		trace.Print(ofs, 1);

		DisplayBugMessage(ofs);

		ofs << "\n";
		ofs.close();
	} else {
		Log(LogCritical, "Application", "Icinga 2 has terminated unexpectedly. Attaching debugger...");
	}

	AttachDebugger(fname, interactive_debugger);
}

#ifdef _WIN32
/**
 * Console control handler. Prepares the application for cleanly
 * shutting down during the next execution of the event loop.
 */
BOOL WINAPI Application::CtrlHandler(DWORD type)
{
	Application::Ptr instance = Application::GetInstance();

	if (!instance)
		return TRUE;

	instance->RequestShutdown();

	SetConsoleCtrlHandler(NULL, FALSE);
	return TRUE;
}

bool Application::IsProcessElevated(void) {
	BOOL fIsElevated = FALSE;
	DWORD dwError = ERROR_SUCCESS;
	HANDLE hToken = NULL;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
		dwError = GetLastError();
	else {
		TOKEN_ELEVATION elevation;
		DWORD dwSize;

		if (!GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize))
			dwError = GetLastError();
		else
			fIsElevated = elevation.TokenIsElevated;
	}

	if (hToken) {
		CloseHandle(hToken);
		hToken = NULL;
	}

	if (ERROR_SUCCESS != dwError) {
		LPSTR mBuf = NULL;
		if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), mBuf, 0, NULL))
			BOOST_THROW_EXCEPTION(std::runtime_error("Failed to format error message, last error was: " + dwError));
		else
			BOOST_THROW_EXCEPTION(std::runtime_error(mBuf));
	}

	return fIsElevated;
}
#endif /* _WIN32 */

/**
 * Handler for unhandled exceptions.
 */
void Application::ExceptionHandler(void)
{
	if (l_InExceptionHandler)
		for (;;)
			Utility::Sleep(5);

	l_InExceptionHandler = true;

#ifndef _WIN32
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_DFL;
	sigaction(SIGABRT, &sa, NULL);
#endif /* _WIN32 */

	String fname = GetCrashReportFilename();
	String dirName = Utility::DirName(fname);

	if (!Utility::PathExists(dirName)) {
#ifndef _WIN32
		if (mkdir(dirName.CStr(), 0700) < 0 && errno != EEXIST) {
#else /*_ WIN32 */
		if (mkdir(dirName.CStr()) < 0 && errno != EEXIST) {
#endif /* _WIN32 */
			std::cerr << "Could not create directory '" << dirName << "': Error " << errno << ", " << strerror(errno) << "\n";
		}
	}

	bool interactive_debugger = Convert::ToBool(ScriptGlobal::Get("AttachDebugger"));

	if (!interactive_debugger) {
		std::ofstream ofs;
		ofs.open(fname.CStr());

		ofs << "Caught unhandled exception." << "\n"
		    << "Current time: " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", Utility::GetTime()) << "\n"
		    << "\n";

		DisplayInfoMessage(ofs);

		try {
			RethrowUncaughtException();
		} catch (const std::exception& ex) {
			Log(LogCritical, "Application")
			    << DiagnosticInformation(ex, false) << "\n"
			    << "\n"
			    << "Additional information is available in '" << fname << "'" << "\n";

			ofs << "\n"
			    << DiagnosticInformation(ex)
			    << "\n";
		}

		DisplayBugMessage(ofs);

		ofs.close();
	}

	AttachDebugger(fname, interactive_debugger);

	abort();
}

#ifdef _WIN32
LONG CALLBACK Application::SEHUnhandledExceptionFilter(PEXCEPTION_POINTERS exi)
{
	if (l_InExceptionHandler)
		return EXCEPTION_CONTINUE_SEARCH;

	l_InExceptionHandler = true;

	String fname = GetCrashReportFilename();
	String dirName = Utility::DirName(fname);

	if (!Utility::PathExists(dirName)) {
#ifndef _WIN32
		if (mkdir(dirName.CStr(), 0700) < 0 && errno != EEXIST) {
#else /*_ WIN32 */
		if (mkdir(dirName.CStr()) < 0 && errno != EEXIST) {
#endif /* _WIN32 */
			std::cerr << "Could not create directory '" << dirName << "': Error " << errno << ", " << strerror(errno) << "\n";
		}
	}

	std::ofstream ofs;
	ofs.open(fname.CStr());

	Log(LogCritical, "Application")
	    << "Icinga 2 has terminated unexpectedly. Additional information can be found in '" << fname << "'";

	DisplayInfoMessage(ofs);

	ofs << "Caught unhandled SEH exception." << "\n"
	    << "Current time: " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", Utility::GetTime()) << "\n"
	    << "\n";

	StackTrace trace(exi);
	ofs << "Stacktrace:" << "\n";
	trace.Print(ofs, 1);

	DisplayBugMessage(ofs);

	return EXCEPTION_CONTINUE_SEARCH;
}
#endif /* _WIN32 */

/**
 * Installs the exception handlers.
 */
void Application::InstallExceptionHandlers(void)
{
	std::set_terminate(&Application::ExceptionHandler);

#ifndef _WIN32
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &Application::SigAbrtHandler;
	sigaction(SIGABRT, &sa, NULL);
#else /* _WIN32 */
	SetUnhandledExceptionFilter(&Application::SEHUnhandledExceptionFilter);
#endif /* _WIN32 */
}

/**
 * Runs the application.
 *
 * @returns The application's exit code.
 */
int Application::Run(void)
{
#ifndef _WIN32
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &Application::SigIntTermHandler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	sa.sa_handler = &Application::SigUsr1Handler;
	sigaction(SIGUSR1, &sa, NULL);
#else /* _WIN32 */
	SetConsoleCtrlHandler(&Application::CtrlHandler, TRUE);
#endif /* _WIN32 */

	try {
		UpdatePidFile(GetPidPath());
	} catch (const std::exception&) {
		Log(LogCritical, "Application")
		    << "Cannot update PID file '" << GetPidPath() << "'. Aborting.";
		return EXIT_FAILURE;
	}

	SetMainTime(Utility::GetTime());

	return Main();
}

/**
 * Grabs the PID file lock and updates the PID. Terminates the application
 * if the PID file is already locked by another instance of the application.
 *
 * @param filename The name of the PID file.
 * @param pid The PID to write; default is the current PID
 */
void Application::UpdatePidFile(const String& filename, pid_t pid)
{
	ObjectLock olock(this);

	if (m_PidFile != NULL)
		fclose(m_PidFile);

	/* There's just no sane way of getting a file descriptor for a
	 * C++ ofstream which is why we're using FILEs here. */
	m_PidFile = fopen(filename.CStr(), "r+");

	if (m_PidFile == NULL)
		m_PidFile = fopen(filename.CStr(), "w");

	if (m_PidFile == NULL) {
		Log(LogCritical, "Application")
		    << "Could not open PID file '" << filename << "'.";
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not open PID file '" + filename + "'"));
	}

#ifndef _WIN32
	int fd = fileno(m_PidFile);

	Utility::SetCloExec(fd);

	struct flock lock;

	lock.l_start = 0;
	lock.l_len = 0;
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;

	if (fcntl(fd, F_SETLK, &lock) < 0) {
		Log(LogCritical, "Application", "Could not lock PID file. Make sure that only one instance of the application is running.");

		Application::Exit(EXIT_FAILURE);
	}

	if (ftruncate(fd, 0) < 0) {
		Log(LogCritical, "Application")
		    << "ftruncate() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";

		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("ftruncate")
		    << boost::errinfo_errno(errno));
	}
#endif /* _WIN32 */

	fprintf(m_PidFile, "%lu\n", (unsigned long)pid);
	fflush(m_PidFile);
}

/**
 * Closes the PID file. Does nothing if the PID file is not currently open.
 */
void Application::ClosePidFile(bool unlink)
{
	ObjectLock olock(this);

	if (m_PidFile != NULL)
	{
		if (unlink) {
			String pidpath = GetPidPath();
			::unlink(pidpath.CStr());
		}

		fclose(m_PidFile);
	}

	m_PidFile = NULL;
}

/**
 * Checks if another process currently owns the pidfile and read it
 *
 * @param filename The name of the PID file.
 * @returns 0: no process owning the pidfile, pid of the process otherwise
 */
pid_t Application::ReadPidFile(const String& filename)
{
	FILE *pidfile = fopen(filename.CStr(), "r");

	if (pidfile == NULL)
		return 0;

#ifndef _WIN32
	int fd = fileno(pidfile);

	struct flock lock;

	lock.l_start = 0;
	lock.l_len = 0;
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;

	if (fcntl(fd, F_GETLK, &lock) < 0) {
		int error = errno;
		fclose(pidfile);
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("fcntl")
		    << boost::errinfo_errno(error));
	}

	if (lock.l_type == F_UNLCK) {
		// nobody has locked the file: no icinga running
		fclose(pidfile);
		return -1;
	}
#endif /* _WIN32 */

	pid_t runningpid;
	int res = fscanf(pidfile, "%d", &runningpid);
	fclose(pidfile);

	// bogus result?
	if (res != 1)
		return 0;

#ifdef _WIN32
	HANDLE hProcess = OpenProcess(0, FALSE, runningpid);

	if (!hProcess)
		return 0;

	CloseHandle(hProcess);
#endif /* _WIN32 */

	return runningpid;
}


/**
 * Retrieves the path of the installation prefix.
 *
 * @returns The path.
 */
String Application::GetPrefixDir(void)
{
	return ScriptGlobal::Get("PrefixDir");
}

/**
 * Sets the path for the installation prefix.
 *
 * @param path The new path.
 */
void Application::DeclarePrefixDir(const String& path)
{
	if (!ScriptGlobal::Exists("PrefixDir"))
		ScriptGlobal::Set("PrefixDir", path);
}

/**
 * Retrives the path of the sysconf dir.
 *
 * @returns The path.
 */
String Application::GetSysconfDir(void)
{
	return ScriptGlobal::Get("SysconfDir");
}

/**
 * Sets the path of the sysconf dir.
 *
 * @param path The new path.
 */
void Application::DeclareSysconfDir(const String& path)
{
	if (!ScriptGlobal::Exists("SysconfDir"))
		ScriptGlobal::Set("SysconfDir", path);
}

/**
 * Retrieves the path for the run dir.
 *
 * @returns The path.
 */
String Application::GetRunDir(void)
{
	return ScriptGlobal::Get("RunDir");
}

/**
 * Sets the path of the run dir.
 *
 * @param path The new path.
 */
void Application::DeclareRunDir(const String& path)
{
	if (!ScriptGlobal::Exists("RunDir"))
		ScriptGlobal::Set("RunDir", path);
}

/**
 * Retrieves the path for the local state dir.
 *
 * @returns The path.
 */
String Application::GetLocalStateDir(void)
{
	return ScriptGlobal::Get("LocalStateDir");
}

/**
 * Sets the path for the local state dir.
 *
 * @param path The new path.
 */
void Application::DeclareLocalStateDir(const String& path)
{
	if (!ScriptGlobal::Exists("LocalStateDir"))
		ScriptGlobal::Set("LocalStateDir", path);
}

/**
 * Retrieves the path for the local state dir.
 *
 * @returns The path.
 */
String Application::GetZonesDir(void)
{
	return ScriptGlobal::Get("ZonesDir", &Empty);
}

/**
 * Sets the path of the zones dir.
 *
 * @param path The new path.
 */
void Application::DeclareZonesDir(const String& path)
{
	if (!ScriptGlobal::Exists("ZonesDir"))
		ScriptGlobal::Set("ZonesDir", path);
}

/**
 * Retrieves the path for the package data dir.
 *
 * @returns The path.
 */
String Application::GetPkgDataDir(void)
{
	String defaultValue = "";
	return ScriptGlobal::Get("PkgDataDir", &Empty);
}

/**
 * Sets the path for the package data dir.
 *
 * @param path The new path.
 */
void Application::DeclarePkgDataDir(const String& path)
{
	if (!ScriptGlobal::Exists("PkgDataDir"))
		ScriptGlobal::Set("PkgDataDir", path);
}

/**
 * Retrieves the path for the include conf dir.
 *
 * @returns The path.
 */
String Application::GetIncludeConfDir(void)
{
	return ScriptGlobal::Get("IncludeConfDir", &Empty);
}

/**
 * Sets the path for the package data dir.
 *
 * @param path The new path.
 */
void Application::DeclareIncludeConfDir(const String& path)
{
	if (!ScriptGlobal::Exists("IncludeConfDir"))
		ScriptGlobal::Set("IncludeConfDir", path);
}

/**
 * Retrieves the path for the state file.
 *
 * @returns The path.
 */
String Application::GetStatePath(void)
{
	return ScriptGlobal::Get("StatePath", &Empty);
}

/**
 * Sets the path for the state file.
 *
 * @param path The new path.
 */
void Application::DeclareStatePath(const String& path)
{
	if (!ScriptGlobal::Exists("StatePath"))
		ScriptGlobal::Set("StatePath", path);
}

/**
 * Retrieves the path for the modified attributes file.
 *
 * @returns The path.
 */
String Application::GetModAttrPath(void)
{
	return ScriptGlobal::Get("ModAttrPath", &Empty);
}

/**
 * Sets the path for the modified attributes file.
 *
 * @param path The new path.
 */
void Application::DeclareModAttrPath(const String& path)
{
	if (!ScriptGlobal::Exists("ModAttrPath"))
		ScriptGlobal::Set("ModAttrPath", path);
}

/**
 * Retrieves the path for the objects file.
 *
 * @returns The path.
 */
String Application::GetObjectsPath(void)
{
	return ScriptGlobal::Get("ObjectsPath", &Empty);
}

/**
 * Sets the path for the objects file.
 *
 * @param path The new path.
 */
void Application::DeclareObjectsPath(const String& path)
{
	if (!ScriptGlobal::Exists("ObjectsPath"))
		ScriptGlobal::Set("ObjectsPath", path);
}

/**
* Retrieves the path for the vars file.
*
* @returns The path.
*/
String Application::GetVarsPath(void)
{
	return ScriptGlobal::Get("VarsPath", &Empty);
}

/**
* Sets the path for the vars file.
*
* @param path The new path.
*/
void Application::DeclareVarsPath(const String& path)
{
	if (!ScriptGlobal::Exists("VarsPath"))
		ScriptGlobal::Set("VarsPath", path);
}

/**
 * Retrieves the path for the PID file.
 *
 * @returns The path.
 */
String Application::GetPidPath(void)
{
	return ScriptGlobal::Get("PidPath", &Empty);
}

/**
 * Sets the path for the PID file.
 *
 * @param path The new path.
 */
void Application::DeclarePidPath(const String& path)
{
	if (!ScriptGlobal::Exists("PidPath"))
		ScriptGlobal::Set("PidPath", path);
}

/**
 * Retrieves the name of the user.
 *
 * @returns The name.
 */
String Application::GetRunAsUser(void)
{
	return ScriptGlobal::Get("RunAsUser");
}

/**
 * Sets the name of the user.
 *
 * @param path The new user name.
 */
void Application::DeclareRunAsUser(const String& user)
{
	if (!ScriptGlobal::Exists("RunAsUser"))
		ScriptGlobal::Set("RunAsUser", user);
}

/**
 * Retrieves the name of the group.
 *
 * @returns The name.
 */
String Application::GetRunAsGroup(void)
{
	return ScriptGlobal::Get("RunAsGroup");
}

/**
 * Sets the name of the group.
 *
 * @param path The new group name.
 */
void Application::DeclareRunAsGroup(const String& group)
{
	if (!ScriptGlobal::Exists("RunAsGroup"))
		ScriptGlobal::Set("RunAsGroup", group);
}

/**
 * Retrieves the file rlimit.
 *
 * @returns The limit.
 */
int Application::GetRLimitFiles(void)
{
	return ScriptGlobal::Get("RLimitFiles");
}

int Application::GetDefaultRLimitFiles(void)
{
	return 16 * 1024;
}

/**
 * Sets the file rlimit.
 *
 * @param path The new file rlimit.
 */
void Application::DeclareRLimitFiles(int limit)
{
	if (!ScriptGlobal::Exists("RLimitFiles"))
		ScriptGlobal::Set("RLimitFiles", limit);
}

/**
 * Retrieves the process rlimit.
 *
 * @returns The limit.
 */
int Application::GetRLimitProcesses(void)
{
	return ScriptGlobal::Get("RLimitProcesses");
}

int Application::GetDefaultRLimitProcesses(void)
{
	return 16 * 1024;
}

/**
 * Sets the process rlimit.
 *
 * @param path The new process rlimit.
 */
void Application::DeclareRLimitProcesses(int limit)
{
	if (!ScriptGlobal::Exists("RLimitProcesses"))
		ScriptGlobal::Set("RLimitProcesses", limit);
}

/**
 * Retrieves the stack rlimit.
 *
 * @returns The limit.
 */
int Application::GetRLimitStack(void)
{
	return ScriptGlobal::Get("RLimitStack");
}

int Application::GetDefaultRLimitStack(void)
{
	return 256 * 1024;
}

/**
 * Sets the stack rlimit.
 *
 * @param path The new stack rlimit.
 */
void Application::DeclareRLimitStack(int limit)
{
	if (!ScriptGlobal::Exists("RLimitStack"))
		ScriptGlobal::Set("RLimitStack", limit);
}

/**
 * Sets the concurrency level.
 *
 * @param path The new concurrency level.
 */
void Application::DeclareConcurrency(int ncpus)
{
	if (!ScriptGlobal::Exists("Concurrency"))
		ScriptGlobal::Set("Concurrency", ncpus);
}

/**
 * Retrieves the concurrency level.
 *
 * @returns The concurrency level.
 */
int Application::GetConcurrency(void)
{
	Value defaultConcurrency = std::thread::hardware_concurrency();
	return ScriptGlobal::Get("Concurrency", &defaultConcurrency);
}

/**
 * Returns the global thread pool.
 *
 * @returns The global thread pool.
 */
ThreadPool& Application::GetTP(void)
{
	static ThreadPool tp;
	return tp;
}

double Application::GetStartTime(void)
{
	return m_StartTime;
}

void Application::SetStartTime(double ts)
{
	m_StartTime = ts;
}

double Application::GetMainTime(void)
{
	return m_MainTime;
}

void Application::SetMainTime(double ts)
{
	m_MainTime = ts;
}

bool Application::GetScriptDebuggerEnabled(void)
{
	return m_ScriptDebuggerEnabled;
}

void Application::SetScriptDebuggerEnabled(bool enabled)
{
	m_ScriptDebuggerEnabled = enabled;
}

double Application::GetLastReloadFailed(void)
{
	return m_LastReloadFailed;
}

void Application::SetLastReloadFailed(double ts)
{
	m_LastReloadFailed = ts;
}

void Application::ValidateName(const String& value, const ValidationUtils& utils)
{
	ObjectImpl<Application>::ValidateName(value, utils);

	if (value != "app")
		BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of("name"), "Application object must be named 'app'."));
}
