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

#ifndef IDOSOCKET_H
#define IDOSOCKET_H

#include "i2-compatido.h"

namespace icinga
{

/**
 * An IDO socket client.
 *
 * @ingroup compatido
 */
class IdoSocket : public TcpClient
{
public:
	typedef shared_ptr<IdoSocket> Ptr;
	typedef weak_ptr<IdoSocket> WeakPtr;

	IdoSocket(TcpClientRole role);

	void SetSocketType(bool);
	bool GetSocketType(void);

	void SendMessage(const String& message);

	void SetReconnect(bool reconnect);
	bool GetReconnect(void);

	boost::signal<void (const IdoSocket::Ptr&, const stringstream&)> OnNewMessage;

        boost::signal<void (const IdoSocket::Ptr&)> OnConnected;
	boost::signal<void (const IdoSocket::Ptr&)> OnDisconnected;

private:
	void DataAvailableHandler(void);
	void ClientClosedHandler(void);

	bool m_Reconnect;
	bool m_SocketType;

	friend IdoSocket::Ptr IdoSocketFactory(SOCKET fd, TcpClientRole role);
};

IdoSocket::Ptr IdoSocketFactory(SOCKET fd, TcpClientRole role);

}

#endif /* JSONRPCCLIENT_H */
