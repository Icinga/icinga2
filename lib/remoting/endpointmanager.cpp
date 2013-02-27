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

#include "i2-remoting.h"

using namespace icinga;

/**
 * Constructor for the EndpointManager class.
 */
EndpointManager::EndpointManager(void)
	: m_NextMessageID(0)
{
	m_RequestTimer = boost::make_shared<Timer>();
	m_RequestTimer->OnTimerExpired.connect(boost::bind(&EndpointManager::RequestTimerHandler, this));
	m_RequestTimer->SetInterval(5);
	m_RequestTimer->Start();

	m_SubscriptionTimer = boost::make_shared<Timer>();
	m_SubscriptionTimer->OnTimerExpired.connect(boost::bind(&EndpointManager::SubscriptionTimerHandler, this));
	m_SubscriptionTimer->SetInterval(5);
	m_SubscriptionTimer->Start();

	m_ReconnectTimer = boost::make_shared<Timer>();
	m_ReconnectTimer->OnTimerExpired.connect(boost::bind(&EndpointManager::ReconnectTimerHandler, this));
	m_ReconnectTimer->SetInterval(5);
	m_ReconnectTimer->Start();
}

/**
 * Sets the SSL context.
 *
 * @param sslContext The new SSL context.
 */
void EndpointManager::SetSSLContext(const shared_ptr<SSL_CTX>& sslContext)
{
	m_SSLContext = sslContext;
}

/**
 * Retrieves the SSL context.
 *
 * @returns The SSL context.
 */
shared_ptr<SSL_CTX> EndpointManager::GetSSLContext(void) const
{
	return m_SSLContext;
}

/**
 * Sets the identity of the endpoint manager. This identity is used when
 * connecting to remote peers.
 *
 * @param identity The new identity.
 */
void EndpointManager::SetIdentity(const String& identity)
{
	m_Identity = identity;

	if (m_Endpoint)
		m_Endpoint->Unregister();

	DynamicObject::Ptr object = DynamicObject::GetObject("Endpoint", identity);

	if (object)
		m_Endpoint = dynamic_pointer_cast<Endpoint>(object);
	else
		m_Endpoint = Endpoint::MakeEndpoint(identity, true, true);
}

/**
 * Retrieves the identity for the endpoint manager.
 *
 * @returns The identity.
 */
String EndpointManager::GetIdentity(void) const
{
	return m_Identity;
}

/**
 * Creates a new JSON-RPC listener on the specified port.
 *
 * @param service The port to listen on.
 */
void EndpointManager::AddListener(const String& service)
{
	shared_ptr<SSL_CTX> sslContext = GetSSLContext();

	if (!sslContext)
		BOOST_THROW_EXCEPTION(logic_error("SSL context is required for AddListener()"));

	stringstream s;
	s << "Adding new listener: port " << service;
	Logger::Write(LogInformation, "icinga", s.str());

	TcpSocket::Ptr server = boost::make_shared<TcpSocket>();

	m_Servers.insert(server);
	server->OnNewClient.connect(boost::bind(&EndpointManager::NewClientHandler,
	   this, _2, TlsRoleServer));

	server->Bind(service, AF_INET6);
	server->Listen();
	server->Start();
}

/**
 * Creates a new JSON-RPC client and connects to the specified host and port.
 *
 * @param node The remote host.
 * @param service The remote port.
 */
void EndpointManager::AddConnection(const String& node, const String& service) {
	shared_ptr<SSL_CTX> sslContext = GetSSLContext();

	if (!sslContext)
		BOOST_THROW_EXCEPTION(logic_error("SSL context is required for AddConnection()"));

	TcpSocket::Ptr client = boost::make_shared<TcpSocket>();
	client->Connect(node, service);
	NewClientHandler(client, TlsRoleClient);
}

/**
 * Processes a new client connection.
 *
 * @param client The new client.
 */
