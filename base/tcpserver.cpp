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
 * Constructor for the TcpServer class.
 */
TcpServer::TcpServer(void)
{
	m_ClientFactory = boost::bind(&TcpClientFactory, RoleInbound);
}

/**
 * Sets the client factory.
 *
 * @param clientFactory The client factory function.
 */
void TcpServer::SetClientFactory(function<TcpClient::Ptr(SOCKET)> clientFactory)
{
	m_ClientFactory = clientFactory;
}

/**
 * Retrieves the client factory.
 *
 * @returns The client factory function.
 */
function<TcpClient::Ptr(SOCKET)> TcpServer::GetFactoryFunction(void) const
{
	return m_ClientFactory;
}

/**
 * Starts listening for incoming client connections.
 */
void TcpServer::Listen(void)
{
	if (listen(GetFD(), SOMAXCONN) < 0) {
		HandleSocketError(SocketException(
		    "listen() failed", GetError()));
		return;
	}
}

/**
 * Checks whether the TCP server wants to read (i.e. accept new clients).
 *
 * @returns true
 */
bool TcpServer::WantsToRead(void) const
{
	return true;
}

/**
 * Accepts a new client and creates a new client object for it
 * using the client factory function.
 */
void TcpServer::HandleReadable(void)
{
	int fd;
	sockaddr_storage addr;
	socklen_t addrlen = sizeof(addr);

	fd = accept(GetFD(), (sockaddr *)&addr, &addrlen);

	if (fd < 0) {
		HandleSocketError(SocketException(
		    "accept() failed", GetError()));
		return;
	}

	TcpClient::Ptr client = m_ClientFactory(fd);

	Event::Post(boost::bind(boost::ref(OnNewClient), GetSelf(), client));
}
