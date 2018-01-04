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

#include "base/socket.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/logger.hpp"
#include <sstream>
#include <iostream>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <socketpair.h>

#ifndef _WIN32
#	include <poll.h>
#endif /* _WIN32 */

using namespace icinga;

/**
 * Constructor for the Socket class.
 */
Socket::Socket(SOCKET fd)
{
	SetFD(fd);
}

/**
 * Destructor for the Socket class.
 */
Socket::~Socket()
{
	Close();
}

/**
 * Sets the file descriptor for this socket object.
 *
 * @param fd The file descriptor.
 */
void Socket::SetFD(SOCKET fd)
{
	if (fd != INVALID_SOCKET) {
#ifndef _WIN32
		/* mark the socket as close-on-exec */
		Utility::SetCloExec(fd);
#endif /* _WIN32 */
	}

	ObjectLock olock(this);
	m_FD = fd;
}

/**
 * Retrieves the file descriptor for this socket object.
 *
 * @returns The file descriptor.
 */
SOCKET Socket::GetFD() const
{
	ObjectLock olock(this);

	return m_FD;
}

/**
 * Closes the socket.
 */
void Socket::Close()
{
	ObjectLock olock(this);

	if (m_FD != INVALID_SOCKET) {
		closesocket(m_FD);
		m_FD = INVALID_SOCKET;
	}
}

/**
 * Retrieves the last error that occured for the socket.
 *
 * @returns An error code.
 */
int Socket::GetError() const
{
	int opt;
	socklen_t optlen = sizeof(opt);

	int rc = getsockopt(GetFD(), SOL_SOCKET, SO_ERROR, (char *)&opt, &optlen);

	if (rc >= 0)
		return opt;

	return 0;
}

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
		sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV) < 0) {
#ifndef _WIN32
		Log(LogCritical, "Socket")
			<< "getnameinfo() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function("getnameinfo")
			<< boost::errinfo_errno(errno));
#else /* _WIN32 */
		Log(LogCritical, "Socket")
			<< "getnameinfo() failed with error code " << WSAGetLastError() << ", \"" << Utility::FormatErrorNumber(WSAGetLastError()) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function("getnameinfo")
			<< errinfo_win32_error(WSAGetLastError()));
#endif /* _WIN32 */
	}

	std::ostringstream s;
	s << "[" << host << "]:" << service;
	return s.str();
}

/**
 * Returns a String describing the local address of the socket.
 *
 * @returns A String describing the local address.
 */
String Socket::GetClientAddress()
{
	boost::mutex::scoped_lock lock(m_SocketMutex);

	sockaddr_storage sin;
	socklen_t len = sizeof(sin);

	if (getsockname(GetFD(), (sockaddr *)&sin, &len) < 0) {
#ifndef _WIN32
		Log(LogCritical, "Socket")
			<< "getsockname() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function("getsockname")
			<< boost::errinfo_errno(errno));
#else /* _WIN32 */
		Log(LogCritical, "Socket")
			<< "getsockname() failed with error code " << WSAGetLastError() << ", \"" << Utility::FormatErrorNumber(WSAGetLastError()) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function("getsockname")
			<< errinfo_win32_error(WSAGetLastError()));
#endif /* _WIN32 */
	}

	String address;
	try {
		address = GetAddressFromSockaddr((sockaddr *)&sin, len);
	} catch (const std::exception&) {
		/* already logged */
	}

	return address;
}

/**
 * Returns a String describing the peer address of the socket.
 *
 * @returns A String describing the peer address.
 */
String Socket::GetPeerAddress()
{
	boost::mutex::scoped_lock lock(m_SocketMutex);

	sockaddr_storage sin;
	socklen_t len = sizeof(sin);

	if (getpeername(GetFD(), (sockaddr *)&sin, &len) < 0) {
#ifndef _WIN32
		Log(LogCritical, "Socket")
			<< "getpeername() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function("getpeername")
			<< boost::errinfo_errno(errno));
#else /* _WIN32 */
		Log(LogCritical, "Socket")
			<< "getpeername() failed with error code " << WSAGetLastError() << ", \"" << Utility::FormatErrorNumber(WSAGetLastError()) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function("getpeername")
			<< errinfo_win32_error(WSAGetLastError()));
#endif /* _WIN32 */
	}

	String address;
	try {
		address = GetAddressFromSockaddr((sockaddr *)&sin, len);
	} catch (const std::exception&) {
		/* already logged */
	}

	return address;
}

/**
 * Starts listening for incoming client connections.
 */
