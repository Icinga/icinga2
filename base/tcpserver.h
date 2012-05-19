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

#ifndef TCPSERVER_H
#define TCPSERVER_H

namespace icinga
{

/**
 * Event arguments for the "new client" event.
 *
 * @ingroup base
 */
struct I2_BASE_API NewClientEventArgs : public EventArgs
{
	TcpSocket::Ptr Client; /**< The new client object. */
};

/**
 * A TCP server that listens on a TCP port and accepts incoming
 * client connections.
 *
 * @ingroup base
 */
class I2_BASE_API TcpServer : public TcpSocket
{
private:
	int ReadableEventHandler(const EventArgs& ea);

	function<TcpClient::Ptr()> m_ClientFactory;

public:
	typedef shared_ptr<TcpServer> Ptr;
	typedef weak_ptr<TcpServer> WeakPtr;

	TcpServer(void);

	void SetClientFactory(function<TcpClient::Ptr()> function);
	function<TcpClient::Ptr()> GetFactoryFunction(void) const;

	virtual void Start();

	void Listen(void);

	Observable<NewClientEventArgs> OnNewClient;

	virtual bool WantsToRead(void) const;
};

}

#endif /* TCPSERVER_H */
