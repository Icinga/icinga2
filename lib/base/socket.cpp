/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "base/socket.h"
#include "base/objectlock.h"
#include "base/utility.h"
#include "base/exception.h"
#include <sstream>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>

using namespace icinga;

/**
 * Constructor for the Socket class.
 */
Socket::Socket(void)
	: m_FD(INVALID_SOCKET)
{ }

/**
 * Constructor for the Socket class.
 */
Socket::Socket(SOCKET fd)
	: m_FD(INVALID_SOCKET)
{
	SetFD(fd);
}

/**
 * Destructor for the Socket class.
 */
Socket::~Socket(void)
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
SOCKET Socket::GetFD(void) const
{
	ObjectLock olock(this);

	return m_FD;
}

/**
 * Closes the socket.
 */
void Socket::Close(void)
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
		BOOST_THROW_EXCEPTION(socket_error()
		    << boost::errinfo_api_function("getnameinfo")
		    << boost::errinfo_errno(errno));
#else /* _WIN32 */
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
String Socket::GetClientAddress(void)
{
	boost::mutex::scoped_lock lock(m_SocketMutex);

	sockaddr_storage sin;
	socklen_t len = sizeof(sin);

	if (getsockname(GetFD(), (sockaddr *)&sin, &len) < 0) {
#ifndef _WIN32
		BOOST_THROW_EXCEPTION(socket_error()
		    << boost::errinfo_api_function("getsockname")
		    << boost::errinfo_errno(errno));
#else /* _WIN32 */
		BOOST_THROW_EXCEPTION(socket_error()
		    << boost::errinfo_api_function("getsockname")
		    << errinfo_win32_error(WSAGetLastError()));
#endif /* _WIN32 */
	}

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

	if (getpeername(GetFD(), (sockaddr *)&sin, &len) < 0) {
#ifndef _WIN32
		BOOST_THROW_EXCEPTION(socket_error()
		    << boost::errinfo_api_function("getpeername")
		    << boost::errinfo_errno(errno));
#else /* _WIN32 */
		BOOST_THROW_EXCEPTION(socket_error()
		    << boost::errinfo_api_function("getpeername")
		    << errinfo_win32_error(WSAGetLastError()));
#endif /* _WIN32 */
	}

	return GetAddressFromSockaddr((sockaddr *)&sin, len);
}

/**
 * Starts listening for incoming client connections.
 */
void Socket::Listen(void)
{
	if (listen(GetFD(), SOMAXCONN) < 0) {
#ifndef _WIN32
		BOOST_THROW_EXCEPTION(socket_error()
		    << boost::errinfo_api_function("listen")
		    << boost::errinfo_errno(errno));
#else /* _WIN32 */
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
	int rc = send(GetFD(), (const char *)buffer, count, 0);

	if (rc < 0) {
#ifndef _WIN32
		BOOST_THROW_EXCEPTION(socket_error()
		    << boost::errinfo_api_function("send")
		    << boost::errinfo_errno(errno));
#else /* _WIN32 */
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
	int rc = recv(GetFD(), (char *)buffer, count, 0);

	if (rc < 0) {
#ifndef _WIN32
		BOOST_THROW_EXCEPTION(socket_error()
		    << boost::errinfo_api_function("recv")
		    << boost::errinfo_errno(errno));
#else /* _WIN32 */
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
Socket::Ptr Socket::Accept(void)
{
	int fd;
	sockaddr_storage addr;
	socklen_t addrlen = sizeof(addr);

	fd = accept(GetFD(), (sockaddr *)&addr, &addrlen);

	if (fd < 0) {
#ifndef _WIN32
		BOOST_THROW_EXCEPTION(socket_error()
		    << boost::errinfo_api_function("accept")
		    << boost::errinfo_errno(errno));
#else /* _WIN32 */
		BOOST_THROW_EXCEPTION(socket_error()
		    << boost::errinfo_api_function("accept")
		    << errinfo_win32_error(WSAGetLastError()));
#endif /* _WIN32 */
	}

	return make_shared<Socket>(fd);
}
