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
 * Constructor for the Socket class.
 */
Socket::Socket(void)
	: m_FD(INVALID_SOCKET), m_Connected(false)
{ }

/**
 * Destructor for the Socket class.
 */
Socket::~Socket(void)
{
	boost::mutex::scoped_lock lock(m_SocketMutex);
	CloseInternal(true);
}

/**
 * Starts I/O processing for this socket.
 */
void Socket::Start(void)
{
	assert(!m_ReadThread.joinable() && !m_WriteThread.joinable());
	assert(GetFD() != INVALID_SOCKET);

	m_ReadThread = thread(boost::bind(&Socket::ReadThreadProc, static_cast<Socket::Ptr>(GetSelf())));
	m_ReadThread.detach();

	m_WriteThread = thread(boost::bind(&Socket::WriteThreadProc, static_cast<Socket::Ptr>(GetSelf())));
	m_WriteThread.detach();
}

/**
 * Sets the file descriptor for this socket object.
 *
 * @param fd The file descriptor.
 */
void Socket::SetFD(SOCKET fd)
{
	/* mark the socket as non-blocking */
	if (fd != INVALID_SOCKET) {
#ifdef F_GETFL
		int flags;
		flags = fcntl(fd, F_GETFL, 0);
		if (flags < 0)
			throw_exception(PosixException("fcntl failed", errno));

		if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
			throw_exception(PosixException("fcntl failed", errno));
#else /* F_GETFL */
		unsigned long lTrue = 1;
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
	boost::mutex::scoped_lock lock(m_SocketMutex);

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

	SetConnected(false);

	closesocket(m_FD);
	m_FD = INVALID_SOCKET;

	/* nobody can possibly have a valid event subscription when the
		destructor has been called */
	if (!from_dtor)
		Event::Post(boost::bind(boost::ref(OnClosed), GetSelf()));
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
 * Processes errors that have occured for the socket.
 */
void Socket::HandleException(void)
{
	throw_exception(SocketException("select() returned fd in except fdset", GetError()));
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

void Socket::HandleReadable(void)
{ }

/**
 * Checks whether data should be written for this socket object.
 *
 * @returns true if the socket should be registered for writing, false otherwise.
 */
bool Socket::WantsToWrite(void) const
{
	return false;
}

void Socket::HandleWritable(void)
{ }

/**
 * Formats a sockaddr in a human-readable way.
 *
 * @returns A String describing the sockaddr.
 */
String Socket::GetAddressFromSockaddr(sockaddr *address, socklen_t len)
{
	char host[NI_MAXHOST];
	char service[NI_MAXSERV];

	if (getnameinfo(address, len, host, sizeof(host), service,
	    sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV) < 0)
		throw_exception(SocketException("getnameinfo() failed",
		    GetLastSocketError()));

	stringstream s;
	s << "[" << host << "]:" << service;
	return s.str();
}

/**
 * Returns a String describing the local address of the socket.
 *
 * @returns A String describing the local address.
 */
String Socket::GetClientAddress(void)
{
	boost::mutex::scoped_lock lock(m_SocketMutex);

	sockaddr_storage sin;
	socklen_t len = sizeof(sin);

	if (getsockname(GetFD(), (sockaddr *)&sin, &len) < 0)
		throw_exception(SocketException("getsockname() failed", GetError()));

	return GetAddressFromSockaddr((sockaddr *)&sin, len);
}

/**
 * Returns a String describing the peer address of the socket.
 *
 * @returns A String describing the peer address.
 */
String Socket::GetPeerAddress(void)
{
	boost::mutex::scoped_lock lock(m_SocketMutex);

	sockaddr_storage sin;
	socklen_t len = sizeof(sin);

	if (getpeername(GetFD(), (sockaddr *)&sin, &len) < 0)
		throw_exception(SocketException("getpeername() failed", GetError()));

	return GetAddressFromSockaddr((sockaddr *)&sin, len);
}

/**
 * Constructor for the SocketException class.
 *
 * @param message The error message.
 * @param errorCode The error code.
 */
SocketException::SocketException(const String& message, int errorCode)	
{
#ifdef _WIN32
	String details = Win32Exception::FormatErrorCode(errorCode);
#else /* _WIN32 */
	String details = PosixException::FormatErrorCode(errorCode);
#endif /* _WIN32 */

	String msg = message + ": " + details;
	SetMessage(msg.CStr());
}

/**
 * Read thread procedure for sockets. This function waits until the
 * socket is readable and processes inbound data.
 */
void Socket::ReadThreadProc(void)
{
	boost::mutex::scoped_lock lock(m_SocketMutex);

	for (;;) {
		fd_set readfds, exceptfds;

		FD_ZERO(&readfds);
		FD_ZERO(&exceptfds);

		int fd = GetFD();

		if (fd == INVALID_SOCKET)
			return;

		if (WantsToRead())
			FD_SET(fd, &readfds);

		FD_SET(fd, &exceptfds);

		lock.unlock();

		timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		int rc = select(fd + 1, &readfds, NULL, &exceptfds, &tv);

		lock.lock();

		if (GetFD() == INVALID_SOCKET)
			return;

		try {
			if (rc < 0)
				throw_exception(SocketException("select() failed", GetError()));

			if (FD_ISSET(fd, &readfds))
				HandleReadable();

			if (FD_ISSET(fd, &exceptfds))
				HandleException();
		} catch (...) {
			m_Exception = boost::current_exception();

			CloseInternal(false);

			break;
		}

		if (WantsToWrite())
			m_WriteCV.notify_all(); /* notify Write thread */
	}
}

/**
 * Write thread procedure for sockets. This function waits until the socket
 * is writable and processes outbound data.
 */
void Socket::WriteThreadProc(void)
{
	boost::mutex::scoped_lock lock(m_SocketMutex);

	for (;;) {
		fd_set writefds;

		FD_ZERO(&writefds);

		while (!WantsToWrite()) {
			m_WriteCV.timed_wait(lock, boost::posix_time::seconds(1));

			if (GetFD() == INVALID_SOCKET)
				return;
		}

		int fd = GetFD();

		if (fd == INVALID_SOCKET)
			return;

		FD_SET(fd, &writefds);

		lock.unlock();

		int rc = select(fd + 1, NULL, &writefds, NULL, NULL);

		lock.lock();

		if (GetFD() == INVALID_SOCKET)
			return;

		try {
			if (rc < 0)
				throw_exception(SocketException("select() failed", GetError()));

			if (FD_ISSET(fd, &writefds))
				HandleWritable();
		} catch (...) {
			m_Exception = boost::current_exception();

			CloseInternal(false);

			break;
		}
	}
}

/**
 * Sets whether the socket is fully connected.
 *
 * @param connected Whether the socket is connected
 */
void Socket::SetConnected(bool connected)
{
	m_Connected = connected;
}

/**
 * Checks whether the socket is fully connected.
 *
 * @returns true if the socket is connected, false otherwise
 */
bool Socket::IsConnected(void) const
{
	return m_Connected;
}

/**
 * Checks whether an exception is available for this socket. Should be called
 * by user-supplied handlers for the OnClosed signal.
 */
void Socket::CheckException(void)
{
	if (m_Exception)
		rethrow_exception(m_Exception);
}
