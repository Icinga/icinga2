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
 *
 * @ingroup base
 */
struct I2_BASE_API SocketErrorEventArgs : public EventArgs
{
	const std::exception& Exception;

	SocketErrorEventArgs(const std::exception& ex)
	    : Exception(ex) { }
};

/**
 * Base class for sockets.
 *
 * @ingroup base
 */
class I2_BASE_API Socket : public Object
{
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

protected:
	Socket(void);

	int GetError(void) const;
	static int GetLastSocketError(void);
	void HandleSocketError(const std::exception& ex);

	virtual void CloseInternal(bool from_dtor);

private:
	SOCKET m_FD; /**< The socket descriptor. */

	int ExceptionEventHandler(const EventArgs& ea);

	static string GetAddressFromSockaddr(sockaddr *address, socklen_t len);
};

/**
 * A socket exception.
 */
class SocketException : public Exception
{
public:
	SocketException(const string& message, int errorCode);
};

}

#endif /* SOCKET_H */
