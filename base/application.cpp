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

	m_ShuttingDown = false;
	m_ConfigHive = make_shared<ConfigHive>();
}

Application::~Application(void)
{
	Timer::StopAllTimers();
	Socket::CloseAllSockets();

	for (map<string, Component::Ptr>::iterator i = m_Components.begin(); i != m_Components.end(); i++) {
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

		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_ZERO(&exceptfds);

		for (list<Socket::WeakPtr>::iterator i = Socket::Sockets.begin(); i != Socket::Sockets.end(); i++) {
			Socket::Ptr socket = i->lock();

			if (socket == NULL)
				continue;

			int fd = socket->GetFD();

			if (socket->WantsToWrite())
				FD_SET(fd, &writefds);

			if (socket->WantsToRead())
				FD_SET(fd, &readfds);

			FD_SET(fd, &exceptfds);

			if (fd > nfds)
				nfds = fd;
		}

		long sleep;

		do {
			Timer::CallExpiredTimers();
			sleep = (long)(Timer::GetNextCall() - time(NULL));
		} while (sleep <= 0);

		if (m_ShuttingDown)
			break;

		timeval tv;
		tv.tv_sec = sleep;
		tv.tv_usec = 0;

		int ready;

		if (nfds == -1) {
			Sleep(tv.tv_sec * 1000 + tv.tv_usec);
			ready = 0;
		} else
			ready = select(nfds + 1, &readfds, &writefds, &exceptfds, &tv);

		if (ready < 0)
			break;
		else if (ready == 0)
			continue;

		EventArgs::Ptr ea = make_shared<EventArgs>();
		ea->Source = shared_from_this();

		list<Socket::WeakPtr>::iterator prev, i;
		for (i = Socket::Sockets.begin(); i != Socket::Sockets.end(); ) {
			prev = i;
			i++;

			Socket::Ptr socket = prev->lock();

			if (socket == NULL)
				continue;

			int fd = socket->GetFD();

			if (FD_ISSET(fd, &writefds))
				socket->OnWritable(ea);

			if (FD_ISSET(fd, &readfds))
				socket->OnReadable(ea);

			if (FD_ISSET(fd, &exceptfds))
				socket->OnException(ea);
		}
	}
}

bool Application::Daemonize(void) {
#ifndef _WIN32
	pid_t pid;
	pid_t sid;
	int fd;

	pid = fork();
	if (pid == -1) {
		return false;
	}

	if (pid) {
		fprintf(stdout, "DONE\n");
		exit(0);
	}

	fd = open("/dev/null", O_RDWR);
	if (fd) {
		if (fd != 0) {
			dup2(fd, 0);
		}

		if (fd != 1) {
			dup2(fd, 1);
		}

		if (fd != 2) {
			dup2(fd, 2);
		}

		if (fd > 2) {
			close(fd);
		}
	}

	sid = setsid();
	if (sid == -1) {
		return false;
	}
#endif

	return true;
}

void Application::Shutdown(void)
{
	m_ShuttingDown = true;
}

ConfigHive::Ptr Application::GetConfigHive(void)
{
	return m_ConfigHive;
}

Component::Ptr Application::LoadComponent(const string& path, const ConfigObject::Ptr& componentConfig)
{
	Component::Ptr component;
	Component *(*pCreateComponent)();

	Log("Loading component '%s'", path.c_str());

#ifdef _WIN32
	HMODULE hModule = LoadLibrary(path.c_str());
#else /* _WIN32 */
	lt_dlhandle hModule = 0;
	lt_dladvise advise;

	if (!lt_dladvise_init(&advise) && !lt_dladvise_local(&advise)) {
		hModule = lt_dlopenadvise(path.c_str(), advise);
	}

	lt_dladvise_destroy(&advise);
#endif /* _WIN32 */

	if (hModule == NULL)
		throw ComponentLoadException("Could not load module");

#ifdef _WIN32
	pCreateComponent = (Component *(*)())GetProcAddress(hModule, "CreateComponent");
#else /* _WIN32 */
	pCreateComponent = (Component *(*)())lt_dlsym(hModule, "CreateComponent");
#endif /* _WIN32 */

	if (pCreateComponent == NULL)
		throw ComponentLoadException("Loadable module does not contain CreateComponent function");

	component = Component::Ptr(pCreateComponent());
	component->SetApplication(static_pointer_cast<Application>(shared_from_this()));
	component->SetConfig(componentConfig);
	m_Components[component->GetName()] = component;

	component->Start();

	return component;
}

Component::Ptr Application::GetComponent(const string& name)
{
	map<string, Component::Ptr>::iterator ci = m_Components.find(name);

	if (ci == m_Components.end())
		return Component::Ptr();

	return ci->second;
}

void Application::UnloadComponent(const string& name)
{
	map<string, Component::Ptr>::iterator ci = m_Components.find(name);

	if (ci == m_Components.end())
		return;

	Log("Unloading component '%s'", name.c_str());

	Component::Ptr component = ci->second;
	component->Stop();
	m_Components.erase(ci);

	// TODO: unload DLL
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

const vector<string>& Application::GetArguments(void)
{
	return m_Arguments;
}

const string& Application::GetExeDirectory(void)
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
		throw exception(/*"getcwd() failed"*/);

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
					throw exception(/*"snprintf() failed"*/);

				if (access(PathTest, X_OK) == 0) {
					strncpy(FullExePath, PathTest, sizeof(FullExePath));

					FoundPath = true;

					break;
				}
			}

			free(PathEnv);

			if (!FoundPath)
				throw exception(/*"Could not determine executable path."*/);
		}
	}

	if ((Buf = realpath(FullExePath, NULL)) == NULL)
		throw exception(/*"realpath() failed"*/);

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

void Application::SigIntHandler(int signum)
{
	Application::Instance->Shutdown();

#ifndef _WIN32
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_DFL;
	sigaction(SIGINT, &sa, NULL);
#endif /* _WIN32 */
}

static void application_sigint_handler(int signum)
{
	Application::Instance->SigIntHandler(signum);
}

int application_main(int argc, char **argv, Application *instance)
{
	int result;

	Application::Instance = Application::Ptr(instance);

#ifndef _WIN32
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = application_sigint_handler;
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
			cout << "---" << endl;

			string klass = typeid(ex).name();

#ifdef HAVE_GCC_ABI_DEMANGLE
			int status;
			char *realname = abi::__cxa_demangle(klass.c_str(), 0, 0, &status);

			if (realname != NULL) {
				klass = string(realname);
				free(realname);
			}
#endif /* HAVE_GCC_ABI_DEMANGLE */

			cout << "Exception: " << klass << endl;
			cout << "Message: " << ex.GetMessage() << endl;

			return EXIT_FAILURE;
		}
	}

	Application::Instance.reset();

	assert(Object::ActiveObjects == 0);

	return result;
}