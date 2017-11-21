/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

using namespace icinga;

void SocketEventEnginePoll::InitializeThread(int tid)
{
	SocketEventDescriptor sed;
	sed.Events = POLLIN;

	m_Sockets[tid][m_EventFDs[tid][0]] = sed;
	m_FDChanged[tid] = true;
}

void SocketEventEnginePoll::ThreadProc(int tid)
{
	Utility::SetThreadName("SocketIO");

	std::vector<pollfd> pfds;
	std::vector<SocketEventDescriptor> descriptors;

	for (;;) {
		{
			boost::mutex::scoped_lock lock(m_EventMutex[tid]);

			if (m_FDChanged[tid]) {
				pfds.resize(m_Sockets[tid].size());
				descriptors.resize(m_Sockets[tid].size());

				int i = 0;

				typedef std::map<SOCKET, SocketEventDescriptor>::value_type kv_pair;

				for (const kv_pair& desc : m_Sockets[tid]) {
					if (desc.second.Events == 0)
						continue;

					if (desc.second.EventInterface)
						desc.second.EventInterface->m_EnginePrivate = &pfds[i];

					pfds[i].fd = desc.first;
					pfds[i].events = desc.second.Events;
					descriptors[i] = desc.second;

					i++;
				}

				pfds.resize(i);

				m_FDChanged[tid] = false;
				m_CV[tid].notify_all();
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
			boost::mutex::scoped_lock lock(m_EventMutex[tid]);

			if (m_FDChanged[tid])
				continue;

			for (std::vector<pollfd>::size_type i = 0; i < pfds.size(); i++) {
				if ((pfds[i].revents & (POLLIN | POLLOUT | POLLHUP | POLLERR)) == 0)
					continue;

				if (pfds[i].fd == m_EventFDs[tid][0]) {
					char buffer[512];
					if (recv(m_EventFDs[tid][0], buffer, sizeof(buffer), 0) < 0)
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

void SocketEventEnginePoll::Register(SocketEvents *se, Object *lifesupportObject)
{
	int tid = se->m_ID % SOCKET_IOTHREADS;

	{
		boost::mutex::scoped_lock lock(m_EventMutex[tid]);

		VERIFY(se->m_FD != INVALID_SOCKET);

		SocketEventDescriptor desc;
		desc.Events = 0;
		desc.EventInterface = se;
		desc.LifesupportObject = lifesupportObject;

		VERIFY(m_Sockets[tid].find(se->m_FD) == m_Sockets[tid].end());

		m_Sockets[tid][se->m_FD] = desc;

		m_FDChanged[tid] = true;

		se->m_Events = true;
	}

	WakeUpThread(tid, true);
}

void SocketEventEnginePoll::Unregister(SocketEvents *se)
{
	int tid = se->m_ID % SOCKET_IOTHREADS;

	{
		boost::mutex::scoped_lock lock(m_EventMutex[tid]);

		if (se->m_FD == INVALID_SOCKET)
			return;

		m_Sockets[tid].erase(se->m_FD);
		m_FDChanged[tid] = true;

		se->m_FD = INVALID_SOCKET;
		se->m_Events = false;
	}

	WakeUpThread(tid, true);
}

void SocketEventEnginePoll::ChangeEvents(SocketEvents *se, int events)
{
	if (se->m_FD == INVALID_SOCKET)
		BOOST_THROW_EXCEPTION(std::runtime_error("Tried to read/write from a closed socket."));

	int tid = se->m_ID % SOCKET_IOTHREADS;

	{
		boost::mutex::scoped_lock lock(m_EventMutex[tid]);

		auto it = m_Sockets[tid].find(se->m_FD);

		if (it == m_Sockets[tid].end())
			return;

		if (it->second.Events == events)
			return;

		it->second.Events = events;

		if (se->m_EnginePrivate && std::this_thread::get_id() == m_Threads[tid].get_id())
			((pollfd *)se->m_EnginePrivate)->events = events;
		else
			m_FDChanged[tid] = true;
	}

	WakeUpThread(tid, false);
}

