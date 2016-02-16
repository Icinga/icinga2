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

struct EventDescription
{
	int REvents;
	SocketEventDescriptor Descriptor;
	Object::Ptr LifesupportReference;
};

#define SOCKET_IOTHREADS 8

static boost::once_flag l_SocketIOOnceFlag = BOOST_ONCE_INIT;
static boost::thread l_SocketIOThreads[SOCKET_IOTHREADS];
#ifdef __linux__
static SOCKET l_SocketIOPollFDs[SOCKET_IOTHREADS];
#endif /* __linux__ */
static SOCKET l_SocketIOEventFDs[SOCKET_IOTHREADS][2];
static bool l_SocketIOFDChanged[SOCKET_IOTHREADS];
static boost::mutex l_SocketIOMutex[SOCKET_IOTHREADS];
static boost::condition_variable l_SocketIOCV[SOCKET_IOTHREADS];
static std::map<SOCKET, SocketEventDescriptor> l_SocketIOSockets[SOCKET_IOTHREADS];

int SocketEvents::m_NextID = 0;

void SocketEvents::InitializeThread(void)
{
	for (int i = 0; i < SOCKET_IOTHREADS; i++) {
#ifdef __linux__
		l_SocketIOPollFDs[i] = epoll_create1(EPOLL_CLOEXEC);
#endif /* __linux__ */

		Socket::SocketPair(l_SocketIOEventFDs[i]);

		Utility::SetNonBlockingSocket(l_SocketIOEventFDs[i][0]);
		Utility::SetNonBlockingSocket(l_SocketIOEventFDs[i][1]);

#ifndef _WIN32
		Utility::SetCloExec(l_SocketIOEventFDs[i][0]);
		Utility::SetCloExec(l_SocketIOEventFDs[i][1]);
#endif /* _WIN32 */

		SocketEventDescriptor sed;
		sed.Events = POLLIN;

		l_SocketIOSockets[i][l_SocketIOEventFDs[i][0]] = sed;
		l_SocketIOFDChanged[i] = true;

		l_SocketIOThreads[i] = boost::thread(&SocketEvents::ThreadProc, i);
	}
}

void SocketEvents::ThreadProc(int tid)
{
	Utility::SetThreadName("SocketIO");

	std::vector<pollfd> pfds;
	std::vector<SocketEventDescriptor> descriptors;

	for (;;) {
		{
			boost::mutex::scoped_lock lock(l_SocketIOMutex[tid]);

			if (l_SocketIOFDChanged[tid]) {
				pfds.resize(l_SocketIOSockets[tid].size());
				descriptors.resize(l_SocketIOSockets[tid].size());

				int i = 0;

				typedef std::map<SOCKET, SocketEventDescriptor>::value_type kv_pair;

				BOOST_FOREACH(const kv_pair& desc, l_SocketIOSockets[tid]) {
					if (desc.second.EventInterface)
						desc.second.EventInterface->m_PFD = &pfds[i];

					pfds[i].fd = desc.first;
					pfds[i].events = desc.second.Events;
					descriptors[i] = desc.second;

					i++;
				}

				l_SocketIOFDChanged[tid] = false;
				l_SocketIOCV[tid].notify_all();
			}
		}

		ASSERT(!pfds.empty());

#ifdef _WIN32
		(void) WSAPoll(&pfds[0], pfds.size(), -1);
#else /* _WIN32 */
		(void) poll(&pfds[0], pfds.size(), -1);
#endif /* _WIN32 */

		std::vector<EventDescription> events;

		{
			boost::mutex::scoped_lock lock(l_SocketIOMutex[tid]);

			if (l_SocketIOFDChanged[tid])
				continue;

			for (int i = 0; i < pfds.size(); i++) {
				if ((pfds[i].revents & (POLLIN | POLLOUT | POLLHUP | POLLERR)) == 0)
					continue;

				if (pfds[i].fd == l_SocketIOEventFDs[tid][0]) {
					char buffer[512];
					if (recv(l_SocketIOEventFDs[tid][0], buffer, sizeof(buffer), 0) < 0)
						Log(LogCritical, "SocketEvents", "Read from event FD failed.");

					continue;
				}

				EventDescription event;
				event.REvents = pfds[i].revents;
				event.Descriptor = descriptors[i];
				event.LifesupportReference = event.Descriptor.LifesupportObject;
				VERIFY(event.LifesupportReference);

				events.push_back(event);
			}
		}

		BOOST_FOREACH(const EventDescription& event, events) {
			try {
				event.Descriptor.EventInterface->OnEvent(event.REvents);
			} catch (const std::exception& ex) {
				Log(LogCritical, "SocketEvents")
				    << "Exception thrown in socket I/O handler:\n"
				    << DiagnosticInformation(ex);
			} catch (...) {
				Log(LogCritical, "SocketEvents", "Exception of unknown type thrown in socket I/O handler.");
			}
		}
	}
}