void EndpointManager::NewClientHandler(const Socket::Ptr& client, TlsRole role)
{
	String peerAddress = client->GetPeerAddress();
	TlsStream::Ptr tlsStream = boost::make_shared<TlsStream>(client, role, GetSSLContext());
	tlsStream->Start();

	m_PendingClients.insert(tlsStream);
	tlsStream->OnConnected.connect(boost::bind(&EndpointManager::ClientConnectedHandler, this, _1, peerAddress));
	tlsStream->OnClosed.connect(boost::bind(&EndpointManager::ClientClosedHandler, this, _1));

	client->Start();
}

void EndpointManager::ClientConnectedHandler(const Stream::Ptr& client, const String& peerAddress)
{
	TlsStream::Ptr tlsStream = static_pointer_cast<TlsStream>(client);
	JsonRpcConnection::Ptr jclient = boost::make_shared<JsonRpcConnection>(tlsStream);

	m_PendingClients.erase(tlsStream);

	shared_ptr<X509> cert = tlsStream->GetPeerCertificate();
	String identity = Utility::GetCertificateCN(cert);

	Logger::Write(LogInformation, "icinga", "New client connection at " + peerAddress + " for identity '" + identity + "'");

	Endpoint::Ptr endpoint = Endpoint::GetByName(identity);

	if (!endpoint)
		endpoint = Endpoint::MakeEndpoint(identity, true);

	endpoint->SetClient(jclient);
}

void EndpointManager::ClientClosedHandler(const Stream::Ptr& client)
{
	TlsStream::Ptr tlsStream = static_pointer_cast<TlsStream>(client);
	m_PendingClients.erase(tlsStream);
}

/**
 * Sends an anonymous unicast message to the specified recipient.
 *
 * @param recipient The recipient of the message.
 * @param message The message.
 */
void EndpointManager::SendUnicastMessage(const Endpoint::Ptr& recipient,
    const MessagePart& message)
{
	SendUnicastMessage(Endpoint::Ptr(), recipient, message);
}

/**
 * Sends a unicast message to the specified recipient.
 *
 * @param sender The sender of the message.
 * @param recipient The recipient of the message.
 * @param message The message.
 */
void EndpointManager::SendUnicastMessage(const Endpoint::Ptr& sender,
    const Endpoint::Ptr& recipient, const MessagePart& message)
{
	/* don't forward messages between non-local endpoints, assume that
	 * anonymous senders (sender == null) are local */
	if ((sender && !sender->IsLocal()) && !recipient->IsLocal())
		return;

	if (ResponseMessage::IsResponseMessage(message))
		recipient->ProcessResponse(sender, message);
	else
		recipient->ProcessRequest(sender, message);
}

/**
 * Sends a message to exactly one recipient out of all recipients who have a
 * subscription for the message's topic.
 *
 * @param sender The sender of the message.
 * @param message The message.
 */
void EndpointManager::SendAnycastMessage(const Endpoint::Ptr& sender,
    const RequestMessage& message)
{
	String method;
	if (!message.GetMethod(&method))
		BOOST_THROW_EXCEPTION(invalid_argument("Message is missing the 'method' property."));

	vector<Endpoint::Ptr> candidates;

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Endpoint")) {
		Endpoint::Ptr endpoint = dynamic_pointer_cast<Endpoint>(object);
		/* don't forward messages between non-local endpoints */
		if ((sender && !sender->IsLocal()) && !endpoint->IsLocal())
			continue;

		if (endpoint->HasSubscription(method))
			candidates.push_back(endpoint);
	}

	if (candidates.empty())
		return;

	Endpoint::Ptr recipient = candidates[rand() % candidates.size()];
	SendUnicastMessage(sender, recipient, message);
}

/**
 * Sends an anonymous message to all recipients who have a subscription for the
 * message#s topic.
 *
 * @param message The message.
 */
void EndpointManager::SendMulticastMessage(const RequestMessage& message)
{
	SendMulticastMessage(Endpoint::Ptr(), message);
}

/**
 * Sends a message to all recipients who have a subscription for the
 * message's topic.
 *
 * @param sender The sender of the message.
 * @param message The message.
 */
