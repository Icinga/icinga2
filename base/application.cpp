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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.            *
 ******************************************************************************/

#include "i2-base.h"

#ifndef _WIN32
#	include <ltdl.h>
#endif

using namespace icinga;

Application::Ptr I2_EXPORT Application::Instance;

/**
 * Application
 *
 * Constructor for the Application class.
 */
Application::Application(void)
{
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(1, 1), &wsaData);
#else /* _WIN32 */
	lt_dlinit();
#endif /* _WIN32 */

	char *debugging = getenv("_DEBUG");
	m_Debugging = (debugging && strtol(debugging, NULL, 10) != 0);

#ifdef _WIN32
	if (IsDebuggerPresent())
		m_Debugging = true;
#endif /* _WIN32 */

	m_ShuttingDown = false;
	m_ConfigHive = make_shared<ConfigHive>();
}

/**
 * ~Application
 *
 * Destructor for the application class.
 */
Application::~Application(void)
{
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
 * RunEventLoop
 *
 * Processes events (e.g. sockets and timers).
 */
void Application::RunEventLoop(void)
{
	while (!m_ShuttingDown) {
		fd_set readfds, writefds, exceptfds;
		int nfds = -1;

		Timer::CallExpiredTimers();

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

		time_t now = time(NULL);
		time_t next = Timer::GetNextCall();
		time_t sleep = (next < now) ? 0 : (next - now);

		if (m_ShuttingDown)
			break;

		timeval tv;
		tv.tv_sec = (sleep < 0) ? 0 : (long)sleep;
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

		EventArgs ea;
		ea.Source = shared_from_this();

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
				socket->OnWritable(ea);

			fd = socket->GetFD();
			if (fd != INVALID_SOCKET && FD_ISSET(fd, &readfds))
				socket->OnReadable(ea);

			fd = socket->GetFD();
			if (fd != INVALID_SOCKET && FD_ISSET(fd, &exceptfds))
				socket->OnException(ea);
		}
	}
}

/**
 * Shutdown
 *
 * Signals the application to shut down during the next
 * execution of the event loop.
 */
void Application::Shutdown(void)
{
	m_ShuttingDown = true;
}

/**
 * GetConfigHive
 *
 * Returns the application's configuration hive.
 *
 * @returns The config hive.
 */
ConfigHive::Ptr Application::GetConfigHive(void) const
{
	return m_ConfigHive;
}

/**
 * LoadComponent
 *
 * Loads a component from a library.
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

	Log("Loading component '" + path + "'");

#ifdef _WIN32
	HMODULE hModule = LoadLibrary(path.c_str());
#else /* _WIN32 */
	lt_dlhandle hModule = lt_dlopen(path.c_str());
#endif /* _WIN32 */

	if (hModule == NULL)
		throw ComponentLoadException("Could not load module");

#ifdef _WIN32
	pCreateComponent = (CreateComponentFunction)GetProcAddress(hModule,
	    "CreateComponent");
#else /* _WIN32 */
	pCreateComponent = (CreateComponentFunction)lt_dlsym(hModule,
	    "CreateComponent");
#endif /* _WIN32 */

	if (pCreateComponent == NULL)
		throw ComponentLoadException("Loadable module does not "
		    "contain CreateComponent function");

	component = Component::Ptr(pCreateComponent());
	component->SetConfig(componentConfig);
	RegisterComponent(component);
	return component;
}

/**
 * RegisterComponent
 *
 * Registers a component object and starts it.
 *
 * @param component The component.
 */
void Application::RegisterComponent(Component::Ptr component)
{
	component->SetApplication(static_pointer_cast<Application>(shared_from_this()));
	m_Components[component->GetName()] = component;

	component->Start();
}

/**
 * UnregisterComponent
 *
 * Unregisters a component object and stops it.
 *
 * @param component The component.
 */
void Application::UnregisterComponent(Component::Ptr component)
{
	string name = component->GetName();

	Log("Unloading component '" + name + "'");
	map<string, Component::Ptr>::iterator i = m_Components.find(name);
	if (i != m_Components.end())
		m_Components.erase(i);
		
	component->Stop();
}

/**
 * GetComponent
 *
 * Finds a loaded component by name.
 *
 * @param name The name of the component.
 * @returns The component or a null pointer if the component could not be found.
 */
Component::Ptr Application::GetComponent(const string& name)
{
	map<string, Component::Ptr>::iterator ci = m_Components.find(name);

	if (ci == m_Components.end())
		return Component::Ptr();

	return ci->second;
}

/**
 * Log
 *
 * Logs a message.
 *
 * @param message The message.
 */
