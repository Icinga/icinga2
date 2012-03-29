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
}

Application::~Application(void)
{
	Timer::StopAllTimers();
	Socket::CloseAllSockets();

#ifdef _WIN32
	WSACleanup();
#endif
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

void Application::Shutdown(void)
{
	m_ShuttingDown = true;
}