void SocketEvents::WakeUpThread(bool wait)
{
	int tid = m_ID % SOCKET_IOTHREADS;

	if (boost::this_thread::get_id() == l_SocketIOThreads[tid].get_id())
		return;

	if (wait) {
		boost::mutex::scoped_lock lock(l_SocketIOMutex[tid]);

		l_SocketIOFDChanged[tid] = true;

		while (l_SocketIOFDChanged[tid]) {
			(void) send(l_SocketIOEventFDs[tid][1], "T", 1, 0);

			boost::system_time const timeout = boost::get_system_time() + boost::posix_time::milliseconds(50);
			l_SocketIOCV[tid].timed_wait(lock, timeout);
		}
	} else {
		(void) send(l_SocketIOEventFDs[tid][1], "T", 1, 0);
	}
}

/**
 * Constructor for the SocketEvents class.
 */
SocketEvents::SocketEvents(const Socket::Ptr& socket, Object *lifesupportObject)
	: m_ID(m_NextID++), m_FD(socket->GetFD()), m_PFD(NULL)
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
	int tid = m_ID % SOCKET_IOTHREADS;

	{
		boost::mutex::scoped_lock lock(l_SocketIOMutex[tid]);

		VERIFY(m_FD != INVALID_SOCKET);

		SocketEventDescriptor desc;
		desc.Events = 0;
		desc.EventInterface = this;
		desc.LifesupportObject = lifesupportObject;

		VERIFY(l_SocketIOSockets[tid].find(m_FD) == l_SocketIOSockets[tid].end());

		l_SocketIOSockets[tid][m_FD] = desc;
		l_SocketIOFDChanged[tid] = true;

		m_Events = true;
	}

	WakeUpThread(true);
}

void SocketEvents::Unregister(void)
{
	int tid = m_ID % SOCKET_IOTHREADS;

	{
		boost::mutex::scoped_lock lock(l_SocketIOMutex[tid]);

		if (m_FD == INVALID_SOCKET)
			return;

		l_SocketIOSockets[tid].erase(m_FD);
		l_SocketIOFDChanged[tid] = true;

		m_FD = INVALID_SOCKET;

		m_Events = false;
	}

	WakeUpThread(true);
}

void SocketEvents::ChangeEvents(int events)
{
	if (m_FD == INVALID_SOCKET)
		BOOST_THROW_EXCEPTION(std::runtime_error("Tried to read/write from a closed socket."));

	int tid = m_ID % SOCKET_IOTHREADS;

	{
		boost::mutex::scoped_lock lock(l_SocketIOMutex[tid]);

		std::map<SOCKET, SocketEventDescriptor>::iterator it = l_SocketIOSockets[tid].find(m_FD);

		if (it == l_SocketIOSockets[tid].end())
			return;

		it->second.Events = events;

		if (m_PFD && boost::this_thread::get_id() == l_SocketIOThreads[tid].get_id())
			m_PFD->events = events;
		else
			l_SocketIOFDChanged[tid] = true;
	}

	WakeUpThread();
}

bool SocketEvents::IsHandlingEvents(void) const
{
	int tid = m_ID % SOCKET_IOTHREADS;
	boost::mutex::scoped_lock lock(l_SocketIOMutex[tid]);
	return m_Events;
}

void SocketEvents::OnEvent(int revents)
{

}

