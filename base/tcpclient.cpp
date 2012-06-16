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
 * Constructor for the TcpClient class.
 *
 * @param role The role of the TCP client socket.
 */
TcpClient::TcpClient(TcpClientRole role)
{
	m_Role = role;

	m_SendQueue = boost::make_shared<FIFO>();
	m_RecvQueue = boost::make_shared<FIFO>();
}

/**
 * Retrieves the role of the socket.
 *
 * @returns The role.
 */
TcpClientRole TcpClient::GetRole(void) const
{
	return m_Role;
}

/**
 * Registers the socket and starts processing events for it.
 */
void TcpClient::Start(void)
{
	TcpSocket::Start();

	OnReadable.connect(boost::bind(&TcpClient::ReadableEventHandler, this));
	OnWritable.connect(boost::bind(&TcpClient::WritableEventHandler, this));
}

/**
 * Creates a socket and connects to the specified node and service.
 *
 * @param node The node.
 * @param service The service.
 */
void TcpClient::Connect(const string& node, const string& service)
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
		HandleSocketError(SocketException(
		    "getaddrinfo() failed", GetLastSocketError()));
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

	freeaddrinfo(result);

	if (fd == INVALID_SOCKET)
		HandleSocketError(runtime_error(
		    "Could not create a suitable socket."));
}

/**
 * Retrieves the send queue for the socket.
 *
 * @returns The send queue.
 */
FIFO::Ptr TcpClient::GetSendQueue(void)
{
	return m_SendQueue;
}

/**
 * Retrieves the recv queue for the socket.
 *
 * @returns The recv queue.
 */
FIFO::Ptr TcpClient::GetRecvQueue(void)
{
	return m_RecvQueue;
}

/**
 * Processes data that is available for this socket.
 */
void TcpClient::ReadableEventHandler(void)
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
		return;

	if (rc <= 0) {
		HandleSocketError(SocketException("recv() failed", GetError()));
		return;
	}

	m_RecvQueue->Write(NULL, rc);

	OnDataAvailable(GetSelf());
}

/**
 * Processes data that can be written for this socket.
 */
void TcpClient::WritableEventHandler(void)
{
	int rc;

	rc = send(GetFD(), (const char *)m_SendQueue->GetReadBuffer(), m_SendQueue->GetSize(), 0);

	if (rc <= 0) {
		HandleSocketError(SocketException("send() failed", GetError()));
		return;
	}

	m_SendQueue->Read(NULL, rc);
}

/**
 * Checks whether data should be read for this socket.
 *
 * @returns true
 */
bool TcpClient::WantsToRead(void) const
{
	return true;
}

/**
 * Checks whether data should be written for this socket.
 *
 * @returns true if data should be written, false otherwise.
 */
bool TcpClient::WantsToWrite(void) const
{
	return (m_SendQueue->GetSize() > 0);
}

/**
 * Default factory function for TCP clients.
 *
 * @param role The role of the new client.
 * @returns The new client.
 */
TcpClient::Ptr icinga::TcpClientFactory(TcpClientRole role)
{
	return boost::make_shared<TcpClient>(role);
}