void EndpointManager::SendMulticastMessage(const Endpoint::Ptr& sender,
    const RequestMessage& message)
{
	String id;
	if (message.GetID(&id))
		BOOST_THROW_EXCEPTION(invalid_argument("Multicast requests must not have an ID."));

	String method;
	if (!message.GetMethod(&method))
		BOOST_THROW_EXCEPTION(invalid_argument("Message is missing the 'method' property."));

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Endpoint")) {
		Endpoint::Ptr recipient = dynamic_pointer_cast<Endpoint>(object);

		/* don't forward messages back to the sender */
		if (sender == recipient)
			continue;

		if (recipient->HasSubscription(method))
			SendUnicastMessage(sender, recipient, message);
	}
}

void EndpointManager::SendAPIMessage(const Endpoint::Ptr& sender, const Endpoint::Ptr& recipient,
    RequestMessage& message,
    const EndpointManager::APICallback& callback, double timeout)
{
	m_NextMessageID++;

	stringstream idstream;
	idstream << m_NextMessageID;

	String id = idstream.str();
	message.SetID(id);

	PendingRequest pr;
	pr.Request = message;
	pr.Callback = callback;
	pr.Timeout = Utility::GetTime() + timeout;

	m_Requests[id] = pr;

	if (!recipient)
		SendAnycastMessage(sender, message);
	else
		SendUnicastMessage(sender, recipient, message);
}

bool EndpointManager::RequestTimeoutLessComparer(const pair<String, PendingRequest>& a,
    const pair<String, PendingRequest>& b)
{
	return a.second.Timeout < b.second.Timeout;
}

void EndpointManager::SubscriptionTimerHandler(void)
{
	Dictionary::Ptr subscriptions = boost::make_shared<Dictionary>();

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Endpoint")) {
		Endpoint::Ptr endpoint = dynamic_pointer_cast<Endpoint>(object);

		/* don't copy subscriptions from non-local endpoints or the identity endpoint */
		if (!endpoint->IsLocalEndpoint() || endpoint == m_Endpoint)
			continue;

		if (endpoint->GetSubscriptions()) {
			String topic;
			BOOST_FOREACH(tie(tuples::ignore, topic), endpoint->GetSubscriptions()) {
				subscriptions->Set(topic, topic);
			}
		}
	}

	subscriptions->Seal();

	if (m_Endpoint)
		m_Endpoint->SetSubscriptions(subscriptions);
}

void EndpointManager::ReconnectTimerHandler(void)
{
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Endpoint")) {
		Endpoint::Ptr endpoint = dynamic_pointer_cast<Endpoint>(object);

		if (endpoint->IsConnected() || endpoint == m_Endpoint)
			continue;

		String node, service;
		node = endpoint->GetNode();
		service = endpoint->GetService();

		if (node.IsEmpty() || service.IsEmpty()) {
			Logger::Write(LogWarning, "icinga", "Can't reconnect "
			    "to endpoint '" + endpoint->GetName() + "': No "
			    "node/service information.");
			continue;
		}

		AddConnection(node, service);
	}
}

void EndpointManager::RequestTimerHandler(void)
{
	map<String, PendingRequest>::iterator it;
	for (it = m_Requests.begin(); it != m_Requests.end(); it++) {
		if (it->second.HasTimedOut()) {
			it->second.Callback(GetSelf(), Endpoint::Ptr(),
			    it->second.Request, ResponseMessage(), true);

			m_Requests.erase(it);

			break;
		}
	}
}

void EndpointManager::ProcessResponseMessage(const Endpoint::Ptr& sender,
    const ResponseMessage& message)
{
	String id;
	if (!message.GetID(&id))
		BOOST_THROW_EXCEPTION(invalid_argument("Response message must have a message ID."));

	map<String, PendingRequest>::iterator it;
	it = m_Requests.find(id);

	if (it == m_Requests.end())
		return;

	it->second.Callback(GetSelf(), sender, it->second.Request, message, false);

	m_Requests.erase(it);
}

EndpointManager::Ptr EndpointManager::GetInstance(void)
{
	static EndpointManager::Ptr instance;

	if (!instance)
		instance = boost::make_shared<EndpointManager>();

	return instance;
}
