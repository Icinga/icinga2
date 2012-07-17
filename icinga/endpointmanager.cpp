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
}

/**
 * Sets the identity of the endpoint manager. This identity is used when
 * connecting to remote peers.
 *
 * @param identity The new identity.
 */
void EndpointManager::SetIdentity(string identity)
{
	m_Identity = identity;
}

/**
 * Retrieves the identity for the endpoint manager.
 *
 * @returns The identity.
 */
string EndpointManager::GetIdentity(void) const
{
	return m_Identity;
}

/**
 * Sets the SSL context that is used for remote connections.
 *
 * @param sslContext The new SSL context.
 */
void EndpointManager::SetSSLContext(shared_ptr<SSL_CTX> sslContext)
{
	m_SSLContext = sslContext;
}

/**
 * Retrieves the SSL context that is used for remote connections.
 *
 * @returns The SSL context.
 */
shared_ptr<SSL_CTX> EndpointManager::GetSSLContext(void) const
{
	return m_SSLContext;
}

/**
 * Creates a new JSON-RPC listener on the specified port.
 *
 * @param service The port to listen on.
 */
void EndpointManager::AddListener(string service)
{
	if (!GetSSLContext())
		throw logic_error("SSL context is required for AddListener()");

	stringstream s;
	s << "Adding new listener: port " << service;
	Logger::Write(LogInformation, "icinga", s.str());

	JsonRpcServer::Ptr server = boost::make_shared<JsonRpcServer>(m_SSLContext);
	RegisterServer(server);

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
void EndpointManager::AddConnection(string node, string service)
{
	stringstream s;
	s << "Adding new endpoint: [" << node << "]:" << service;
	Logger::Write(LogInformation, "icinga", s.str());

	JsonRpcEndpoint::Ptr endpoint = boost::make_shared<JsonRpcEndpoint>();
	RegisterEndpoint(endpoint);
	endpoint->Connect(node, service, m_SSLContext);
}

/**
 * Registers a new JSON-RPC server with this endpoint manager.
 *
 * @param server The JSON-RPC server.
 */
void EndpointManager::RegisterServer(JsonRpcServer::Ptr server)
{
	m_Servers.push_back(server);
	server->OnNewClient.connect(boost::bind(&EndpointManager::NewClientHandler,
	    this, _2));
}

/**
 * Processes a new client connection.
 *
 * @param ncea Event arguments.
 */
void EndpointManager::NewClientHandler(const TcpClient::Ptr& client)
{
	Logger::Write(LogInformation, "icinga", "Accepted new client from " + client->GetPeerAddress());

	JsonRpcEndpoint::Ptr endpoint = boost::make_shared<JsonRpcEndpoint>();
	endpoint->SetClient(static_pointer_cast<JsonRpcClient>(client));
	client->Start();
	RegisterEndpoint(endpoint);
}

/**
 * Unregisters a JSON-RPC server.
 *
 * @param server The JSON-RPC server.
 */
void EndpointManager::UnregisterServer(JsonRpcServer::Ptr server)
{
	m_Servers.erase(
	    remove(m_Servers.begin(), m_Servers.end(), server),
	    m_Servers.end());
	// TODO: unbind event
}

/**
 * Registers a new endpoint with this endpoint manager.
 *
 * @param endpoint The new endpoint.
 */
void EndpointManager::RegisterEndpoint(Endpoint::Ptr endpoint)
{
	endpoint->SetEndpointManager(GetSelf());

	UnregisterEndpoint(endpoint);

	string identity = endpoint->GetIdentity();

	if (!identity.empty()) {
		m_Endpoints[identity] = endpoint;
		OnNewEndpoint(GetSelf(), endpoint);
	} else {
		m_PendingEndpoints.push_back(endpoint);
	}

	if (endpoint->IsLocal()) {
		/* this endpoint might have introduced new subscriptions
		 * or publications which affect remote endpoints, we need
		 * to close all fully-connected remote endpoints to make sure
		 * these subscriptions/publications are kept up-to-date. */
		Iterator prev, it;
		for (it = m_Endpoints.begin(); it != m_Endpoints.end(); ) {
			prev = it;
			it++;

			if (!prev->second->IsLocal())
				m_Endpoints.erase(prev);
		}
	}
}

/**
 * Unregisters an endpoint.
 *
 * @param endpoint The endpoint.
 */
void EndpointManager::UnregisterEndpoint(Endpoint::Ptr endpoint)
{
	m_PendingEndpoints.erase(
	    remove(m_PendingEndpoints.begin(), m_PendingEndpoints.end(), endpoint),
	    m_PendingEndpoints.end());

	string identity = endpoint->GetIdentity();
	if (!identity.empty())
		m_Endpoints.erase(identity);
}

/**
 * Sends a unicast message to the specified recipient.
 *
 * @param sender The sender of the message.
 * @param recipient The recipient of the message.
 * @param message The request.
 */
void EndpointManager::SendUnicastMessage(Endpoint::Ptr sender,
    Endpoint::Ptr recipient, const MessagePart& message)
{
	/* don't forward messages between non-local endpoints */
	if (!sender->IsLocal() && !recipient->IsLocal())
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
void EndpointManager::SendAnycastMessage(Endpoint::Ptr sender,
    const RequestMessage& message)
{
	string method;
	if (!message.GetMethod(&method))
		throw invalid_argument("Message is missing the 'method' property.");

	vector<Endpoint::Ptr> candidates;
	BOOST_FOREACH(const Endpoint::Ptr& endpoint, m_Endpoints | map_values) {
		/* don't forward messages between non-local endpoints */
		if (!sender->IsLocal() && !endpoint->IsLocal())
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
 * Sends a message to all recipients who have a subscription for the
 * message's topic.
 *
 * @param sender The sender of the message.
 * @param message The message.
 */
void EndpointManager::SendMulticastMessage(Endpoint::Ptr sender,
    const RequestMessage& message)
{
	string id;
	if (message.GetID(&id))
		throw invalid_argument("Multicast requests must not have an ID.");

	string method;
	if (!message.GetMethod(&method))
		throw invalid_argument("Message is missing the 'method' property.");

	BOOST_FOREACH(const Endpoint::Ptr& recipient, m_Endpoints | map_values) {
		/* don't forward messages back to the sender */
		if (sender == recipient)
			continue;

		if (recipient->HasSubscription(method))
			SendUnicastMessage(sender, recipient, message);
	}
}

/**
 * Calls the specified callback function for each registered endpoint.
 *
 * @param callback The callback function.
 */
void EndpointManager::ForEachEndpoint(function<void (const EndpointManager::Ptr&, const Endpoint::Ptr&)> callback)
{
	map<string, Endpoint::Ptr>::iterator prev, i;
	for (i = m_Endpoints.begin(); i != m_Endpoints.end(); ) {
		prev = i;
		i++;

		callback(GetSelf(), prev->second);
	}
}

/**
 * Retrieves an endpoint that has the specified identity.
 *
 * @param identity The identity of the endpoint.
 */
Endpoint::Ptr EndpointManager::GetEndpointByIdentity(string identity) const
{
	map<string, Endpoint::Ptr>::const_iterator i;
	i = m_Endpoints.find(identity);
	if (i != m_Endpoints.end())
		return i->second;
	else
		return Endpoint::Ptr();
}

void EndpointManager::SendAPIMessage(const Endpoint::Ptr& sender, const Endpoint::Ptr& recipient,
    RequestMessage& message,
    function<void(const EndpointManager::Ptr&, const Endpoint::Ptr, const RequestMessage&, const ResponseMessage&, bool TimedOut)> callback, time_t timeout)
{
	m_NextMessageID++;

	stringstream idstream;
	idstream << m_NextMessageID;

	string id = idstream.str();
	message.SetID(id);

	PendingRequest pr;
	pr.Request = message;
	pr.Callback = callback;
	pr.Timeout = time(NULL) + timeout;

	m_Requests[id] = pr;

	if (!recipient)
		SendAnycastMessage(sender, message);
	else
		SendUnicastMessage(sender, recipient, message);
}

bool EndpointManager::RequestTimeoutLessComparer(const pair<string, PendingRequest>& a,
    const pair<string, PendingRequest>& b)
{
	return a.second.Timeout < b.second.Timeout;
}

void EndpointManager::RequestTimerHandler(void)
{
	map<string, PendingRequest>::iterator it;
	for (it = m_Requests.begin(); it != m_Requests.end(); it++) {
		if (it->second.HasTimedOut()) {
			it->second.Callback(GetSelf(), Endpoint::Ptr(), it->second.Request, ResponseMessage(), true);

			m_Requests.erase(it);

			break;
		}
	}
}

void EndpointManager::ProcessResponseMessage(const Endpoint::Ptr& sender, const ResponseMessage& message)
{
	string id;
	if (!message.GetID(&id))
		throw invalid_argument("Response message must have a message ID.");

	map<string, PendingRequest>::iterator it;
	it = m_Requests.find(id);

	if (it == m_Requests.end())
		return;

	it->second.Callback(GetSelf(), sender, it->second.Request, message, false);

	m_Requests.erase(it);
}

EndpointManager::Iterator EndpointManager::Begin(void)
{
	return m_Endpoints.begin();
}

EndpointManager::Iterator EndpointManager::End(void)
{
	return m_Endpoints.end();
}

EndpointManager::Ptr EndpointManager::GetInstance(void)
{
	static EndpointManager::Ptr instance;

	if (!instance)
		instance = boost::make_shared<EndpointManager>();

	return instance;
}
