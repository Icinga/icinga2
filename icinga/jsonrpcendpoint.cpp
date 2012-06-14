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

#include "i2-icinga.h"

using namespace icinga;

string JsonRpcEndpoint::GetAddress(void) const
{
	if (!m_Client)
		return "<disconnected endpoint>";

	return m_Client->GetPeerAddress();
}

JsonRpcClient::Ptr JsonRpcEndpoint::GetClient(void)
{
	return m_Client;
}

void JsonRpcEndpoint::Connect(string node, string service, shared_ptr<SSL_CTX> sslContext)
{
	JsonRpcClient::Ptr client = make_shared<JsonRpcClient>(RoleOutbound, sslContext);
	SetClient(client);
	client->Connect(node, service);
	client->Start();
}

void JsonRpcEndpoint::SetClient(JsonRpcClient::Ptr client)
{
	m_Client = client;
	client->OnNewMessage.connect(bind(&JsonRpcEndpoint::NewMessageHandler, this, _1));
	client->OnClosed.connect(bind(&JsonRpcEndpoint::ClientClosedHandler, this, _1));
	client->OnError.connect(bind(&JsonRpcEndpoint::ClientErrorHandler, this, _1));
	client->OnVerifyCertificate.connect(bind(&JsonRpcEndpoint::VerifyCertificateHandler, this, _1));
}

bool JsonRpcEndpoint::IsLocal(void) const
{
	return false;
}

bool JsonRpcEndpoint::IsConnected(void) const
{
	return (bool)m_Client;
}

void JsonRpcEndpoint::ProcessRequest(Endpoint::Ptr sender, const RequestMessage& message)
{
	if (IsConnected()) {
		string id;
		if (message.GetID(&id))
			// TODO: remove calls after a certain timeout (and notify callers?)
			m_PendingCalls[id] = sender;

		m_Client->SendMessage(message);
	} else {
		// TODO: persist the event
	}
}

void JsonRpcEndpoint::ProcessResponse(Endpoint::Ptr sender, const ResponseMessage& message)
{
	m_Client->SendMessage(message);
}

int JsonRpcEndpoint::NewMessageHandler(const NewMessageEventArgs& nmea)
{
	const MessagePart& message = nmea.Message;
	Endpoint::Ptr sender = static_pointer_cast<Endpoint>(shared_from_this());

	if (ResponseMessage::IsResponseMessage(message)) {
		/* rather than routing the message to the right virtual
		 * endpoint we just process it here right away. */
		GetEndpointManager()->ProcessResponseMessage(sender, message);
		return 0;
	}

	string method;
	if (!message.GetProperty("method", &method))
		return 0;

	if (!HasPublication(method))
		return 0;

	RequestMessage request = message;

	string id;
	if (request.GetID(&id))
		GetEndpointManager()->SendAnycastMessage(sender, request);
	else
		GetEndpointManager()->SendMulticastMessage(sender, request);

	return 0;
}

int JsonRpcEndpoint::ClientClosedHandler(const EventArgs&)
{
	Application::Log("Lost connection to endpoint: identity=" + GetIdentity());

	m_PendingCalls.clear();

	// TODO: _only_ clear non-persistent publications/subscriptions
	// unregister ourselves if no persistent publications/subscriptions are left (use a timer for that, once we have a TTL property for the topics)
	ClearSubscriptions();
	ClearPublications();

	// remove the endpoint if there are no more subscriptions */
	if (BeginSubscriptions() == EndSubscriptions()) {
		Hold();
		GetEndpointManager()->UnregisterEndpoint(static_pointer_cast<Endpoint>(shared_from_this()));
	}

	m_Client.reset();

	// TODO: persist events, etc., for now we just disable the endpoint

	return 0;
}

int JsonRpcEndpoint::ClientErrorHandler(const SocketErrorEventArgs& ea)
{
	cerr << "Error occured for JSON-RPC socket: Message=" << ea.Exception.what() << endl;

	return 0;
}

int JsonRpcEndpoint::VerifyCertificateHandler(const VerifyCertificateEventArgs& ea)
{
	if (ea.Certificate && ea.ValidCertificate) {
		string identity = Utility::GetCertificateCN(ea.Certificate);

		if (GetIdentity().empty() && !identity.empty())
			SetIdentity(identity);
	}

	return 0;
}

void JsonRpcEndpoint::Stop(void)
{
	if (m_Client)
		m_Client->Close();
}
