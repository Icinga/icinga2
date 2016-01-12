/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "base/socketevents.hpp"
#include "base/exception.hpp"
#include "base/logger.hpp"
#include <boost/thread/once.hpp>
#include <boost/foreach.hpp>
#include <map>

#ifndef _WIN32
#	include <poll.h>
#endif /* _WIN32 */

using namespace icinga;

struct SocketEventDescriptor
{
	int Events;
	SocketEvents *EventInterface;
	Object *LifesupportObject;

	SocketEventDescriptor(void)
		: Events(0), EventInterface(NULL), LifesupportObject(NULL)
	{ }
};

static boost::thread l_SocketIOThread;
static boost::once_flag l_SocketIOOnceFlag = BOOST_ONCE_INIT;
static SOCKET l_SocketIOEventFDs[2];
static boost::mutex l_SocketIOMutex;
static boost::condition_variable l_SocketIOCV;
static bool l_SocketIOFDChanged;
static std::map<SOCKET, SocketEventDescriptor> l_SocketIOSockets;

void SocketEvents::InitializeThread(void)
{
	Socket::SocketPair(l_SocketIOEventFDs);

	Utility::SetNonBlockingSocket(l_SocketIOEventFDs[0]);
	Utility::SetNonBlockingSocket(l_SocketIOEventFDs[1]);

#ifndef _WIN32
	Utility::SetCloExec(l_SocketIOEventFDs[0]);
	Utility::SetCloExec(l_SocketIOEventFDs[1]);
#endif /* _WIN32 */

	SocketEventDescriptor sed;
	sed.Events = POLLIN;

	l_SocketIOSockets[l_SocketIOEventFDs[0]] = sed;

	l_SocketIOThread = boost::thread(&SocketEvents::ThreadProc);
}

void SocketEvents::ThreadProc(void)
{
	Utility::SetThreadName("SocketIO");

	for (;;) {
		pollfd *pfds;
		int pfdcount;

		typedef std::map<SOCKET, SocketEventDescriptor>::value_type SocketDesc;

		{
			boost::mutex::scoped_lock lock(l_SocketIOMutex);

			pfdcount = l_SocketIOSockets.size();
			pfds  = new pollfd[pfdcount];

			int i = 0;

			BOOST_FOREACH(const SocketDesc& desc, l_SocketIOSockets) {
				pfds[i].fd = desc.first;
				pfds[i].events = desc.second.Events;
				pfds[i].revents = 0;

				i++;
			}
		}

#ifdef _WIN32
		(void) WSAPoll(pfds, pfdcount, -1);
#else /* _WIN32 */
		(void) poll(pfds, pfdcount, -1);
#endif /* _WIN32 */

		{
			boost::mutex::scoped_lock lock(l_SocketIOMutex);

			if (l_SocketIOFDChanged) {
				l_SocketIOFDChanged = false;
				l_SocketIOCV.notify_all();
				delete [] pfds;
				continue;
			}
		}

		for (int i = 0; i < pfdcount; i++) {
			if ((pfds[i].revents & (POLLIN | POLLOUT | POLLHUP | POLLERR)) == 0)
				continue;

			if (pfds[i].fd == l_SocketIOEventFDs[0]) {
				char buffer[512];
				if (recv(l_SocketIOEventFDs[0], buffer, sizeof(buffer), 0) < 0)
					Log(LogCritical, "SocketEvents", "Read from event FD failed.");

				continue;
			}

			SocketEventDescriptor desc;
			Object::Ptr ltref;

			{
				boost::mutex::scoped_lock lock(l_SocketIOMutex);

				std::map<SOCKET, SocketEventDescriptor>::const_iterator it = l_SocketIOSockets.find(pfds[i].fd);

				if (it == l_SocketIOSockets.end())
					continue;

				desc = it->second;

				/* We must hold a ref-counted reference to the event object to keep it alive. */
				ltref = desc.LifesupportObject;
				VERIFY(ltref);
			}

			try {
				desc.EventInterface->OnEvent(pfds[i].revents);
			} catch (const std::exception& ex) {
				Log(LogCritical, "SocketEvents")
				    << "Exception thrown in socket I/O handler:\n"
				    << DiagnosticInformation(ex);
			} catch (...) {
				Log(LogCritical, "SocketEvents", "Exception of unknown type thrown in socket I/O handler.");
			}
		}

		delete [] pfds;
	}
}

void SocketEvents::WakeUpThread(bool wait)
{
	if (wait) {
		if (boost::this_thread::get_id() != l_SocketIOThread.get_id()) {
			boost::mutex::scoped_lock lock(l_SocketIOMutex);

			l_SocketIOFDChanged = true;

			while (l_SocketIOFDChanged) {
				(void) send(l_SocketIOEventFDs[1], "T", 1, 0);

				boost::system_time const timeout = boost::get_system_time() + boost::posix_time::milliseconds(50);
				l_SocketIOCV.timed_wait(lock, timeout);
			}
		}
	} else {
		(void) send(l_SocketIOEventFDs[1], "T", 1, 0);
	}
}

/**
 * Constructor for the SocketEvents class.
 */
SocketEvents::SocketEvents(const Socket::Ptr& socket, Object *lifesupportObject)
	: m_FD(socket->GetFD())
{
	boost::call_once(l_SocketIOOnceFlag, &SocketEvents::InitializeThread);

	Register(lifesupportObject);
}

SocketEvents::~SocketEvents(void)
{
	VERIFY(m_FD == INVALID_SOCKET);
}

void SocketEvents::Register(Object *lifesupportObject)
{
	boost::mutex::scoped_lock lock(l_SocketIOMutex);

	VERIFY(m_FD != INVALID_SOCKET);

	SocketEventDescriptor desc;
	desc.Events = 0;
	desc.EventInterface = this;
	desc.LifesupportObject = lifesupportObject;

	VERIFY(l_SocketIOSockets.find(m_FD) == l_SocketIOSockets.end());

	l_SocketIOSockets[m_FD] = desc;

	m_Events = true;

	/* There's no need to wake up the I/O thread here. */
}

void SocketEvents::Unregister(void)
{
	{
		boost::mutex::scoped_lock lock(l_SocketIOMutex);

		if (m_FD == INVALID_SOCKET)
			return;

		l_SocketIOSockets.erase(m_FD);
		m_FD = INVALID_SOCKET;

		m_Events = false;
	}

	WakeUpThread(true);
}

void SocketEvents::ChangeEvents(int events)
{
	{
		boost::mutex::scoped_lock lock(l_SocketIOMutex);

		if (m_FD == INVALID_SOCKET)
			BOOST_THROW_EXCEPTION(std::runtime_error("Tried to read/write from a closed socket."));

		std::map<SOCKET, SocketEventDescriptor>::iterator it = l_SocketIOSockets.find(m_FD);

		if (it == l_SocketIOSockets.end())
			return;

		it->second.Events = events;
	}

	WakeUpThread();
}

bool SocketEvents::IsHandlingEvents(void) const
{
	boost::mutex::scoped_lock lock(l_SocketIOMutex);
	return m_Events;
}

void SocketEvents::OnEvent(int revents)
{

}

