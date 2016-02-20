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

#ifndef SOCKETEVENTS_H
#define SOCKETEVENTS_H

#include "base/i2-base.hpp"
#include "base/socket.hpp"

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
class I2_BASE_API SocketEvents
{
public:
	~SocketEvents(void);

	virtual void OnEvent(int revents);

	void Unregister(void);

	void ChangeEvents(int events);

	bool IsHandlingEvents(void) const;

protected:
	SocketEvents(const Socket::Ptr& socket, Object *lifesupportObject);

private:
	int m_ID;
	SOCKET m_FD;
	bool m_Events;
#ifndef __linux__
	pollfd *m_PFD;
#endif /* __linux__ */

	static int m_NextID;

	static void InitializeThread(void);
	static void ThreadProc(int tid);

	void WakeUpThread(bool wait = false);

	int GetPollEvents(void) const;

	void Register(Object *lifesupportObject);

	static int PollToEpoll(int events);
	static int EpollToPoll(int events);
};

}

#endif /* SOCKETEVENTS_H */
