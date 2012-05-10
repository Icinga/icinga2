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
 * TCPServer
 *
 * Constructor for the TCPServer class.
 */
TCPServer::TCPServer(void)
{
	m_ClientFactory = bind(&TCPClientFactory, RoleInbound);
}

/**
 * SetClientFactory
 *
 * Sets the client factory.
 *
 * @param clientFactory The client factory function.
 */
void TCPServer::SetClientFactory(function<TCPClient::Ptr()> clientFactory)
{
	m_ClientFactory = clientFactory;
}

/**
 * GetFactoryFunction
 *
 * Retrieves the client factory.
 *
 * @returns The client factory function.
 */
function<TCPClient::Ptr()> TCPServer::GetFactoryFunction(void) const
{
	return m_ClientFactory;
}

/**
 * Start
 *
 * Registers the TCP server and starts processing events for it.
 */
void TCPServer::Start(void)
{
	TCPSocket::Start();

	OnReadable += bind_weak(&TCPServer::ReadableEventHandler, shared_from_this());
}

/**
 * Listen
 *
 * Starts listening for incoming client connections.
 */
void TCPServer::Listen(void)
{
	int rc = listen(GetFD(), SOMAXCONN);

	if (rc < 0) {
		HandleSocketError();
		return;
	}
}

/**
 * ReadableEventHandler
 *
 * Accepts a new client and creates a new client object for it
 * using the client factory function.
 *
 * @param - Event arguments.
 * @returns 0
 */
int TCPServer::ReadableEventHandler(const EventArgs&)
{
	int fd;
	sockaddr_storage addr;
	socklen_t addrlen = sizeof(addr);

	fd = accept(GetFD(), (sockaddr *)&addr, &addrlen);

	if (fd < 0) {
		HandleSocketError();
		return 0;
	}

	NewClientEventArgs nea;
	nea.Source = shared_from_this();
	nea.Client = static_pointer_cast<TCPSocket>(m_ClientFactory());
	nea.Client->SetFD(fd);
	nea.Client->Start();
	OnNewClient(nea);

	return 0;
}

/**
 * WantsToRead
 *
 * Checks whether the TCP server wants to read (i.e. accept new clients).
 *
 * @returns true
 */
bool TCPServer::WantsToRead(void) const
{
	return true;
}
