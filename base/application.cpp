#include "i2-base.h"

#ifndef _WIN32
#	include <ltdl.h>
#endif

using namespace icinga;

Application::Ptr I2_EXPORT Application::Instance;

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

Application::~Application(void)
{
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

void Application::Shutdown(void)
{
	m_ShuttingDown = true;
}

ConfigHive::Ptr Application::GetConfigHive(void) const
{
	return m_ConfigHive;
}

Component::Ptr Application::LoadComponent(const string& path,
    const ConfigObject::Ptr& componentConfig)
{
	Component::Ptr component;
	Component *(*pCreateComponent)();

	Log("Loading component '%s'", path.c_str());

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

void Application::RegisterComponent(Component::Ptr component)
{
	component->SetApplication(static_pointer_cast<Application>(shared_from_this()));
	m_Components[component->GetName()] = component;

	component->Start();
}

void Application::UnregisterComponent(Component::Ptr component)
{
	string name = component->GetName();

	Log("Unloading component '%s'", name.c_str());
	map<string, Component::Ptr>::iterator i = m_Components.find(name);
	if (i != m_Components.end()) {
		m_Components.erase(i);
		component->Stop();
	}
}

Component::Ptr Application::GetComponent(const string& name)
{
	map<string, Component::Ptr>::iterator ci = m_Components.find(name);

	if (ci == m_Components.end())
		return Component::Ptr();

	return ci->second;
}

void Application::Log(const char *format, ...)
{
	char message[512];
	va_list marker;

	va_start(marker, format);
	vsnprintf(message, sizeof(message), format, marker);
	va_end(marker);

	// TODO: log to file
	fprintf(stderr, "%s\n", message);
}

void Application::SetArguments(const vector<string>& arguments)
{
	m_Arguments = arguments;
}

const vector<string>& Application::GetArguments(void) const
{
	return m_Arguments;
}

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

void Application::AddComponentSearchDir(const string& componentDirectory)
{
#ifdef _WIN32
	SetDllDirectory(componentDirectory.c_str());
#else /* _WIN32 */
	lt_dladdsearchdir(componentDirectory.c_str());
#endif /* _WIN32 */
}

bool Application::IsDebugging(void) const
{
	return m_Debugging;
}

#ifndef _WIN32
static void ApplicationSigIntHandler(int signum)
{
	Application::Instance->Shutdown();

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_DFL;
	sigaction(SIGINT, &sa, NULL);
}
#endif /* _WIN32 */

int icinga::RunApplication(int argc, char **argv, Application *instance)
{
	int result;

	Application::Instance = Application::Ptr(instance);

#ifndef _WIN32
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = ApplicationSigIntHandler;
	sigaction(SIGINT, &sa, NULL);
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
			cerr << "---" << endl;
			cerr << "Exception: " << Utility::GetTypeName(ex) << endl;
			cerr << "Message: " << ex.GetMessage() << endl;

			return EXIT_FAILURE;
		}
	}

	return result;
}
