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

#include "i2-base.h"

using namespace icinga;

/**
 * A collection of weak pointers to Socket objects which have been
 * registered with the socket sub-system.
 */
Socket::CollectionType Socket::Sockets;

/**
 * Constructor for the Socket class.
 */
Socket::Socket(void)
{
	m_FD = INVALID_SOCKET;
}

/**
 * Destructor for the Socket class.
 */
Socket::~Socket(void)
{
	CloseInternal(true);
}

/**
 * Registers the socket and starts handling events for it.
 */
void Socket::Start(void)
{
	assert(m_FD != INVALID_SOCKET);

	OnException += bind_weak(&Socket::ExceptionEventHandler, shared_from_this());

	Sockets.push_back(static_pointer_cast<Socket>(shared_from_this()));
}

/**
 * Unregisters the sockets and stops handling events for it.
 */
void Socket::Stop(void)
{
	Sockets.remove_if(WeakPtrEqual<Socket>(this));
}

/**
 * Sets the file descriptor for this socket object.
 *
 * @param fd The file descriptor.
 */
void Socket::SetFD(SOCKET fd)
{
	unsigned long lTrue = 1;

	if (fd != INVALID_SOCKET) {
#ifdef F_GETFL
		int flags;
		flags = fcntl(fd, F_GETFL, 0);
		if (flags < 0)
			throw PosixException("fcntl failed", errno);

		if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
			throw PosixException("fcntl failed", errno);
#else /* F_GETFL */
		ioctlsocket(fd, FIONBIO, &lTrue);
#endif /* F_GETFL */
	}

	m_FD = fd;
}

/**
 * Retrieves the file descriptor for this socket object.
 *
 * @returns The file descriptor.
 */
SOCKET Socket::GetFD(void) const
{
	return m_FD;
}

/**
 * Closes the socket.
 */
void Socket::Close(void)
{
	CloseInternal(false);
}

/**
 * Closes the socket.
 *
 * @param from_dtor Whether this method was called from the destructor.
 */
void Socket::CloseInternal(bool from_dtor)
{
	if (m_FD == INVALID_SOCKET)
		return;

	closesocket(m_FD);
	m_FD = INVALID_SOCKET;

	/* nobody can possibly have a valid event subscription when the
		destructor has been called */
	if (!from_dtor) {
		Stop();

		EventArgs ea;
		ea.Source = shared_from_this();
		OnClosed(ea);
	}
}

/**
 * Retrieves the last error that occured for the socket.
 *
 * @returns An error code.
 */
int Socket::GetError(void) const
{
	int opt;
	socklen_t optlen = sizeof(opt);

	int rc = getsockopt(GetFD(), SOL_SOCKET, SO_ERROR, (char *)&opt, &optlen);

	if (rc >= 0)
		return opt;

	return 0;
}

/**
 * Retrieves the last socket error.
 *
 * @returns An error code.
 */
int Socket::GetLastSocketError(void)
{
#ifdef _WIN32
	return WSAGetLastError();
#else /* _WIN32 */
	return errno;
#endif /* _WIN32 */
}

/**
 * Handles a socket error by calling the OnError event or throwing an exception
 * when there are no observers for the OnError event.
 *
 * @param exception An exception.
 */
void Socket::HandleSocketError(const Exception& exception)
{
	if (OnError.HasObservers()) {
		SocketErrorEventArgs sea(exception);
		OnError(sea);

		Close();
	} else {
		throw exception;
	}
}

/**
 * Processes errors that have occured for the socket.
 *
 * @param - Event arguments for the socket error.
 * @returns 0
 */
int Socket::ExceptionEventHandler(const EventArgs&)
{
	HandleSocketError(SocketException(
	    "select() returned fd in except fdset", GetError()));

	return 0;
}

/**
 * Checks whether data should be read for this socket object.
 *
 * @returns true if the socket should be registered for reading, false otherwise.
 */
bool Socket::WantsToRead(void) const
{
	return false;
}

/**
 * Checks whether data should be written for this socket object.
 *
 * @returns true if the socket should be registered for writing, false otherwise.
 */
bool Socket::WantsToWrite(void) const
{
	return false;
}

/**
 * Formats a sockaddr in a human-readable way.
 *
 * @returns A string describing the sockaddr.
 */
string Socket::GetAddressFromSockaddr(sockaddr *address, socklen_t len)
{
	char host[NI_MAXHOST];
	char service[NI_MAXSERV];

	if (getnameinfo(address, len, host, sizeof(host), service, sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV) < 0)
		throw InvalidArgumentException(); /* TODO: throw proper exception */

	stringstream s;
	s << "[" << host << "]:" << service;
	return s.str();
}

/**
 * Returns a string describing the local address of the socket.
 *
 * @returns A string describing the local address.
 */
string Socket::GetClientAddress(void)
{
	sockaddr_storage sin;
	socklen_t len = sizeof(sin);

	if (getsockname(GetFD(), (sockaddr *)&sin, &len) < 0) {
		HandleSocketError(SocketException(
		    "getsockname() failed", GetError()));

		return string();
	}

	return GetAddressFromSockaddr((sockaddr *)&sin, len);
}

/**
 * Returns a string describing the peer address of the socket.
 *
 * @returns A string describing the peer address.
 */
string Socket::GetPeerAddress(void)
{
	sockaddr_storage sin;
	socklen_t len = sizeof(sin);

	if (getpeername(GetFD(), (sockaddr *)&sin, &len) < 0) {
		HandleSocketError(SocketException(
		    "getpeername() failed", GetError()));

		return string();
	}

	return GetAddressFromSockaddr((sockaddr *)&sin, len);
}

/**
 * Constructor for the SocketException class.
 *
 * @param message The error message.
 * @param errorCode The error code.
 */
SocketException::SocketException(const string& message, int errorCode)	
{
#ifdef _WIN32
	string details = Win32Exception::FormatErrorCode(errorCode);
#else /* _WIN32 */
	string details = PosixException::FormatErrorCode(errorCode);
#endif /* _WIN32 */

	string msg = message + ": " + details;
	SetMessage(msg.c_str());
}