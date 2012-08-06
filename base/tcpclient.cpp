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
	: m_Role(role), m_SendQueue(boost::make_shared<FIFO>()),
	  m_RecvQueue(boost::make_shared<FIFO>())
{ }

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
 * Creates a socket and connects to the specified node and service.
 *
 * @param node The node.
 * @param service The service.
 */
void TcpClient::Connect(const String& node, const String& service)
{
	m_Role = RoleOutbound;

	addrinfo hints;
	addrinfo *result;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	int rc = getaddrinfo(node.CStr(), service.CStr(), &hints, &result);

	if (rc < 0)
		throw_exception(SocketException("getaddrinfo() failed", GetLastSocketError()));

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

		if (rc >= 0) {
			SetConnected(true);
			OnConnected(GetSelf());
		}

		break;
	}

	freeaddrinfo(result);

	if (fd == INVALID_SOCKET)
		throw_exception(runtime_error("Could not create a suitable socket."));
}

void TcpClient::HandleWritable(void)
{
	int rc;
	char data[1024];
	size_t count;

	if (!IsConnected()) {
		SetConnected(true);
		Event::Post(boost::bind(boost::cref(OnConnected), GetSelf()));
	}

	for (;;) {
		{
			mutex::scoped_lock lock(m_QueueMutex);

			count = m_SendQueue->GetAvailableBytes();

			if (count == 0)
				break;

			if (count > sizeof(data))
				count = sizeof(data);

			m_SendQueue->Peek(data, count);
		}

		rc = send(GetFD(), (const char *)data, count, 0);

		if (rc <= 0)
			throw_exception(SocketException("send() failed", GetError()));

		{
			mutex::scoped_lock lock(m_QueueMutex);
			m_SendQueue->Read(NULL, rc);
		}
	}
}

/**
 * Implements IOQueue::GetAvailableBytes.
 */
size_t TcpClient::GetAvailableBytes(void) const
{
	mutex::scoped_lock lock(m_QueueMutex);

	return m_RecvQueue->GetAvailableBytes();
}

/**
 * Implements IOQueue::Peek.
 */
void TcpClient::Peek(void *buffer, size_t count)
{
	mutex::scoped_lock lock(m_QueueMutex);

	m_RecvQueue->Peek(buffer, count);
}

/**
 * Implements IOQueue::Read.
 */
void TcpClient::Read(void *buffer, size_t count)
{
	mutex::scoped_lock lock(m_QueueMutex);

	m_RecvQueue->Read(buffer, count);
}

/**
 * Implements IOQueue::Write.
 */
void TcpClient::Write(const void *buffer, size_t count)
{
	mutex::scoped_lock lock(m_QueueMutex);

	m_SendQueue->Write(buffer, count);
}

void TcpClient::HandleReadable(void)
{
	if (!IsConnected()) {
		SetConnected(true);
		Event::Post(boost::bind(boost::cref(OnConnected), GetSelf()));
	}

	for (;;) {
		char data[1024];
		int rc = recv(GetFD(), data, sizeof(data), 0);

	#ifdef _WIN32
		if (rc < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
	#else /* _WIN32 */
		if (rc < 0 && errno == EAGAIN)
	#endif /* _WIN32 */
			return;

		if (rc <= 0)
			throw_exception(SocketException("recv() failed", GetError()));

		{
			mutex::scoped_lock lock(m_QueueMutex);

			m_RecvQueue->Write(data, rc);
		}
	}

	Event::Post(boost::bind(boost::ref(OnDataAvailable), GetSelf()));
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
	{
		mutex::scoped_lock lock(m_QueueMutex);

		if (m_SendQueue->GetAvailableBytes() > 0)
			return true;
	}

	return (!IsConnected());
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
