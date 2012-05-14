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
 * Creates a socket.
 *
 * @param family The socket family for the new socket.
 */
void TCPSocket::MakeSocket(int family)
{
	assert(GetFD() == INVALID_SOCKET);

	int fd = socket(family, SOCK_STREAM, 0);

	if (fd == INVALID_SOCKET) {
		HandleSocketError();

		return;
	}

	SetFD(fd);
}

/**
 * Creates a socket and binds it to the specified service.
 *
 * @param service The service.
 * @param family The address family for the socket.
 */
void TCPSocket::Bind(string service, int family)
{
	Bind(string(), service, family);
}

/**
 * Creates a socket and binds it to the specified node and service.
 *
 * @param service The service.
 * @param family The address family for the socket.
 */
void TCPSocket::Bind(string node, string service, int family)
{
	addrinfo hints;
	addrinfo *result;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(node.empty() ? NULL : node.c_str(), service.c_str(), &hints, &result) < 0) {
		HandleSocketError();

		return;
	}

	int fd = INVALID_SOCKET;

	for (addrinfo *info = result; info != NULL; info = info->ai_next) {
		fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

		if (fd == INVALID_SOCKET)
			continue;

		SetFD(fd);

		const int optFalse = 0;
		setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&optFalse, sizeof(optFalse));

#ifndef _WIN32
		const int optTrue = 1;
		setsockopt(GetFD(), SOL_SOCKET, SO_REUSEADDR, (char *)&optTrue, sizeof(optTrue));
#endif /* _WIN32 */

		int rc = ::bind(fd, info->ai_addr, info->ai_addrlen);

#ifdef _WIN32
		if (rc < 0 && WSAGetLastError() != WSAEWOULDBLOCK) {
#else /* _WIN32 */
		if (rc < 0 && errno != EINPROGRESS) {
#endif /* _WIN32 */
			closesocket(fd);
			SetFD(INVALID_SOCKET);

			continue;
		}

		break;
	}

	if (fd == INVALID_SOCKET)
		HandleSocketError();

	freeaddrinfo(result);
}