void Socket::Listen()
{
	if (listen(GetFD(), SOMAXCONN) < 0) {
#ifndef _WIN32
		Log(LogCritical, "Socket")
			<< "listen() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function("listen")
			<< boost::errinfo_errno(errno));
#else /* _WIN32 */
		Log(LogCritical, "Socket")
			<< "listen() failed with error code " << WSAGetLastError() << ", \"" << Utility::FormatErrorNumber(WSAGetLastError()) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function("listen")
			<< errinfo_win32_error(WSAGetLastError()));
#endif /* _WIN32 */
	}
}

/**
 * Sends data for the socket.
 */
size_t Socket::Write(const void *buffer, size_t count)
{
	int rc;

#ifndef _WIN32
	rc = write(GetFD(), (const char *)buffer, count);
#else /* _WIN32 */
	rc = send(GetFD(), (const char *)buffer, count, 0);
#endif /* _WIN32 */

	if (rc < 0) {
#ifndef _WIN32
		Log(LogCritical, "Socket")
			<< "send() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function("send")
			<< boost::errinfo_errno(errno));
#else /* _WIN32 */
		Log(LogCritical, "Socket")
			<< "send() failed with error code " << WSAGetLastError() << ", \"" << Utility::FormatErrorNumber(WSAGetLastError()) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function("send")
			<< errinfo_win32_error(WSAGetLastError()));
#endif /* _WIN32 */
	}

	return rc;
}

/**
 * Processes data that can be written for this socket.
 */
size_t Socket::Read(void *buffer, size_t count)
{
	int rc;

#ifndef _WIN32
	rc = read(GetFD(), (char *)buffer, count);
#else /* _WIN32 */
	rc = recv(GetFD(), (char *)buffer, count, 0);
#endif /* _WIN32 */

	if (rc < 0) {
#ifndef _WIN32
		Log(LogCritical, "Socket")
			<< "recv() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function("recv")
			<< boost::errinfo_errno(errno));
#else /* _WIN32 */
		Log(LogCritical, "Socket")
			<< "recv() failed with error code " << WSAGetLastError() << ", \"" << Utility::FormatErrorNumber(WSAGetLastError()) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function("recv")
			<< errinfo_win32_error(WSAGetLastError()));
#endif /* _WIN32 */
	}

	return rc;
}

/**
 * Accepts a new client and creates a new client object for it.
 */
Socket::Ptr Socket::Accept()
{
	sockaddr_storage addr;
	socklen_t addrlen = sizeof(addr);

	SOCKET fd = accept(GetFD(), (sockaddr *)&addr, &addrlen);

#ifndef _WIN32
	if (fd < 0) {
		Log(LogCritical, "Socket")
			<< "accept() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function("accept")
			<< boost::errinfo_errno(errno));
	}
#else /* _WIN32 */
	if (fd == INVALID_SOCKET) {
		Log(LogCritical, "Socket")
			<< "accept() failed with error code " << WSAGetLastError() << ", \"" << Utility::FormatErrorNumber(WSAGetLastError()) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function("accept")
			<< errinfo_win32_error(WSAGetLastError()));
	}
#endif /* _WIN32 */

	return new Socket(fd);
}

bool Socket::Poll(bool read, bool write, struct timeval *timeout)
{
	int rc;

#ifdef _WIN32
	fd_set readfds, writefds, exceptfds;

	FD_ZERO(&readfds);
	if (read)
		FD_SET(GetFD(), &readfds);

	FD_ZERO(&writefds);
	if (write)
		FD_SET(GetFD(), &writefds);

	FD_ZERO(&exceptfds);
	FD_SET(GetFD(), &exceptfds);

	rc = select(GetFD() + 1, &readfds, &writefds, &exceptfds, timeout);

	if (rc < 0) {
		Log(LogCritical, "Socket")
			<< "select() failed with error code " << WSAGetLastError() << ", \"" << Utility::FormatErrorNumber(WSAGetLastError()) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function("select")
			<< errinfo_win32_error(WSAGetLastError()));
	}
#else /* _WIN32 */
	pollfd pfd;
	pfd.fd = GetFD();
	pfd.events = (read ? POLLIN : 0) | (write ? POLLOUT : 0);
	pfd.revents = 0;

	rc = poll(&pfd, 1, timeout ? (timeout->tv_sec + 1000 + timeout->tv_usec / 1000) : -1);

	if (rc < 0) {
		Log(LogCritical, "Socket")
			<< "poll() failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function("poll")
			<< boost::errinfo_errno(errno));
	}
#endif /* _WIN32 */

	return (rc != 0);
}

void Socket::MakeNonBlocking()
{
#ifdef _WIN32
	Utility::SetNonBlockingSocket(GetFD());
#else /* _WIN32 */
	Utility::SetNonBlocking(GetFD());
#endif /* _WIN32 */
}

void Socket::SocketPair(SOCKET s[2])
{
	if (dumb_socketpair(s, 0) < 0)
		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function("socketpair")
			<< boost::errinfo_errno(errno));
}

