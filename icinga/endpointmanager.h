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

/**
 * Forwards messages between endpoints.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API EndpointManager : public Object
{
public:
	typedef shared_ptr<EndpointManager> Ptr;
	typedef weak_ptr<EndpointManager> WeakPtr;

	typedef map<string, Endpoint::Ptr>::iterator Iterator;

	EndpointManager(void)
		: m_NextMessageID(0)
	{ }

	void SetIdentity(string identity);
	string GetIdentity(void) const;

	void SetSSLContext(shared_ptr<SSL_CTX> sslContext);
	shared_ptr<SSL_CTX> GetSSLContext(void) const;

	void AddListener(string service);
	void AddConnection(string node, string service);

	void RegisterEndpoint(Endpoint::Ptr endpoint);
	void UnregisterEndpoint(Endpoint::Ptr endpoint);

	void SendUnicastMessage(Endpoint::Ptr sender, Endpoint::Ptr recipient, const MessagePart& message);
	void SendAnycastMessage(Endpoint::Ptr sender, const RequestMessage& message);
	void SendMulticastMessage(Endpoint::Ptr sender, const RequestMessage& message);

	void SendAPIMessage(Endpoint::Ptr sender, RequestMessage& message,
	    function<void(const EndpointManager::Ptr&, const Endpoint::Ptr, const RequestMessage&, const ResponseMessage&, bool TimedOut)> callback, time_t timeout = 10);

	void ProcessResponseMessage(const Endpoint::Ptr& sender, const ResponseMessage& message);

	void ForEachEndpoint(function<void (const EndpointManager::Ptr&, const Endpoint::Ptr&)> callback);
	Iterator Begin(void);
	Iterator End(void);

	Endpoint::Ptr GetEndpointByIdentity(string identity) const;

	boost::signal<void (const EndpointManager::Ptr&, const Endpoint::Ptr&)> OnNewEndpoint;

private:
	string m_Identity;
	shared_ptr<SSL_CTX> m_SSLContext;

	vector<JsonRpcServer::Ptr> m_Servers;
	vector<Endpoint::Ptr> m_PendingEndpoints;
	map<string, Endpoint::Ptr> m_Endpoints;

	/**
	 * Information about a pending API request.
	 *
	 * @ingroup icinga
	 */
	struct I2_ICINGA_API PendingRequest
	{
		time_t Timeout;
		RequestMessage Request;
		function<void(const EndpointManager::Ptr&, const Endpoint::Ptr, const RequestMessage&, const ResponseMessage&, bool TimedOut)> Callback;

		bool HasTimedOut(void) const
		{
			return time(NULL) > Timeout;
		}
	};

	long m_NextMessageID;
	map<string, PendingRequest> m_Requests;
	Timer::Ptr m_RequestTimer;

	void RegisterServer(JsonRpcServer::Ptr server);
	void UnregisterServer(JsonRpcServer::Ptr server);

	static bool RequestTimeoutLessComparer(const pair<string, PendingRequest>& a, const pair<string, PendingRequest>& b);
	void RescheduleRequestTimer(void);
	void RequestTimerHandler(void);

	void NewClientHandler(const TcpClient::Ptr& client);
};

}

#endif /* ENDPOINTMANAGER_H */
