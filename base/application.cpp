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

#ifndef _WIN32
#	include <ltdl.h>
#endif

using std::cout;
using std::endl;

using namespace icinga;

Application::Ptr I2_EXPORT Application::m_Instance;
bool I2_EXPORT Application::m_ShuttingDown = false;
bool I2_EXPORT Application::m_Debugging = false;

/**
 * Constructor for the Application class.
 */
Application::Application(void)
{
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
		throw Win32Exception("WSAStartup failed", WSAGetLastError());
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

	/* stop all components */
	for (map<string, Component::Ptr>::iterator i = m_Components.begin();
	    i != m_Components.end(); i++) {
		i->second->Stop();
	}

	m_Components.clear();

#ifdef _WIN32
	WSACleanup();
#else /* _WIN32 */
	//lt_dlexit();
#endif /* _WIN32 */
}

/**
 * Retrieves a pointer to the application singleton object.
 *
 * @returns The application object.
 */
Application::Ptr Application::GetInstance(void)
{
	if (m_ShuttingDown)
		return Application::Ptr();
	else
		return m_Instance;
}

/**
 * Processes events for registered sockets and timers and calls whatever
 * handlers have been set up for these events.
 */
void Application::RunEventLoop(void)
{
	while (!m_ShuttingDown) {
		fd_set readfds, writefds, exceptfds;
		int nfds = -1;

		Object::ClearHeldObjects();

		long sleep = Timer::ProcessTimers();

		if (m_ShuttingDown)
			break;

		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_ZERO(&exceptfds);

		Socket::CollectionType::iterator prev, i;
		for (i = Socket::Sockets.begin();
		    i != Socket::Sockets.end(); ) {
			Socket::Ptr socket = i->lock();

			prev = i;
			i++;

			if (!socket) {
				Socket::Sockets.erase(prev);
				continue;
			}

			int fd = socket->GetFD();

			if (socket->WantsToWrite())
				FD_SET(fd, &writefds);

			if (socket->WantsToRead())
				FD_SET(fd, &readfds);

			FD_SET(fd, &exceptfds);

			if (fd > nfds)
				nfds = fd;
		}

		timeval tv;
		tv.tv_sec = sleep;
		tv.tv_usec = 0;

		int ready;

		if (nfds == -1) {
			Sleep(tv.tv_sec * 1000 + tv.tv_usec);
			ready = 0;
		} else
			ready = select(nfds + 1, &readfds, &writefds,
			    &exceptfds, &tv);

		if (ready < 0)
			break;
		else if (ready == 0)
			continue;

		for (i = Socket::Sockets.begin();
		    i != Socket::Sockets.end(); ) {
			Socket::Ptr socket = i->lock();

			prev = i;
			i++;

			if (!socket) {
				Socket::Sockets.erase(prev);
				continue;
			}

			int fd;

			fd = socket->GetFD();
			if (fd != INVALID_SOCKET && FD_ISSET(fd, &writefds))
				socket->OnWritable(socket);

			fd = socket->GetFD();
			if (fd != INVALID_SOCKET && FD_ISSET(fd, &readfds))
				socket->OnReadable(socket);

			fd = socket->GetFD();
			if (fd != INVALID_SOCKET && FD_ISSET(fd, &exceptfds))
				socket->OnException(socket);
		}
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
 * Loads a component from a shared library.
 *
 * @param path The path of the component library.
 * @param componentConfig The configuration for the component.
 * @returns The component.
 */
Component::Ptr Application::LoadComponent(const string& path,
    const ConfigObject::Ptr& componentConfig)
{
	Component::Ptr component;
	Component *(*pCreateComponent)();

	Log(LogInformation, "base", "Loading component '" + path + "'");

#ifdef _WIN32
	HMODULE hModule = LoadLibrary(path.c_str());
#else /* _WIN32 */
	lt_dlhandle hModule = lt_dlopen(path.c_str());
#endif /* _WIN32 */

	if (hModule == NULL)
		throw runtime_error("Could not load module");

#ifdef _WIN32
	pCreateComponent = (CreateComponentFunction)GetProcAddress(hModule,
	    "CreateComponent");
#else /* _WIN32 */
#	ifdef __GNUC__
	/* suppress compiler warning for void * cast */
	__extension__
#	endif
	pCreateComponent = (CreateComponentFunction)lt_dlsym(hModule,
	    "CreateComponent");
#endif /* _WIN32 */

	if (pCreateComponent == NULL)
		throw runtime_error("Loadable module does not contain "
		    "CreateComponent function");

	component = Component::Ptr(pCreateComponent());
	component->SetConfig(componentConfig);
	RegisterComponent(component);
	return component;
}

/**
 * Registers a component object and starts it.
 *
 * @param component The component.
 */
void Application::RegisterComponent(const Component::Ptr& component)
{
	m_Components[component->GetName()] = component;

	component->Start();
}

/**
 * Unregisters a component object and stops it.
 *
 * @param component The component.
 */
void Application::UnregisterComponent(const Component::Ptr& component)
{
	string name = component->GetName();

	Log(LogInformation, "base", "Unloading component '" + name + "'");
	map<string, Component::Ptr>::iterator i = m_Components.find(name);
	if (i != m_Components.end())
		m_Components.erase(i);
		
	component->Stop();
}

/**
 * Finds a loaded component by name.
 *
 * @param name The name of the component.
 * @returns The component or a null pointer if the component could not be found.
 */
Component::Ptr Application::GetComponent(const string& name) const
{
	map<string, Component::Ptr>::const_iterator i = m_Components.find(name);

	if (i == m_Components.end())
		return Component::Ptr();

	return i->second;
}

/**
 * Writes a message to the application's log.
 *
 * @param severity The message severity.
 * @param facility The log facility.
 * @param message The message.
 */
void Application::Log(LogSeverity severity, const string& facility, const string& message)
{
	char timestamp[100];

	// TODO: make this configurable
	if (severity < LogInformation)
		return;

	string severityStr;
	switch (severity) {
		case LogDebug:
			severityStr = "debug";
			break;
		case LogInformation:
			severityStr = "info";
			break;
		case LogWarning:
			severityStr = "warning";
			break;
		case LogCritical:
			severityStr = "critical";
			break;
		default:
			assert(!"Invalid severity specified.");
	}

	time_t now;
	time(&now);
	tm tmnow = *localtime(&now);

	strftime(timestamp, sizeof(timestamp), "%Y/%m/%d %H:%M:%S", &tmnow);

	cout << "[" << timestamp << "] "
		 << severityStr << "/" << facility << ": "
		 << message << endl;
}

/**
 * Retrieves the directory the application's binary is contained in.
 *
 * @returns The directory.
 */
string Application::GetExeDirectory(void) const
{
	static string result;

	if (!result.empty())
		return result;

#ifndef _WIN32
	string argv0 = m_Arguments[0];

	char buffer[MAXPATHLEN];
	if (getcwd(buffer, sizeof(buffer)) == NULL)
		throw PosixException("getcwd failed", errno);
	string workingDirectory = buffer;

	if (argv0[0] != '/')
		result = workingDirectory + "/" + argv0;
	else
		result = argv0;

	if (argv0.find_first_of('/') == string::npos) {
		const char *pathEnv = getenv("PATH");
		if (pathEnv != NULL) {
			vector<string> paths;
			boost::algorithm::split(paths, pathEnv, boost::is_any_of(":"));

			bool foundPath = false;
			for (vector<string>::iterator it = paths.begin(); it != paths.end(); it++) {
				string pathTest = *it + "/" + argv0;

				if (access(pathTest.c_str(), X_OK) == 0) {
					result = pathTest;
					foundPath = true;
					break;
				}
			}

			if (!foundPath) {
				result.clear();
				throw runtime_error("Could not determine executable path.");
			}
		}
	}

	if (realpath(result.c_str(), buffer) == NULL)
		throw PosixException("realpath failed", errno);

	result = buffer;

	size_t pos = result.find_last_of('/');
	if (pos != string::npos)
		result.erase(pos);
#else /* _WIN32 */
	char FullExePath[MAXPATHLEN];

	GetModuleFileName(NULL, FullExePath, sizeof(FullExePath));

	PathRemoveFileSpec(FullExePath);

	result = FullExePath;
#endif /* _WIN32 */

	return result;
}

/**
 * Adds a directory to the component search path.
 *
 * @param componentDirectory The directory.
 */
void Application::AddComponentSearchDir(const string& componentDirectory)
{
#ifdef _WIN32
	SetDllDirectory(componentDirectory.c_str());
#else /* _WIN32 */
	lt_dladdsearchdir(componentDirectory.c_str());
#endif /* _WIN32 */
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
		m_Arguments.push_back(string(argv[i]));

	if (IsDebugging()) {
		result = Main(m_Arguments);

		Application::m_Instance.reset();
	} else {
		try {
			result = Main(m_Arguments);
		} catch (const std::exception& ex) {
			Application::m_Instance.reset();

			Application::Log(LogCritical, "base", "---");
			Application::Log(LogCritical, "base", "Exception: " + Utility::GetTypeName(ex));
			Application::Log(LogCritical, "base", "Message: " + string(ex.what()));

			return EXIT_FAILURE;
		}
	}

	return result;
}
