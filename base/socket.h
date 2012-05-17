/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#ifndef SOCKET_H
#define SOCKET_H

namespace icinga {

/**
 * Event arguments for socket errors.
 */
struct I2_BASE_API SocketErrorEventArgs : public EventArgs
{
	int Code; /**< The error code. */
	string Message; /**< A message describing the error. */
};

/**
 * Base class for sockets.
 */
class I2_BASE_API Socket : public Object
{
private:
	SOCKET m_FD; /**< The socket descriptor. */

	int ExceptionEventHandler(const EventArgs& ea);

	static string GetAddressFromSockaddr(sockaddr *address, socklen_t len);

protected:
	Socket(void);

	void HandleSocketError(void);
	virtual void CloseInternal(bool from_dtor);

public:
	typedef shared_ptr<Socket> Ptr;
	typedef weak_ptr<Socket> WeakPtr;

	typedef list<Socket::WeakPtr> CollectionType;

	static Socket::CollectionType Sockets;

	~Socket(void);

	void SetFD(SOCKET fd);
	SOCKET GetFD(void) const;

	Observable<EventArgs> OnReadable;
	Observable<EventArgs> OnWritable;
	Observable<EventArgs> OnException;

	Observable<SocketErrorEventArgs> OnError;
	Observable<EventArgs> OnClosed;

	virtual bool WantsToRead(void) const;
	virtual bool WantsToWrite(void) const;

	virtual void Start(void);
	virtual void Stop(void);

	void Close(void);

	string GetClientAddress(void);
	string GetPeerAddress(void);
};

}

#endif /* SOCKET_H */
