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
		throw InvalidArgumentException("SSL context is required for AddListener()");

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
	server->OnNewClient += bind_weak(&EndpointManager::NewClientHandler,
	    shared_from_this());
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
		throw InvalidArgumentException("Identity must be empty.");

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
	/* don't forward messages back to the sender */
	if (sender == recipient)
		return;

	/* don't forward messages between non-local endpoints */
	if (!sender->IsLocal() && !recipient->IsLocal())
		return;

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
    const RpcRequest& message)
{
	throw NotImplementedException();
}

/**
 * Sends a message to all recipients who have a subscription for the
 * message's topic.
 *
 * @param sender The sender of the message.
 * @param message The message.
 */
void EndpointManager::SendMulticastMessage(Endpoint::Ptr sender,
    const RpcRequest& message)
{
	string id;
	if (message.GetID(&id))
		throw InvalidArgumentException("Multicast requests must not have an ID.");

	string method;
	if (!message.GetMethod(&method))
		throw InvalidArgumentException("Message is missing the 'method' property.");

	for (vector<Endpoint::Ptr>::iterator i = m_Endpoints.begin(); i != m_Endpoints.end(); i++)
	{
		Endpoint::Ptr recipient = *i;
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
