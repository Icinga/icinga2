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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.            *
 ******************************************************************************/

#include "i2-base.h"

using namespace icinga;

/**
 * TCPClient
 *
 * Constructor for the TCPClient class.
 *
 * @param role The role of the TCP client socket.
 */
TCPClient::TCPClient(TCPClientRole role)
{
	m_Role = role;

	m_SendQueue = make_shared<FIFO>();
	m_RecvQueue = make_shared<FIFO>();
}

/**
 * GetRole
 *
 * Retrieves the role of the socket.
 *
 * @returns The role.
 */
TCPClientRole TCPClient::GetRole(void) const
{
	return m_Role;
}

/**
 * Start
 *
 * Registers the socket and starts processing events for it.
 */
void TCPClient::Start(void)
{
	TCPSocket::Start();

	OnReadable += bind_weak(&TCPClient::ReadableEventHandler, shared_from_this());
	OnWritable += bind_weak(&TCPClient::WritableEventHandler, shared_from_this());
}

/**
 * Connect
 *
 * Creates a socket and connects to the specified node and service.
 *
 * @param node The node.
 * @param service The service.
 */
void TCPClient::Connect(const string& node, const string& service)
{
	m_Role = RoleOutbound;

	addrinfo hints;
	addrinfo *result;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	int rc = getaddrinfo(node.c_str(), service.c_str(), &hints, &result);

	if (rc < 0) {
		HandleSocketError();

		return;
	}

	int fd = INVALID_SOCKET;

	for (addrinfo *info = result; info != NULL; info = info->ai_next) {
		fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

		if (fd == INVALID_SOCKET)
			continue;

		SetFD(fd);

		rc = connect(fd, info->ai_addr, info->ai_addrlen);

#ifdef _WIN32
	if (rc < 0 && WSAGetLastError() != WSAEWOULDBLOCK)
#else /* _WIN32 */
	if (rc < 0 && errno != EINPROGRESS)
#endif /* _WIN32 */
			continue;

		break;
	}

	if (fd == INVALID_SOCKET)
		HandleSocketError();

	freeaddrinfo(result);
}

/**
 * GetSendQueue
 *
 * Retrieves the send queue for the socket.
 *
 * @returns The send queue.
 */
FIFO::Ptr TCPClient::GetSendQueue(void)
{
	return m_SendQueue;
}

/**
 * GetRecvQueue
 *
 * Retrieves the recv queue for the socket.
 *
 * @returns The recv queue.
 */
FIFO::Ptr TCPClient::GetRecvQueue(void)
{
	return m_RecvQueue;
}

/**
 * ReadableEventHandler
 *
 * Processes data that is available for this socket.
 *
 * @param - Event arguments.
 * @returns 0
 */
int TCPClient::ReadableEventHandler(const EventArgs&)
{
	int rc;

	size_t bufferSize = FIFO::BlockSize / 2;
	char *buffer = (char *)m_RecvQueue->GetWriteBuffer(&bufferSize);
	rc = recv(GetFD(), buffer, bufferSize, 0);

#ifdef _WIN32
	if (rc < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
#else /* _WIN32 */
	if (rc < 0 && errno == EAGAIN)
#endif /* _WIN32 */
		return 0;

	if (rc <= 0) {
		HandleSocketError();
		return 0;
	}

	m_RecvQueue->Write(NULL, rc);

	EventArgs dea;
	dea.Source = shared_from_this();
	OnDataAvailable(dea);

	return 0;
}

/**
 * WritableEventHandler
 *
 * Processes data that can be written for this socket.
 *
 * @param - Event arguments.
 * @returns 0
 */
int TCPClient::WritableEventHandler(const EventArgs&)
{
	int rc;

	rc = send(GetFD(), (const char *)m_SendQueue->GetReadBuffer(), m_SendQueue->GetSize(), 0);

	if (rc <= 0) {
		HandleSocketError();
		return 0;
	}

	m_SendQueue->Read(NULL, rc);

	return 0;
}

/**
 * WantsToRead
 *
 * Checks whether data should be read for this socket.
 *
 * @returns true
 */
bool TCPClient::WantsToRead(void) const
{
	return true;
}

/**
 * WantsToWrite
 *
 * Checks whether data should be written for this socket.
 *
 * @returns true if data should be written, false otherwise.
 */
bool TCPClient::WantsToWrite(void) const
{
	return (m_SendQueue->GetSize() > 0);
}

/**
 * TCPClientFactory
 *
 * Default factory function for TCP clients.
 *
 * @param role The role of the new client.
 * @returns The new client.
 */
TCPClient::Ptr icinga::TCPClientFactory(TCPClientRole role)
{
	return make_shared<TCPClient>(role);
}
