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
 *
 * @ingroup base
 */
enum TcpClientRole
{
	RoleInbound, /**< Inbound socket, i.e. one that was returned
			  from accept(). */
	RoleOutbound /**< Outbound socket, i.e. one that is connect()'d to a
			  remote socket. */
};

/**
 * A TCP client connection.
 *
 * @ingroup base
 */
class I2_BASE_API TcpClient : public TcpSocket
{
public:
	typedef shared_ptr<TcpClient> Ptr;
	typedef weak_ptr<TcpClient> WeakPtr;

	TcpClient(TcpClientRole role);

	TcpClientRole GetRole(void) const;

	virtual void Start(void);

	void Connect(const string& node, const string& service);

	FIFO::Ptr GetSendQueue(void);
	FIFO::Ptr GetRecvQueue(void);

	virtual bool WantsToRead(void) const;
	virtual bool WantsToWrite(void) const;

	boost::signal<void (const Object::Ptr&)> OnDataAvailable;

private:
	TcpClientRole m_Role;

	FIFO::Ptr m_SendQueue;
	FIFO::Ptr m_RecvQueue;

	virtual void ReadableEventHandler(void);
	virtual void WritableEventHandler(void);
};

/**
 * Returns a new unconnected TcpClient object that has the specified
 * connection role.
 *
 * @param role The role of the new object.
 * @returns A new TcpClient object.
 */
TcpClient::Ptr TcpClientFactory(TcpClientRole role);

}

#endif /* TCPCLIENT_H */
