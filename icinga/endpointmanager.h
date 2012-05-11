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

#ifndef ENDPOINTMANAGER_H
#define ENDPOINTMANAGER_H

namespace icinga
{

struct I2_ICINGA_API NewEndpointEventArgs : public EventArgs
{
	icinga::Endpoint::Ptr Endpoint;
};

class I2_ICINGA_API EndpointManager : public Object
{
	string m_Identity;
	shared_ptr<SSL_CTX> m_SSLContext;

	list<JsonRpcServer::Ptr> m_Servers;
	list<Endpoint::Ptr> m_Endpoints;

	void RegisterServer(JsonRpcServer::Ptr server);
	void UnregisterServer(JsonRpcServer::Ptr server);

	int NewClientHandler(const NewClientEventArgs& ncea);

public:
	typedef shared_ptr<EndpointManager> Ptr;
	typedef weak_ptr<EndpointManager> WeakPtr;

	void SetIdentity(string identity);
	string GetIdentity(void) const;

	void SetSSLContext(shared_ptr<SSL_CTX> sslContext);
	shared_ptr<SSL_CTX> GetSSLContext(void) const;

	void AddListener(string service);
	void AddConnection(string node, string service);

	void RegisterEndpoint(Endpoint::Ptr endpoint);
	void UnregisterEndpoint(Endpoint::Ptr endpoint);

	void SendUnicastRequest(Endpoint::Ptr sender, Endpoint::Ptr recipient, const JsonRpcRequest& request, bool fromLocal = true);
	void SendAnycastRequest(Endpoint::Ptr sender, const JsonRpcRequest& request, bool fromLocal = true);
	void SendMulticastRequest(Endpoint::Ptr sender, const JsonRpcRequest& request, bool fromLocal = true);

	void ForEachEndpoint(function<int (const NewEndpointEventArgs&)> callback);

	Endpoint::Ptr GetEndpointByIdentity(string identity) const;

	Event<NewEndpointEventArgs> OnNewEndpoint;
};

}

#endif /* ENDPOINTMANAGER_H */
