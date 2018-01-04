/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef SOCKETEVENTS_H
#define SOCKETEVENTS_H

#include "base/i2-base.hpp"
#include "base/socket.hpp"
#include <thread>

#ifndef _WIN32
#	include <poll.h>
#endif /* _WIN32 */

namespace icinga
{

/**
 * Socket event interface
 *
 * @ingroup base
 */
class SocketEvents
{
public:
	~SocketEvents();

	virtual void OnEvent(int revents);

	void Unregister();

	void ChangeEvents(int events);

	bool IsHandlingEvents() const;

	void *GetEnginePrivate() const;
	void SetEnginePrivate(void *priv);

protected:
	SocketEvents(const Socket::Ptr& socket, Object *lifesupportObject);

private:
	int m_ID;
	SOCKET m_FD;
	bool m_Events;
	void *m_EnginePrivate;

	static int m_NextID;

	static void InitializeEngine();

	void WakeUpThread(bool wait = false);

	void Register(Object *lifesupportObject);

	friend class SocketEventEnginePoll;
	friend class SocketEventEngineEpoll;
};

#define SOCKET_IOTHREADS 8

struct SocketEventDescriptor
{
	int Events{POLLIN};
	SocketEvents *EventInterface{nullptr};
	Object *LifesupportObject{nullptr};
};

struct EventDescription
{
	int REvents;
	SocketEventDescriptor Descriptor;
	Object::Ptr LifesupportReference;
};

class SocketEventEngine
{
public:
	void Start();

	void WakeUpThread(int sid, bool wait);

	boost::mutex& GetMutex(int tid);

protected:
	virtual void InitializeThread(int tid) = 0;
	virtual void ThreadProc(int tid) = 0;
	virtual void Register(SocketEvents *se, Object *lifesupportObject) = 0;
	virtual void Unregister(SocketEvents *se) = 0;
	virtual void ChangeEvents(SocketEvents *se, int events) = 0;

	std::thread m_Threads[SOCKET_IOTHREADS];
	SOCKET m_EventFDs[SOCKET_IOTHREADS][2];
	bool m_FDChanged[SOCKET_IOTHREADS];
	boost::mutex m_EventMutex[SOCKET_IOTHREADS];
	boost::condition_variable m_CV[SOCKET_IOTHREADS];
	std::map<SOCKET, SocketEventDescriptor> m_Sockets[SOCKET_IOTHREADS];

	friend class SocketEvents;
};

class SocketEventEnginePoll final : public SocketEventEngine
{
public:
	void Register(SocketEvents *se, Object *lifesupportObject) override;
	void Unregister(SocketEvents *se) override;
	void ChangeEvents(SocketEvents *se, int events) override;

protected:
	void InitializeThread(int tid) override;
	void ThreadProc(int tid) override;
};

#ifdef __linux__
class SocketEventEngineEpoll : public SocketEventEngine
{
public:
	virtual void Register(SocketEvents *se, Object *lifesupportObject);
	virtual void Unregister(SocketEvents *se);
	virtual void ChangeEvents(SocketEvents *se, int events);

protected:
	virtual void InitializeThread(int tid);
	virtual void ThreadProc(int tid);

private:
	SOCKET m_PollFDs[SOCKET_IOTHREADS];

	static int PollToEpoll(int events);
	static int EpollToPoll(int events);
};
#endif /* __linux__ */

}

#endif /* SOCKETEVENTS_H */
