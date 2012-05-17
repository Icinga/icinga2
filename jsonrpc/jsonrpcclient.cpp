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

JsonRpcClient::JsonRpcClient(TCPClientRole role, shared_ptr<SSL_CTX> sslContext)
    : TLSClient(role, sslContext) { }

void JsonRpcClient::Start(void)
{
	TLSClient::Start();

	OnDataAvailable += bind_weak(&JsonRpcClient::DataAvailableHandler, shared_from_this());
}

void JsonRpcClient::SendMessage(const MessagePart& message)
{
	Netstring::WriteStringToFIFO(GetSendQueue(), message.ToJsonString());
}

int JsonRpcClient::DataAvailableHandler(const EventArgs&)
{
	for (;;) {
		try {
			string jsonString;
			MessagePart message;

			if (!Netstring::ReadStringFromFIFO(GetRecvQueue(), &jsonString))
				break;

			message = MessagePart(jsonString);

			NewMessageEventArgs nea;
			nea.Source = shared_from_this();
			nea.Message = message;
			OnNewMessage(nea);
		} catch (const Exception& ex) {
			Application::Log("Exception while processing message from JSON-RPC client: " + string(ex.GetMessage()));
			Close();

			return 1;
		}
	}

	return 0;
}

TCPClient::Ptr icinga::JsonRpcClientFactory(TCPClientRole role, shared_ptr<SSL_CTX> sslContext)
{
	return make_shared<JsonRpcClient>(role, sslContext);
}
