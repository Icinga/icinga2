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

#ifndef TCPCLIENT_H
#define TCPCLIENT_H

namespace icinga
{

/**
 * The role of a TCP client object.
 */
enum I2_BASE_API TCPClientRole
{
	RoleInbound, /**< inbound socket, i.e. one that was returned from accept() */
	RoleOutbound /**< outbound socket, i.e. one that is connect()'d to a remote socket */
};

/**
 * A TCP client connection.
 */
class I2_BASE_API TCPClient : public TCPSocket
{
private:
	TCPClientRole m_Role;

	FIFO::Ptr m_SendQueue;
	FIFO::Ptr m_RecvQueue;

	virtual int ReadableEventHandler(const EventArgs& ea);
	virtual int WritableEventHandler(const EventArgs& ea);

public:
	typedef shared_ptr<TCPClient> Ptr;
	typedef weak_ptr<TCPClient> WeakPtr;

	TCPClient(TCPClientRole role);

	TCPClientRole GetRole(void) const;

	virtual void Start(void);

	void Connect(const string& node, const string& service);

	FIFO::Ptr GetSendQueue(void);
	FIFO::Ptr GetRecvQueue(void);

	virtual bool WantsToRead(void) const;
	virtual bool WantsToWrite(void) const;

	Event<EventArgs> OnDataAvailable;
};

/**
 * Returns a new unconnected TCPClient object that has the specified
 * connection role.
 *
 * @param role The role of the new object.
 * @returns A new TCPClient object.
 */
TCPClient::Ptr TCPClientFactory(TCPClientRole role);

}

#endif /* TCPCLIENT_H */