void Application::Log(string message)
{
	char timestamp[100];

	time_t now;
	time(&now);
	tm tmnow = *localtime(&now);

	strftime(timestamp, sizeof(timestamp), "%a %B %d %Y %H:%M:%S", &tmnow);

	cout << "[" << timestamp << "]: " << message << endl;
}

/**
 * SetArguments
 *
 * Sets the application's arguments.
 *
 * @param arguments The arguments.
 */
void Application::SetArguments(const vector<string>& arguments)
{
	m_Arguments = arguments;
}

/**
 * GetArguments
 *
 * Retrieves the application's arguments.
 *
 * @returns The arguments.
 */
const vector<string>& Application::GetArguments(void) const
{
	return m_Arguments;
}

/**
 * GetExeDirectory
 *
 * Retrieves the directory the application's binary is contained in.
 *
 * @returns The directory.
 */
string Application::GetExeDirectory(void) const
{
	static string ExePath;

	if (ExePath.length() != 0)
		return ExePath;

#ifndef _WIN32
	char Cwd[MAXPATHLEN];
	char *Buf, *PathEnv, *Directory, PathTest[MAXPATHLEN], FullExePath[MAXPATHLEN];
	bool FoundPath;

	const char *argv0 = m_Arguments[0].c_str();

	if (getcwd(Cwd, sizeof(Cwd)) == NULL)
		throw PosixException("getcwd failed", errno);

	if (argv0[0] != '/')
		snprintf(FullExePath, sizeof(FullExePath), "%s/%s", Cwd, argv0);
	else
		strncpy(FullExePath, argv0, sizeof(FullExePath));

	if (strchr(argv0, '/') == NULL) {
		PathEnv = getenv("PATH");

		if (PathEnv != NULL) {
			PathEnv = Memory::StrDup(PathEnv);

			FoundPath = false;

			for (Directory = strtok(PathEnv, ":"); Directory != NULL; Directory = strtok(NULL, ":")) {
				if (snprintf(PathTest, sizeof(PathTest), "%s/%s", Directory, argv0) < 0)
					throw PosixException("snprintf failed", errno);

				if (access(PathTest, X_OK) == 0) {
					strncpy(FullExePath, PathTest, sizeof(FullExePath));

					FoundPath = true;

					break;
				}
			}

			free(PathEnv);

			if (!FoundPath)
				throw Exception("Could not determine executable path.");
		}
	}

	if ((Buf = realpath(FullExePath, NULL)) == NULL)
		throw PosixException("realpath failed", errno);

	// remove filename
	char *LastSlash = strrchr(Buf, '/');

	if (LastSlash != NULL)
		*LastSlash = '\0';

	ExePath = string(Buf);

	free(Buf);
#else /* _WIN32 */
	char FullExePath[MAXPATHLEN];

	GetModuleFileName(NULL, FullExePath, MAXPATHLEN);

	PathRemoveFileSpec(FullExePath);

	ExePath = string(FullExePath);
#endif /* _WIN32 */

	return ExePath;
}

/**
 * AddComponentSearchDir
 *
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
 * IsDebugging
 *
 * Retrieves the debugging mode of the application.
 *
 * @returns true if the application is being debugged, false otherwise
 */
bool Application::IsDebugging(void) const
{
	return m_Debugging;
}

#ifndef _WIN32
/**
 * ApplicationSigIntHandler
 *
 * Signal handler for SIGINT.
 *
 * @param signum The signal number.
 */
static void ApplicationSigIntHandler(int signum)
{
	assert(signum == SIGINT);

	Application::Instance->Shutdown();

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_DFL;
	sigaction(SIGINT, &sa, NULL);
}
#endif /* _WIN32 */

/**
 * RunApplication
 *
 * Runs the specified application.
 *
 * @param argc The number of arguments.
 * @param argv The arguments that should be passed to the application.
 * @param instance The application instance.
 * @returns The application's exit code.
 */
int icinga::RunApplication(int argc, char **argv, Application *instance)
{
	int result;

	Application::Instance = Application::Ptr(instance);

#ifndef _WIN32
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = ApplicationSigIntHandler;
	sigaction(SIGINT, &sa, NULL);

	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL);
#endif /* _WIN32 */

	vector<string> args;

	for (int i = 0; i < argc; i++)
		args.push_back(string(argv[i]));

	Application::Instance->SetArguments(args);

	if (Application::Instance->IsDebugging()) {
		result = Application::Instance->Main(args);
	} else {
		try {
			result = Application::Instance->Main(args);
		} catch (const Exception& ex) {
			Application::Log("---");
			Application::Log("Exception: " + Utility::GetTypeName(ex));
			Application::Log("Message: " + ex.GetMessage());

			return EXIT_FAILURE;
		}
	}

	return result;
}
