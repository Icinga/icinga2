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

using namespace icinga;

Application::Ptr Application::m_Instance;
bool Application::m_ShuttingDown = false;
bool Application::m_Debugging = false;
boost::thread::id Application::m_MainThreadID;

/**
 * Constructor for the Application class.
 */
Application::Application(void)
	: m_PidFile(NULL)
{
#ifdef _WIN32
	/* disable GUI-based error messages for LoadLibrary() */
	SetErrorMode(SEM_FAILCRITICALERRORS);

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
		throw_exception(Win32Exception("WSAStartup failed", WSAGetLastError()));
#else /* _WIN32 */
	lt_dlinit();
#endif /* _WIN32 */

	char *debugging = getenv("_DEBUG");
	m_Debugging = (debugging && strtol(debugging, NULL, 10) != 0);

#ifdef _WIN32
	if (IsDebuggerPresent())
		m_Debugging = true;
#endif /* _WIN32 */
}

/**
 * Destructor for the application class.
 */
Application::~Application(void)
{
	m_ShuttingDown = true;

	DynamicObject::DeactivateObjects();

#ifdef _WIN32
	WSACleanup();
#endif /* _WIN32 */

	ClosePidFile();
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

/**
 * Processes events for registered sockets and timers and calls whatever
 * handlers have been set up for these events.
 */
void Application::RunEventLoop(void)
{
#ifdef _DEBUG
	double nextProfile = 0;
#endif /* _DEBUG */

	while (!m_ShuttingDown) {
		Object::ClearHeldObjects();

		double sleep = Timer::ProcessTimers();

		if (m_ShuttingDown)
			break;

		Event::ProcessEvents(boost::get_system_time() + boost::posix_time::milliseconds(sleep * 1000));

		DynamicObject::FinishTx();
		DynamicObject::BeginTx();

#ifdef _DEBUG
		if (nextProfile < Utility::GetTime()) {
			stringstream msgbuf;
			msgbuf << "Active objects: " << Object::GetAliveObjects();
			Logger::Write(LogInformation, "base", msgbuf.str());

			Object::PrintMemoryProfile();

			nextProfile = Utility::GetTime() + 15.0;
		}
#endif /* _DEBUG */
	}
}

/**
 * Signals the application to shut down during the next
 * execution of the event loop.
 */
void Application::Shutdown(void)
{
	m_ShuttingDown = true;
}

/**
 * Retrieves the full path of the executable.
 *
 * @returns The path.
 */
String Application::GetExePath(void) const
{
	static String result;

	if (!result.IsEmpty())
		return result;

	String executablePath;

#ifndef _WIN32
	String argv0 = m_Arguments[0];

	char buffer[MAXPATHLEN];
	if (getcwd(buffer, sizeof(buffer)) == NULL)
		throw_exception(PosixException("getcwd failed", errno));
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
			vector<String> paths;
			boost::algorithm::split(paths, pathEnv, boost::is_any_of(":"));

			bool foundPath = false;
			BOOST_FOREACH(String& path, paths) {
				String pathTest = path + "/" + argv0;

				if (access(pathTest.CStr(), X_OK) == 0) {
					executablePath = pathTest;
					foundPath = true;
					break;
				}
			}

			if (!foundPath) {
				executablePath.Clear();
				throw_exception(runtime_error("Could not determine executable path."));
			}
		}
	}

	if (realpath(executablePath.CStr(), buffer) == NULL)
		throw_exception(PosixException("realpath failed", errno));

	result = buffer;
#else /* _WIN32 */
	char FullExePath[MAXPATHLEN];

	if (!GetModuleFileName(NULL, FullExePath, sizeof(FullExePath)))
		throw_exception(Win32Exception("GetModuleFileName() failed", GetLastError()));

	result = FullExePath;
#endif /* _WIN32 */

	return result;
}

/**
 * Retrieves the debugging mode of the application.
 *
 * @returns true if the application is being debugged, false otherwise
 */
bool Application::IsDebugging(void)
{
	return m_Debugging;
}

bool Application::IsMainThread(void)
{
	return (boost::this_thread::get_id() == m_MainThreadID);
}

#ifndef _WIN32
/**
 * Signal handler for SIGINT. Prepares the application for cleanly
 * shutting down during the next execution of the event loop.
 *
 * @param signum The signal number.
 */
void Application::SigIntHandler(int signum)
{
	assert(signum == SIGINT);

	Application::Ptr instance = Application::GetInstance();

	if (!instance)
		return;

	instance->Shutdown();

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_DFL;
	sigaction(SIGINT, &sa, NULL);
}
#else /* _WIN32 */
/**
 * Console control handler. Prepares the application for cleanly
 * shutting down during the next execution of the event loop.
 */
BOOL WINAPI Application::CtrlHandler(DWORD type)
{
	Application::Ptr instance = Application::GetInstance();

	if (!instance)
		return TRUE;

	instance->GetInstance()->Shutdown();

	SetConsoleCtrlHandler(NULL, FALSE);
	return TRUE;
}
#endif /* _WIN32 */

/**
 * Runs the application.
 *
 * @param argc The number of arguments.
 * @param argv The arguments that should be passed to the application.
 * @returns The application's exit code.
 */
int Application::Run(int argc, char **argv)
{
	int result;

	assert(!Application::m_Instance);

	m_MainThreadID = boost::this_thread::get_id();

	Application::m_Instance = GetSelf();

#ifndef _WIN32
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &Application::SigIntHandler;
	sigaction(SIGINT, &sa, NULL);

	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL);
#else
	SetConsoleCtrlHandler(&Application::CtrlHandler, TRUE);
#endif /* _WIN32 */

	m_Arguments.clear();
	for (int i = 0; i < argc; i++)
		m_Arguments.push_back(String(argv[i]));

	DynamicObject::BeginTx();

	if (IsDebugging()) {
		result = Main(m_Arguments);

		Application::m_Instance.reset();
	} else {
		try {
			result = Main(m_Arguments);
		} catch (const exception& ex) {
			Application::m_Instance.reset();

			Logger::Write(LogCritical, "base", "---");
			Logger::Write(LogCritical, "base", "Exception: " + Utility::GetTypeName(typeid(ex)));
			Logger::Write(LogCritical, "base", "Message: " + String(ex.what()));

			return EXIT_FAILURE;
		}
	}

	DynamicObject::FinishTx();

	return result;
}

void Application::UpdatePidFile(const String& filename)
{
	ClosePidFile();

	/* There's just no sane way of getting a file descriptor for a
	 * C++ ofstream which is why we're using FILEs here. */
	m_PidFile = fopen(filename.CStr(), "w");

	if (m_PidFile == NULL)
		throw_exception(runtime_error("Could not open PID file '" + filename + "'"));

#ifndef _WIN32
	if (flock(fileno(m_PidFile), LOCK_EX | LOCK_NB) < 0) {
		ClosePidFile();

		throw_exception(runtime_error("Another instance of the application is "
		    "already running. Remove the '" + filename + "' file if "
		    "you're certain that this is not the case."));
	}
#endif /* _WIN32 */

#ifndef _WIN32
	pid_t pid = getpid();
#else /* _WIN32 */
	DWORD pid = GetCurrentProcessId();
#endif /* _WIN32 */

	fprintf(m_PidFile, "%d", pid);
	fflush(m_PidFile);
}

void Application::ClosePidFile(void)
{
	if (m_PidFile != NULL)
		fclose(m_PidFile);

	m_PidFile = NULL;
}

