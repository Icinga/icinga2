/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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
#include <map>
#ifdef __linux__
#	include <sys/epoll.h>

using namespace icinga;

void SocketEventEngineEpoll::InitializeThread(int tid)
{
	m_PollFDs[tid] = epoll_create(128);
	Utility::SetCloExec(m_PollFDs[tid]);

	SocketEventDescriptor sed;

	m_Sockets[tid][m_EventFDs[tid][0]] = sed;
	m_FDChanged[tid] = true;

	epoll_event event;
	memset(&event, 0, sizeof(event));
	event.data.fd = m_EventFDs[tid][0];
	event.events = EPOLLIN;
	epoll_ctl(m_PollFDs[tid], EPOLL_CTL_ADD, m_EventFDs[tid][0], &event);
}

int SocketEventEngineEpoll::PollToEpoll(int events)
{
	int result = 0;

	if (events & POLLIN)
		result |= EPOLLIN;

	if (events & POLLOUT)
		result |= EPOLLOUT;

	return events;
}

int SocketEventEngineEpoll::EpollToPoll(int events)
{
	int result = 0;

	if (events & EPOLLIN)
		result |= POLLIN;

	if (events & EPOLLOUT)
		result |= POLLOUT;

	return events;
}

void SocketEventEngineEpoll::ThreadProc(int tid)
{
	Utility::SetThreadName("SocketIO");

	for (;;) {
		{
			boost::mutex::scoped_lock lock(m_EventMutex[tid]);

			if (m_FDChanged[tid]) {
				m_FDChanged[tid] = false;
				m_CV[tid].notify_all();
			}
		}

		epoll_event pevents[64];
		int ready = epoll_wait(m_PollFDs[tid], pevents, sizeof(pevents) / sizeof(pevents[0]), -1);

		std::vector<EventDescription> events;

		{
			boost::mutex::scoped_lock lock(m_EventMutex[tid]);

			if (m_FDChanged[tid]) {
				m_FDChanged[tid] = false;

				continue;
			}

			for (int i = 0; i < ready; i++) {
				if (pevents[i].data.fd == m_EventFDs[tid][0]) {
					char buffer[512];
					if (recv(m_EventFDs[tid][0], buffer, sizeof(buffer), 0) < 0)
						Log(LogCritical, "SocketEvents", "Read from event FD failed.");

					continue;
				}

				if ((pevents[i].events & (EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR)) == 0)
					continue;

				EventDescription event;
				event.REvents = SocketEventEngineEpoll::EpollToPoll(pevents[i].events);
				event.Descriptor = m_Sockets[tid][pevents[i].data.fd];

				events.emplace_back(std::move(event));
			}
		}

		for (const EventDescription& event : events) {
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

void SocketEventEngineEpoll::Register(SocketEvents *se)
{
	int tid = se->m_ID % SOCKET_IOTHREADS;

	{
		boost::mutex::scoped_lock lock(m_EventMutex[tid]);

		VERIFY(se->m_FD != INVALID_SOCKET);

		SocketEventDescriptor desc;
		desc.EventInterface = se;

		VERIFY(m_Sockets[tid].find(se->m_FD) == m_Sockets[tid].end());

		m_Sockets[tid][se->m_FD] = desc;

		epoll_event event;
		memset(&event, 0, sizeof(event));
		event.data.fd = se->m_FD;
		event.events = 0;
		epoll_ctl(m_PollFDs[tid], EPOLL_CTL_ADD, se->m_FD, &event);

		se->m_Events = true;
	}
}

void SocketEventEngineEpoll::Unregister(SocketEvents *se)
{
	int tid = se->m_ID % SOCKET_IOTHREADS;

	{
		boost::mutex::scoped_lock lock(m_EventMutex[tid]);

		if (se->m_FD == INVALID_SOCKET)
			return;

		m_Sockets[tid].erase(se->m_FD);
		m_FDChanged[tid] = true;

		epoll_ctl(m_PollFDs[tid], EPOLL_CTL_DEL, se->m_FD, nullptr);

		se->m_FD = INVALID_SOCKET;
		se->m_Events = false;
	}

	WakeUpThread(tid, true);
}

void SocketEventEngineEpoll::ChangeEvents(SocketEvents *se, int events)
{
	if (se->m_FD == INVALID_SOCKET)
		BOOST_THROW_EXCEPTION(std::runtime_error("Tried to read/write from a closed socket."));

	int tid = se->m_ID % SOCKET_IOTHREADS;

	{
		boost::mutex::scoped_lock lock(m_EventMutex[tid]);

		auto it = m_Sockets[tid].find(se->m_FD);

		if (it == m_Sockets[tid].end())
			return;

		epoll_event event;
		memset(&event, 0, sizeof(event));
		event.data.fd = se->m_FD;
		event.events = SocketEventEngineEpoll::PollToEpoll(events);
		epoll_ctl(m_PollFDs[tid], EPOLL_CTL_MOD, se->m_FD, &event);
	}
}
#endif /* __linux__ */
