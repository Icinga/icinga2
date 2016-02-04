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
#ifdef __linux__
#	include <sys/epoll.h>
#endif /* __linux__ */

using namespace icinga;

struct SocketEventDescriptor
{
#ifndef __linux__
	int Events;
#endif /* __linux__ */
	SocketEvents *EventInterface;
	Object *LifesupportObject;

	SocketEventDescriptor(void)
		:
#ifndef __linux__
		Events(POLLIN),
#endif /* __linux__ */
		EventInterface(NULL), LifesupportObject(NULL)
	{ }
};

struct EventDescription
{
	int REvents;
	SocketEventDescriptor Descriptor;
	Object::Ptr LifesupportReference;
};

#define IOTHREADS 8

static boost::once_flag l_SocketIOOnceFlag = BOOST_ONCE_INIT;
static boost::thread l_SocketIOThreads[IOTHREADS];
#ifdef __linux__
static SOCKET l_SocketIOPollFDs[IOTHREADS];
#endif /* __linux__ */
static SOCKET l_SocketIOEventFDs[IOTHREADS][2];
static bool l_SocketIOFDChanged[IOTHREADS];
static boost::mutex l_SocketIOMutex[IOTHREADS];
static boost::condition_variable l_SocketIOCV[IOTHREADS];
static std::map<SOCKET, SocketEventDescriptor> l_SocketIOSockets[IOTHREADS];

int SocketEvents::m_NextID = 0;

void SocketEvents::InitializeThread(void)
{
	for (int i = 0; i < IOTHREADS; i++) {
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
#ifndef __linux__
		sed.Events = POLLIN;
#endif /* __linux__ */

		l_SocketIOSockets[i][l_SocketIOEventFDs[i][0]] = sed;
		l_SocketIOFDChanged[i] = true;

#ifdef __linux__
		epoll_event event;
		memset(&event, 0, sizeof(event));
		event.data.fd = l_SocketIOEventFDs[i][0];
		event.events = EPOLLIN;
		epoll_ctl(l_SocketIOPollFDs[i], EPOLL_CTL_ADD, l_SocketIOEventFDs[i][0], &event);
#endif /* __linux__ */

		l_SocketIOThreads[i] = boost::thread(&SocketEvents::ThreadProc, i);
	}
}

#ifdef __linux__
int SocketEvents::PollToEpoll(int events)
{
	int result = 0;

	if (events & POLLIN)
		result |= EPOLLIN;

	if (events & POLLOUT)
		result |= EPOLLOUT;

	return events;
}

int SocketEvents::EpollToPoll(int events)
{
	int result = 0;

	if (events & EPOLLIN)
		result |= POLLIN;

	if (events & EPOLLOUT)
		result |= POLLOUT;

	return events;
}
#endif /* __linux__ */

