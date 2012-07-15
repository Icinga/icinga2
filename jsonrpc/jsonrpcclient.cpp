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

#include "i2-jsonrpc.h"

using namespace icinga;

/**
 * Constructor for the JsonRpcClient class.
 *
 * @param role The role of the underlying TCP client.
 * @param sslContext SSL context for the TLS connection.
 */
JsonRpcClient::JsonRpcClient(TcpClientRole role, shared_ptr<SSL_CTX> sslContext)
	: TlsClient(role, sslContext)
{
	OnDataAvailable.connect(boost::bind(&JsonRpcClient::DataAvailableHandler, this));
}

/**
 * Sends a message to the connected peer.
 *
 * @param message The message.
 */
void JsonRpcClient::SendMessage(const MessagePart& message)
{
	Netstring::WriteStringToIOQueue(this, message.ToJsonString());
}

/**
 * Processes inbound data.
 */
void JsonRpcClient::DataAvailableHandler(void)
{
	for (;;) {
		string jsonString;
		MessagePart message;

		if (!Netstring::ReadStringFromIOQueue(this, &jsonString))
			return;

		try {
			message = MessagePart(jsonString);
			OnNewMessage(GetSelf(), message);
		} catch (const exception& ex) {
			Logger::Write(LogCritical, "jsonrpc", "Exception while processing message from JSON-RPC client: " + string(ex.what()));
		}
	}
}

/**
 * Factory function for JSON-RPC clients.
 *
 * @param fd The file descriptor.
 * @param role The role of the underlying TCP client.
 * @param sslContext SSL context for the TLS connection.
 * @returns A new JSON-RPC client.
 */
JsonRpcClient::Ptr icinga::JsonRpcClientFactory(SOCKET fd, TcpClientRole role, shared_ptr<SSL_CTX> sslContext)
{
	JsonRpcClient::Ptr client = boost::make_shared<JsonRpcClient>(role, sslContext);
	client->SetFD(fd);
	return client;
}
