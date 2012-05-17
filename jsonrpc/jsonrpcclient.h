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

#ifndef JSONRPCCLIENT_H
#define JSONRPCCLIENT_H

namespace icinga
{

struct I2_JSONRPC_API NewMessageEventArgs : public EventArgs
{
	typedef shared_ptr<NewMessageEventArgs> Ptr;
	typedef weak_ptr<NewMessageEventArgs> WeakPtr;

	icinga::MessagePart Message;
};

class I2_JSONRPC_API JsonRpcClient : public TLSClient
{
private:
	int DataAvailableHandler(const EventArgs& ea);

public:
	typedef shared_ptr<JsonRpcClient> Ptr;
	typedef weak_ptr<JsonRpcClient> WeakPtr;

	JsonRpcClient(TCPClientRole role, shared_ptr<SSL_CTX> sslContext);

	void SendMessage(const MessagePart& message);

	virtual void Start(void);

	Observable<NewMessageEventArgs> OnNewMessage;
};

TCPClient::Ptr JsonRpcClientFactory(TCPClientRole role, shared_ptr<SSL_CTX> sslContext);

}

#endif /* JSONRPCCLIENT_H */
