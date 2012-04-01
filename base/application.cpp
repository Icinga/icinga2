#include "i2-base.h"

using namespace icinga;

Application::RefType Application::Instance;

Application::Application(void)
{
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(1, 1), &wsaData);
#endif

	m_ShuttingDown = false;
	m_ConfigHive = new_object<ConfigHive>();
}

Application::~Application(void)
{
	Timer::StopAllTimers();
	Socket::CloseAllSockets();

#ifdef _WIN32
	WSACleanup();
#endif

	for (map<string, Component::RefType>::iterator i = m_Components.begin(); i != m_Components.end(); i++) {
		i->second->Stop();
	}
}

void Application::RunEventLoop(void)
{
	while (!m_ShuttingDown) {
		fd_set readfds, writefds, exceptfds;
		int nfds = -1;

		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_ZERO(&exceptfds);

		for (list<Socket::WeakRefType>::iterator i = Socket::Sockets.begin(); i != Socket::Sockets.end(); i++) {
			Socket::RefType socket = i->lock();

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

		EventArgs::RefType ea = new_object<EventArgs>();
		ea->Source = shared_from_this();

		list<Socket::WeakRefType>::iterator prev, i;
		for (i = Socket::Sockets.begin(); i != Socket::Sockets.end(); ) {
			prev = i;
			i++;

			Socket::RefType socket = prev->lock();

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

ConfigHive::RefType Application::GetConfigHive(void)
{
	return m_ConfigHive;
}

Component::RefType Application::LoadComponent(string name)
{
	Component::RefType component;
	Component *(*pCreateComponent)();

	component = GetComponent(name);

	if (component.get() != NULL)
		return component;

	Log("Loading component '%s'", name.c_str());

	ConfigObject::RefType componentConfig = m_ConfigHive->GetObject("component", name);

	if (componentConfig.get() == NULL) {
		componentConfig = new_object<ConfigObject>();
		componentConfig->SetName(name);
		componentConfig->SetType("component");
		m_ConfigHive->AddObject(componentConfig);
	}

	string path = componentConfig->GetProperty("path", name);

	HMODULE hModule = LoadLibrary(path.c_str());

	if (hModule == INVALID_HANDLE_VALUE)
		throw exception(/*"Could not load module"*/);

	pCreateComponent = (Component *(*)())GetProcAddress(hModule, "CreateComponent");

	if (pCreateComponent == NULL)
		throw exception(/*"Module does not contain CreateComponent function"*/);

	component = Component::RefType(pCreateComponent());
	component->SetApplication(static_pointer_cast<Application>(shared_from_this()));
	component->SetConfig(componentConfig);
	m_Components[component->GetName()] = component;

	component->Start();

	return component;
}

Component::RefType Application::GetComponent(string name)
{
	map<string, Component::RefType>::iterator ci = m_Components.find(name);

	if (ci == m_Components.end())
		return Component::RefType();

	return ci->second;
}

void Application::UnloadComponent(string name)
{
	map<string, Component::RefType>::iterator ci = m_Components.find(name);

	if (ci == m_Components.end())
		return;

	Log("Unloading component '%s'", name.c_str());

	Component::RefType component = ci->second;
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