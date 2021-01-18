/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/application.hpp"
#include "base/application-ti.cpp"
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
#include "base/tlsutility.hpp"
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
#else /* _WIN32 */
#include <signal.h>
#endif /* _WIN32 */

using namespace icinga;

REGISTER_TYPE(Application);

boost::signals2::signal<void ()> Application::OnReopenLogs;
Application::Ptr Application::m_Instance = nullptr;
bool Application::m_ShuttingDown = false;
bool Application::m_RequestRestart = false;
bool Application::m_RequestReopenLogs = false;
pid_t Application::m_ReloadProcess = 0;

#ifndef _WIN32
pid_t Application::m_UmbrellaProcess = 0;
#endif /* _WIN32 */

static bool l_Restarting = false;
static bool l_InExceptionHandler = false;
int Application::m_ArgC;
char **Application::m_ArgV;
double Application::m_StartTime;
bool Application::m_ScriptDebuggerEnabled = false;
double Application::m_LastReloadFailed;

/**
 * Constructor for the Application class.
 */
void Application::OnConfigLoaded()
{
	m_PidFile = nullptr;

	ASSERT(m_Instance == nullptr);
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

#ifdef _WIN32
	ClosePidFile(true);
#endif /* _WIN32 */

	ObjectImpl<Application>::Stop(runtimeRemoved);
}

Application::~Application()
{
	m_Instance = nullptr;
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

void Application::InitializeBase()
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
	sigaction(SIGPIPE, &sa, nullptr);
#endif /* _WIN32 */

	Loader::ExecuteDeferredInitializers();

	/* Make sure the thread pool gets initialized. */
	GetTP().Start();

	/* Make sure the timer thread gets initialized. */
	Timer::Initialize();
}

void Application::UninitializeBase()
{
	Timer::Uninitialize();

	GetTP().Stop();
}

/**
 * Retrieves a pointer to the application singleton object.
 *
 * @returns The application object.
 */
Application::Ptr Application::GetInstance()
{
	return m_Instance;
}

