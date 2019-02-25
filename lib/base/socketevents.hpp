/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef SOCKETEVENTS_H
#define SOCKETEVENTS_H

#include "base/i2-base.hpp"
#include "base/socket.hpp"
#include "base/stream.hpp"
#include <boost/thread/condition_variable.hpp>
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
class SocketEvents : public Stream
{
public:
	DECLARE_PTR_TYPEDEFS(SocketEvents);

	~SocketEvents();

	virtual void OnEvent(int revents);

	void Unregister();

	void ChangeEvents(int events);

	bool IsHandlingEvents() const;

	void *GetEnginePrivate() const;
	void SetEnginePrivate(void *priv);

protected:
	SocketEvents(const Socket::Ptr& socket);

private:
	int m_ID;
	SOCKET m_FD;
	bool m_Events;
	void *m_EnginePrivate;

	static int m_NextID;

	static void InitializeEngine();

	void WakeUpThread(bool wait = false);

	void Register();

	friend class SocketEventEnginePoll;
	friend class SocketEventEngineEpoll;
};

#define SOCKET_IOTHREADS 8

struct SocketEventDescriptor
{
	int Events{POLLIN};
	SocketEvents::Ptr EventInterface;
};

struct EventDescription
{
	int REvents;
	SocketEventDescriptor Descriptor;
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
	virtual void Register(SocketEvents *se) = 0;
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
	void Register(SocketEvents *se) override;
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
	virtual void Register(SocketEvents *se);
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
