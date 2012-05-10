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

#ifndef TCPCLIENT_H
#define TCPCLIENT_H

namespace icinga
{

enum I2_BASE_API TCPClientRole
{
	RoleInbound,
	RoleOutbound
};

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

TCPClient::Ptr TCPClientFactory(TCPClientRole role);

}

#endif /* TCPCLIENT_H */
