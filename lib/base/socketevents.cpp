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
#include "base/application.hpp"
#include "base/scriptglobal.hpp"
#include <boost/thread/once.hpp>
#include <map>
#ifdef __linux__
#	include <sys/epoll.h>
#endif /* __linux__ */

using namespace icinga;

static boost::once_flag l_SocketIOOnceFlag = BOOST_ONCE_INIT;
static SocketEventEngine *l_SocketIOEngine;

int SocketEvents::m_NextID = 0;

void SocketEventEngine::Start(void)
{
	for (int tid = 0; tid < SOCKET_IOTHREADS; tid++) {
		Socket::SocketPair(m_EventFDs[tid]);

		Utility::SetNonBlockingSocket(m_EventFDs[tid][0]);
		Utility::SetNonBlockingSocket(m_EventFDs[tid][1]);

#ifndef _WIN32
		Utility::SetCloExec(m_EventFDs[tid][0]);
		Utility::SetCloExec(m_EventFDs[tid][1]);
#endif /* _WIN32 */

		InitializeThread(tid);

		m_Threads[tid] = boost::thread(std::bind(&SocketEventEngine::ThreadProc, this, tid));
	}
}

void SocketEventEngine::WakeUpThread(int sid, bool wait)
{
	int tid = sid % SOCKET_IOTHREADS;

	if (boost::this_thread::get_id() == m_Threads[tid].get_id())
		return;

	if (wait) {
		boost::mutex::scoped_lock lock(m_EventMutex[tid]);

		m_FDChanged[tid] = true;

		while (m_FDChanged[tid]) {
			(void) send(m_EventFDs[tid][1], "T", 1, 0);

			boost::system_time const timeout = boost::get_system_time() + boost::posix_time::milliseconds(50);
			m_CV[tid].timed_wait(lock, timeout);
		}
	} else {
		(void) send(m_EventFDs[tid][1], "T", 1, 0);
	}
}

void SocketEvents::InitializeEngine(void)
{
	String eventEngine = ScriptGlobal::Get("EventEngine", &Empty);

	if (eventEngine.IsEmpty())
#ifdef __linux__
		eventEngine = "epoll";
#else /* __linux__ */
		eventEngine = "poll";
#endif /* __linux__ */

	if (eventEngine == "poll")
		l_SocketIOEngine = new SocketEventEnginePoll();
#ifdef __linux__
	else if (eventEngine == "epoll")
		l_SocketIOEngine = new SocketEventEngineEpoll();
#endif /* __linux__ */
	else {
		Log(LogWarning, "SocketEvents")
		    << "Invalid event engine selected: " << eventEngine << " - Falling back to 'poll'";

		eventEngine = "poll";

		l_SocketIOEngine = new SocketEventEnginePoll();
	}

	l_SocketIOEngine->Start();

	ScriptGlobal::Set("EventEngine", eventEngine);
}

/**
 * Constructor for the SocketEvents class.
 */
SocketEvents::SocketEvents(const Socket::Ptr& socket, Object *lifesupportObject)
	: m_ID(m_NextID++), m_FD(socket->GetFD()), m_EnginePrivate(NULL)
{
	boost::call_once(l_SocketIOOnceFlag, &SocketEvents::InitializeEngine);

	Register(lifesupportObject);
}

SocketEvents::~SocketEvents(void)
{
	VERIFY(m_FD == INVALID_SOCKET);
}

void SocketEvents::Register(Object *lifesupportObject)
{
	l_SocketIOEngine->Register(this, lifesupportObject);
}

void SocketEvents::Unregister(void)
{
	l_SocketIOEngine->Unregister(this);
}

void SocketEvents::ChangeEvents(int events)
{
	l_SocketIOEngine->ChangeEvents(this, events);
}

boost::mutex& SocketEventEngine::GetMutex(int tid)
{
	return m_EventMutex[tid];
}

bool SocketEvents::IsHandlingEvents(void) const
{
	int tid = m_ID % SOCKET_IOTHREADS;
	boost::mutex::scoped_lock lock(l_SocketIOEngine->GetMutex(tid));
	return m_Events;
}

void SocketEvents::OnEvent(int revents)
{

}