void SocketEvents::ThreadProc(int tid)
{
	Utility::SetThreadName("SocketIO");

#ifndef __linux__
	std::vector<pollfd> pfds;
	std::vector<SocketEventDescriptor> descriptors;
#endif /* __linux__ */

	for (;;) {
		{
			boost::mutex::scoped_lock lock(l_SocketIOMutex[tid]);

			if (l_SocketIOFDChanged[tid]) {
#ifndef __linux__
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
#endif /* __linux__ */

				l_SocketIOFDChanged[tid] = false;
				l_SocketIOCV[tid].notify_all();
			}
		}

#ifndef __linux__
		ASSERT(!pfds.empty());
#endif /* __linux__ */

#ifdef __linux__
		epoll_event pevents[64];
		int ready = epoll_wait(l_SocketIOPollFDs[tid], pevents, sizeof(pevents) / sizeof(pevents[0]), -1);
#elif _WIN32
		(void) WSAPoll(&pfds[0], pfds.size(), -1);
#else /* _WIN32 */
		(void) poll(&pfds[0], pfds.size(), -1);
#endif /* _WIN32 */

		std::vector<EventDescription> events;

		{
			boost::mutex::scoped_lock lock(l_SocketIOMutex[tid]);

			if (l_SocketIOFDChanged[tid]) {
#ifdef __linux__
				l_SocketIOFDChanged[tid] = false;
#endif /* __linux__ */

				continue;
			}

#ifdef __linux__
			for (int i = 0; i < ready; i++) {
				if (pevents[i].data.fd == l_SocketIOEventFDs[tid][0]) {
					char buffer[512];
					if (recv(l_SocketIOEventFDs[tid][0], buffer, sizeof(buffer), 0) < 0)
						Log(LogCritical, "SocketEvents", "Read from event FD failed.");

					continue;
				}

				if ((pevents[i].events & (EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR)) == 0)
					continue;

				EventDescription event;
				event.REvents = EpollToPoll(pevents[i].events);
				event.Descriptor = l_SocketIOSockets[tid][pevents[i].data.fd];
				event.LifesupportReference = event.Descriptor.LifesupportObject;
				VERIFY(event.LifesupportReference);

				events.push_back(event);
			}
#else /* __linux__ */
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
#endif /* __linux__ */
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
	int tid = m_ID % IOTHREADS;

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
	: m_ID(m_NextID++), m_FD(socket->GetFD())
#ifndef __linux__
	  , m_PFD(NULL)
#endif /* __linux__ */
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
	int tid = m_ID % IOTHREADS;

	{
		boost::mutex::scoped_lock lock(l_SocketIOMutex[tid]);

		VERIFY(m_FD != INVALID_SOCKET);

		SocketEventDescriptor desc;
#ifndef __linux__
		desc.Events = 0;
#endif /* __linux__ */
		desc.EventInterface = this;
		desc.LifesupportObject = lifesupportObject;

		VERIFY(l_SocketIOSockets[tid].find(m_FD) == l_SocketIOSockets[tid].end());

		l_SocketIOSockets[tid][m_FD] = desc;

#ifdef __linux__
		epoll_event event;
		memset(&event, 0, sizeof(event));
		event.data.fd = m_FD;
		event.events = 0;
		epoll_ctl(l_SocketIOPollFDs[tid], EPOLL_CTL_ADD, m_FD, &event);
#else /* __linux__ */
		l_SocketIOFDChanged[tid] = true;
#endif /* __linux__ */

		m_Events = true;
	}

#ifndef __linux__
	WakeUpThread(true);
#endif /* __linux__ */
}

void SocketEvents::Unregister(void)
{
	int tid = m_ID % IOTHREADS;

	{
		boost::mutex::scoped_lock lock(l_SocketIOMutex[tid]);

		if (m_FD == INVALID_SOCKET)
			return;

		l_SocketIOSockets[tid].erase(m_FD);
		l_SocketIOFDChanged[tid] = true;

#ifdef __linux__
		epoll_ctl(l_SocketIOPollFDs[tid], EPOLL_CTL_DEL, m_FD, NULL);
#endif /* __linux__ */

		m_FD = INVALID_SOCKET;
		m_Events = false;
	}

	WakeUpThread(true);
}

void SocketEvents::ChangeEvents(int events)
{
	if (m_FD == INVALID_SOCKET)
		BOOST_THROW_EXCEPTION(std::runtime_error("Tried to read/write from a closed socket."));

	int tid = m_ID % IOTHREADS;

#ifdef __linux__
	epoll_event event;
	memset(&event, 0, sizeof(event));
	event.data.fd = m_FD;
	event.events = PollToEpoll(events);
	epoll_ctl(l_SocketIOPollFDs[tid], EPOLL_CTL_MOD, m_FD, &event);
#else /* __linux__ */
	{
		boost::mutex::scoped_lock lock(l_SocketIOMutex[tid]);

		std::map<SOCKET, SocketEventDescriptor>::iterator it = l_SocketIOSockets[tid].find(m_FD);

		if (it == l_SocketIOSockets[tid].end())
			return;

		if (it->second.Events == events)
			return;

		it->second.Events = events;

		if (m_PFD && boost::this_thread::get_id() == l_SocketIOThreads[tid].get_id())
			m_PFD->events = events;
		else
			l_SocketIOFDChanged[tid] = true;
	}

	WakeUpThread();
#endif /* __linux__ */
}

bool SocketEvents::IsHandlingEvents(void) const
{
	int tid = m_ID % IOTHREADS;
	boost::mutex::scoped_lock lock(l_SocketIOMutex[tid]);
	return m_Events;
}

void SocketEvents::OnEvent(int revents)
{

}

