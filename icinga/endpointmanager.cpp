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
	Application::Log(s.str());

	JsonRpcServer::Ptr server = make_shared<JsonRpcServer>(m_SSLContext);
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
	Application::Log(s.str());

	JsonRpcEndpoint::Ptr endpoint = make_shared<JsonRpcEndpoint>();
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
	server->OnNewClient.connect(bind(&EndpointManager::NewClientHandler,
	    this, _1));
}

/**
 * Processes a new client connection.
 *
 * @param ncea Event arguments.
 */
int EndpointManager::NewClientHandler(const NewClientEventArgs& ncea)
{
	string address = ncea.Client->GetPeerAddress();
	Application::Log("Accepted new client from " + address);

	JsonRpcEndpoint::Ptr endpoint = make_shared<JsonRpcEndpoint>();
	endpoint->SetClient(static_pointer_cast<JsonRpcClient>(ncea.Client));
	RegisterEndpoint(endpoint);

	return 0;
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
	if (!endpoint->IsLocal() && endpoint->GetIdentity() != "")
		throw invalid_argument("Identity must be empty.");

	endpoint->SetEndpointManager(static_pointer_cast<EndpointManager>(shared_from_this()));
	m_Endpoints.push_back(endpoint);

	NewEndpointEventArgs neea;
	neea.Source = shared_from_this();
	neea.Endpoint = endpoint;
	OnNewEndpoint(neea);
}

/**
 * Unregisters an endpoint.
 *
 * @param endpoint The endpoint.
 */
void EndpointManager::UnregisterEndpoint(Endpoint::Ptr endpoint)
{
	m_Endpoints.erase(
	    remove(m_Endpoints.begin(), m_Endpoints.end(), endpoint),
	    m_Endpoints.end());
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
	for (vector<Endpoint::Ptr>::iterator i = m_Endpoints.begin(); i != m_Endpoints.end(); i++)
	{
		Endpoint::Ptr endpoint = *i;
		if (endpoint->HasSubscription(method))
			candidates.push_back(endpoint);
	}

	if (candidates.size() == 0)
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

	for (vector<Endpoint::Ptr>::iterator i = m_Endpoints.begin(); i != m_Endpoints.end(); i++)
	{
		Endpoint::Ptr recipient = *i;

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
void EndpointManager::ForEachEndpoint(function<int (const NewEndpointEventArgs&)> callback)
{
	NewEndpointEventArgs neea;
	neea.Source = shared_from_this();

	vector<Endpoint::Ptr>::iterator prev, i;
	for (i = m_Endpoints.begin(); i != m_Endpoints.end(); ) {
		prev = i;
		i++;

		neea.Endpoint = *prev;
		callback(neea);
	}
}

/**
 * Retrieves an endpoint that has the specified identity.
 *
 * @param identity The identity of the endpoint.
 */
Endpoint::Ptr EndpointManager::GetEndpointByIdentity(string identity) const
{
	vector<Endpoint::Ptr>::const_iterator i;
	for (i = m_Endpoints.begin(); i != m_Endpoints.end(); i++) {
		if ((*i)->GetIdentity() == identity)
			return *i;
	}

	return Endpoint::Ptr();
}

void EndpointManager::SendAPIMessage(Endpoint::Ptr sender,
    RequestMessage& message,
    function<int(const NewResponseEventArgs&)> callback, time_t timeout)
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
	RescheduleRequestTimer();

	SendAnycastMessage(sender, message);
}

bool EndpointManager::RequestTimeoutLessComparer(const pair<string, PendingRequest>& a,
    const pair<string, PendingRequest>& b)
{
	return a.second.Timeout < b.second.Timeout;
}

void EndpointManager::RescheduleRequestTimer(void)
{
	map<string, PendingRequest>::iterator it;
	it = min_element(m_Requests.begin(), m_Requests.end(),
	    &EndpointManager::RequestTimeoutLessComparer);

	if (!m_RequestTimer) {
		m_RequestTimer = make_shared<Timer>();
		m_RequestTimer->OnTimerExpired.connect(bind(&EndpointManager::RequestTimerHandler, this, _1));
	}

	if (it != m_Requests.end()) {
		time_t now;
		time(&now);

		time_t next_timeout = (it->second.Timeout < now) ? now : it->second.Timeout;
		m_RequestTimer->SetInterval(next_timeout - now);
		m_RequestTimer->Start();
	} else {
		m_RequestTimer->Stop();
	}
}

int EndpointManager::RequestTimerHandler(const TimerEventArgs& ea)
{
	map<string, PendingRequest>::iterator it;
	for (it = m_Requests.begin(); it != m_Requests.end(); it++) {
		if (it->second.HasTimedOut()) {
			NewResponseEventArgs nrea;
			nrea.Request = it->second.Request;
			nrea.Source = shared_from_this();
			nrea.TimedOut = true;

			it->second.Callback(nrea);

			m_Requests.erase(it);

			break;
		}
	}

	RescheduleRequestTimer();

	return 0;
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

	NewResponseEventArgs nrea;
	nrea.Sender = sender;
	nrea.Request = it->second.Request;
	nrea.Response = message;
	nrea.Source = shared_from_this();
	nrea.TimedOut = false;

	it->second.Callback(nrea);

	m_Requests.erase(it);
	RescheduleRequestTimer();
}