void Application::SetResourceLimits()
{
#ifdef __linux__
	rlimit rl;

#	ifdef RLIMIT_NOFILE
	rlim_t fileLimit = Configuration::RLimitFiles;

	if (fileLimit != 0) {
		if (fileLimit < (rlim_t)GetDefaultRLimitFiles()) {
			Log(LogWarning, "Application")
				<< "The user-specified value for RLimitFiles cannot be smaller than the default value (" << GetDefaultRLimitFiles() << "). Using the default value instead.";
			fileLimit = GetDefaultRLimitFiles();
		}

		rl.rlim_cur = fileLimit;
		rl.rlim_max = rl.rlim_cur;

		if (setrlimit(RLIMIT_NOFILE, &rl) < 0)
			Log(LogWarning, "Application")
			    << "Failed to adjust resource limit for open file handles (RLIMIT_NOFILE) with error \"" << strerror(errno) << "\"";
#	else /* RLIMIT_NOFILE */
		Log(LogNotice, "Application", "System does not support adjusting the resource limit for open file handles (RLIMIT_NOFILE)");
#	endif /* RLIMIT_NOFILE */
	}

#	ifdef RLIMIT_NPROC
	rlim_t processLimit = Configuration::RLimitProcesses;

	if (processLimit != 0) {
		if (processLimit < (rlim_t)GetDefaultRLimitProcesses()) {
			Log(LogWarning, "Application")
				<< "The user-specified value for RLimitProcesses cannot be smaller than the default value (" << GetDefaultRLimitProcesses() << "). Using the default value instead.";
			processLimit = GetDefaultRLimitProcesses();
		}

		rl.rlim_cur = processLimit;
		rl.rlim_max = rl.rlim_cur;

		if (setrlimit(RLIMIT_NPROC, &rl) < 0)
			Log(LogWarning, "Application")
			    << "Failed adjust resource limit for number of processes (RLIMIT_NPROC) with error \"" << strerror(errno) << "\"";
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

	stackLimit = Configuration::RLimitStack;

	if (stackLimit != 0) {
		if (stackLimit < (rlim_t)GetDefaultRLimitStack()) {
			Log(LogWarning, "Application")
				<< "The user-specified value for RLimitStack cannot be smaller than the default value (" << GetDefaultRLimitStack() << "). Using the default value instead.";
			stackLimit = GetDefaultRLimitStack();
		}

		if (set_stack_rlimit)
			rl.rlim_cur = stackLimit;
		else
			rl.rlim_cur = rl.rlim_max;

		if (setrlimit(RLIMIT_STACK, &rl) < 0)
			Log(LogWarning, "Application")
			    << "Failed adjust resource limit for stack size (RLIMIT_STACK) with error \"" << strerror(errno) << "\"";
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

			new_argv[argc + 1] = nullptr;

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

int Application::GetArgC()
{
	return m_ArgC;
}

void Application::SetArgC(int argc)
{
	m_ArgC = argc;
}

char **Application::GetArgV()
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
void Application::RunEventLoop()
{
	double lastLoop = Utility::GetTime();

	while (!m_ShuttingDown) {
		if (m_RequestRestart) {
			m_RequestRestart = false;         // we are now handling the request, once is enough

#ifdef _WIN32
			// are we already restarting? ignore request if we already are
			if (!l_Restarting) {
				l_Restarting = true;
				m_ReloadProcess = StartReloadProcess();
			}
#else /* _WIN32 */
			Log(LogNotice, "Application")
				<< "Got reload command, forwarding to umbrella process (PID " << m_UmbrellaProcess << ")";

			(void)kill(m_UmbrellaProcess, SIGHUP);
#endif /* _WIN32 */
		} else {
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
	}

	Log(LogInformation, "Application", "Shutting down...");

	ConfigObject::StopObjects();
	Application::GetInstance()->OnShutdown();

#ifdef I2_DEBUG
	UninitializeBase(); // Inspired from Exit()
#endif /* I2_DEBUG */
}

bool Application::IsShuttingDown()
{
	return m_ShuttingDown;
}

bool Application::IsRestarting()
{
	return l_Restarting;
}

void Application::OnShutdown()
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

	std::thread t([pr]() { ReloadProcessCallbackInternal(pr); });
	t.detach();
}

pid_t Application::StartReloadProcess()
{
	// prepare arguments
	ArrayData args;
	args.push_back(GetExePath(m_ArgV[0]));

	for (int i=1; i < Application::GetArgC(); i++) {
		if (std::string(Application::GetArgV()[i]) != "--reload-internal")
			args.push_back(Application::GetArgV()[i]);
		else
			i++;     // the next parameter after --reload-internal is the pid, remove that too
	}

#ifndef _WIN32
	args.push_back("--reload-internal");
	args.push_back(Convert::ToString(Utility::GetPid()));
#else /* _WIN32 */
	args.push_back("--validate");
#endif /* _WIN32 */

	double reloadTimeout = Application::GetReloadTimeout();

	Process::Ptr process = new Process(Process::PrepareCommand(new Array(std::move(args))));
	process->SetTimeout(reloadTimeout);
	process->Run(&ReloadProcessCallback);

	Log(LogInformation, "Application")
		<< "Got reload command: Started new instance with PID '"
		<< (unsigned long)(process->GetPID()) << "' (timeout is "
		<< reloadTimeout << "s).";

	return process->GetPID();
}

/**
 * Signals the application to shut down during the next
 * execution of the event loop.
 */
void Application::RequestShutdown()
{
	Log(LogInformation, "Application", "Received request to shut down.");

	m_ShuttingDown = true;
}

/**
 * Signals the application to restart during the next
 * execution of the event loop.
 */
void Application::RequestRestart()
{
	m_RequestRestart = true;
}

/**
 * Signals the application to reopen log files during the
 * next execution of the event loop.
 */
void Application::RequestReopenLogs()
{
	m_RequestReopenLogs = true;
}

#ifndef _WIN32
/**
 * Sets the PID of the Icinga umbrella process.
 *
 * @param pid The PID of the Icinga umbrella process.
 */
void Application::SetUmbrellaProcess(pid_t pid)
{
	m_UmbrellaProcess = pid;
}
#endif /* _WIN32 */

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
	if (!getcwd(buffer, sizeof(buffer))) {
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
		String pathEnv = Utility::GetFromEnvironment("PATH");
		if (!pathEnv.IsEmpty()) {
			std::vector<String> paths = String(pathEnv).Split(":");

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

	if (!realpath(executablePath.CStr(), buffer)) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("realpath")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(executablePath));
	}

	return buffer;
#else /* _WIN32 */
	char FullExePath[MAXPATHLEN];

	if (!GetModuleFileName(nullptr, FullExePath, sizeof(FullExePath)))
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
	/* icinga-app prints its own version information, stack traces need it here. */
	if (!skipVersion)
		os << "  Application version: " << GetAppVersion() << "\n\n";

	os << "System information:\n"
		<< "  Platform: " << Utility::GetPlatformName() << "\n"
		<< "  Platform version: " << Utility::GetPlatformVersion() << "\n"
		<< "  Kernel: " << Utility::GetPlatformKernel() << "\n"
		<< "  Kernel version: " << Utility::GetPlatformKernelVersion() << "\n"
		<< "  Architecture: " << Utility::GetPlatformArchitecture() << "\n";

	Namespace::Ptr systemNS = ScriptGlobal::Get("System");

	os << "\nBuild information:\n"
		<< "  Compiler: " << systemNS->Get("BuildCompilerName") << " " << systemNS->Get("BuildCompilerVersion") << "\n"
		<< "  Build host: " << systemNS->Get("BuildHostName") << "\n"
		<< "  OpenSSL version: " << GetOpenSSLVersion() << "\n";

	os << "\nApplication information:\n"
		<< "\nGeneral paths:\n"
		<< "  Config directory: " << Configuration::ConfigDir << "\n"
		<< "  Data directory: " << Configuration::DataDir << "\n"
		<< "  Log directory: " << Configuration::LogDir << "\n"
		<< "  Cache directory: " << Configuration::CacheDir << "\n"
		<< "  Spool directory: " << Configuration::SpoolDir << "\n"
		<< "  Run directory: " << Configuration::InitRunDir << "\n"
		<< "\nOld paths (deprecated):\n"
		<< "  Installation root: " << Configuration::PrefixDir << "\n"
		<< "  Sysconf directory: " << Configuration::SysconfDir << "\n"
		<< "  Run directory (base): " << Configuration::RunDir << "\n"
		<< "  Local state directory: " << Configuration::LocalStateDir << "\n"
		<< "\nInternal paths:\n"
		<< "  Package data directory: " << Configuration::PkgDataDir << "\n"
		<< "  State path: " << Configuration::StatePath << "\n"
		<< "  Modified attributes path: " << Configuration::ModAttrPath << "\n"
		<< "  Objects path: " << Configuration::ObjectsPath << "\n"
		<< "  Vars path: " << Configuration::VarsPath << "\n"
		<< "  PID path: " << Configuration::PidPath << "\n";

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

String Application::GetCrashReportFilename()
{
	return Configuration::LogDir + "/crash/report." + Convert::ToString(Utility::GetTime());
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
				nullptr
			};

			argv = const_cast<char **>(uargv);

			(void) execvp(argv[0], argv);
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
				nullptr
			};

			argv = const_cast<char **>(uargv);

			(void) execvp(argv[0], argv);
		}

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

/**
 * Signal handler for SIGUSR1. This signal causes Icinga to re-open
 * its log files and is mainly for use by logrotate.
 *
 * @param - The signal number.
 */
void Application::SigUsr1Handler(int)
{
	Log(LogInformation, "Application")
		<< "Received USR1 signal, reopening application logs.";

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
	sigaction(SIGABRT, &sa, nullptr);
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

	bool interactive_debugger = Configuration::AttachDebugger;

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

	SetConsoleCtrlHandler(nullptr, FALSE);
	return TRUE;
}

bool Application::IsProcessElevated() {
	BOOL fIsElevated = FALSE;
	DWORD dwError = ERROR_SUCCESS;
	HANDLE hToken = nullptr;

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
		hToken = nullptr;
	}

	if (ERROR_SUCCESS != dwError) {
		LPSTR mBuf = nullptr;
		if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), mBuf, 0, nullptr))
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
void Application::ExceptionHandler()
{
	if (l_InExceptionHandler)
		for (;;)
			Utility::Sleep(5);

	l_InExceptionHandler = true;

#ifndef _WIN32
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_DFL;
	sigaction(SIGABRT, &sa, nullptr);
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

	bool interactive_debugger = Configuration::AttachDebugger;

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
void Application::InstallExceptionHandlers()
{
	std::set_terminate(&Application::ExceptionHandler);

#ifndef _WIN32
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &Application::SigAbrtHandler;
	sigaction(SIGABRT, &sa, nullptr);
#else /* _WIN32 */
	SetUnhandledExceptionFilter(&Application::SEHUnhandledExceptionFilter);
#endif /* _WIN32 */
}

/**
 * Runs the application.
 *
 * @returns The application's exit code.
 */
int Application::Run()
{
#ifndef _WIN32
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &Application::SigUsr1Handler;
	sigaction(SIGUSR1, &sa, nullptr);
#else /* _WIN32 */
	SetConsoleCtrlHandler(&Application::CtrlHandler, TRUE);
#endif /* _WIN32 */

#ifdef _WIN32
	try {
		UpdatePidFile(Configuration::PidPath);
	} catch (const std::exception&) {
		Log(LogCritical, "Application")
			<< "Cannot update PID file '" << Configuration::PidPath << "'. Aborting.";
		return EXIT_FAILURE;
	}
#endif /* _WIN32 */

	SetStartTime(Utility::GetTime());

	return Main();
}

void Application::UpdatePidFile(const String& filename)
{
	UpdatePidFile(filename, Utility::GetPid());
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

	if (m_PidFile)
		fclose(m_PidFile);

	/* There's just no sane way of getting a file descriptor for a
	 * C++ ofstream which is why we're using FILEs here. */
	m_PidFile = fopen(filename.CStr(), "r+");

	if (!m_PidFile)
		m_PidFile = fopen(filename.CStr(), "w");

	if (!m_PidFile) {
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

	if (m_PidFile) {
		if (unlink) {
			String pidpath = Configuration::PidPath;
			::unlink(pidpath.CStr());
		}

		fclose(m_PidFile);
	}

	m_PidFile = nullptr;
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

	if (!pidfile)
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

int Application::GetDefaultRLimitFiles()
{
	return 16 * 1024;
}

int Application::GetDefaultRLimitProcesses()
{
	return 16 * 1024;
}

int Application::GetDefaultRLimitStack()
{
	return 256 * 1024;
}

double Application::GetReloadTimeout()
{
	return ScriptGlobal::Get("ReloadTimeout");
}

/**
 * Returns the global thread pool.
 *
 * @returns The global thread pool.
 */
ThreadPool& Application::GetTP()
{
	static ThreadPool tp;
	return tp;
}

double Application::GetStartTime()
{
	return m_StartTime;
}

void Application::SetStartTime(double ts)
{
	m_StartTime = ts;
}

double Application::GetUptime()
{
	return Utility::GetTime() - m_StartTime;
}

bool Application::GetScriptDebuggerEnabled()
{
	return m_ScriptDebuggerEnabled;
}

void Application::SetScriptDebuggerEnabled(bool enabled)
{
	m_ScriptDebuggerEnabled = enabled;
}

double Application::GetLastReloadFailed()
{
	return m_LastReloadFailed;
}

void Application::SetLastReloadFailed(double ts)
{
	m_LastReloadFailed = ts;
}

void Application::ValidateName(const Lazy<String>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<Application>::ValidateName(lvalue, utils);

	if (lvalue() != "app")
		BOOST_THROW_EXCEPTION(ValidationError(this, { "name" }, "Application object must be named 'app'."));
}
