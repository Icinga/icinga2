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

void EndpointManager::SetIdentity(string identity)
{
	m_Identity = identity;
}

string EndpointManager::GetIdentity(void) const
{
	return m_Identity;
}

void EndpointManager::SetSSLContext(shared_ptr<SSL_CTX> sslContext)
{
	m_SSLContext = sslContext;
}

shared_ptr<SSL_CTX> EndpointManager::GetSSLContext(void) const
{
	return m_SSLContext;
}

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

void EndpointManager::AddConnection(string node, string service)
{
	stringstream s;
	s << "Adding new endpoint: [" << node << "]:" << service;
	Application::Log(s.str());

	JsonRpcEndpoint::Ptr endpoint = make_shared<JsonRpcEndpoint>();
	RegisterEndpoint(endpoint);
	endpoint->Connect(node, service, m_SSLContext);
}

void EndpointManager::RegisterServer(JsonRpcServer::Ptr server)
{
	m_Servers.push_back(server);
	server->OnNewClient += bind_weak(&EndpointManager::NewClientHandler, shared_from_this());
}

int EndpointManager::NewClientHandler(const NewClientEventArgs& ncea)
{
	string address = ncea.Client->GetPeerAddress();
	Application::Log("Accepted new client from " + address);

	JsonRpcEndpoint::Ptr endpoint = make_shared<JsonRpcEndpoint>();
	endpoint->SetClient(static_pointer_cast<JsonRpcClient>(ncea.Client));
	RegisterEndpoint(endpoint);

	return 0;
}

void EndpointManager::UnregisterServer(JsonRpcServer::Ptr server)
{
	m_Servers.erase(
	    remove(m_Servers.begin(), m_Servers.end(), server),
	    m_Servers.end());
	// TODO: unbind event
}

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

void EndpointManager::UnregisterEndpoint(Endpoint::Ptr endpoint)
{
	m_Endpoints.erase(
	    remove(m_Endpoints.begin(), m_Endpoints.end(), endpoint),
	    m_Endpoints.end());
}

void EndpointManager::SendUnicastRequest(Endpoint::Ptr sender, Endpoint::Ptr recipient, const JsonRpcRequest& request, bool fromLocal)
{
	if (sender == recipient)
		return;

	/* don't forward messages between non-local endpoints */
	if (!fromLocal && !recipient->IsLocal())
		return;

	string method;
	if (!request.GetMethod(&method))
		throw InvalidArgumentException("Missing 'method' parameter.");

	if (recipient->IsMethodSink(method)) {
		//Application::Log(sender->GetAddress() + " -> " + recipient->GetAddress() + ": " + method);
		recipient->ProcessRequest(sender, request);
	}
}

void EndpointManager::SendAnycastRequest(Endpoint::Ptr sender, const JsonRpcRequest& request, bool fromLocal)
{
	throw NotImplementedException();
}

void EndpointManager::SendMulticastRequest(Endpoint::Ptr sender, const JsonRpcRequest& request, bool fromLocal)
{
#ifdef _DEBUG
	string id;
	if (request.GetID(&id))
		throw InvalidArgumentException("Multicast requests must not have an ID.");
#endif /* _DEBUG */

	string method;
	if (!request.GetMethod(&method))
		throw InvalidArgumentException("Message is missing the 'method' property.");

	for (vector<Endpoint::Ptr>::iterator i = m_Endpoints.begin(); i != m_Endpoints.end(); i++)
	{
		SendUnicastRequest(sender, *i, request, fromLocal);
	}
}

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

Endpoint::Ptr EndpointManager::GetEndpointByIdentity(string identity) const
{
	vector<Endpoint::Ptr>::const_iterator i;
	for (i = m_Endpoints.begin(); i != m_Endpoints.end(); i++) {
		if ((*i)->GetIdentity() == identity)
			return *i;
	}

	return Endpoint::Ptr();
}
