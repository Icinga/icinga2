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

#include "remoting/i2-remoting.h"
#include "remoting/endpoint.h"
#include "base/tcpsocket.h"
#include "base/tlsstream.h"
#include "base/timer.h"
#include "base/utility.h"
#include <boost/signals2.hpp>

namespace icinga
{

/**
 * Forwards messages between endpoints.
 *
 * @ingroup remoting
 */
class I2_REMOTING_API EndpointManager : public Object
{
public:
	typedef shared_ptr<EndpointManager> Ptr;
	typedef weak_ptr<EndpointManager> WeakPtr;

	EndpointManager(void);

	static EndpointManager *GetInstance(void);

	void SetIdentity(const String& identity);
	String GetIdentity(void) const;

	void SetSSLContext(const shared_ptr<SSL_CTX>& sslContext);
	shared_ptr<SSL_CTX> GetSSLContext(void) const;

	void AddListener(const String& service);
	void AddConnection(const String& node, const String& service);

	void SendUnicastMessage(const Endpoint::Ptr& recipient, const MessagePart& message);
	void SendUnicastMessage(const Endpoint::Ptr& sender, const Endpoint::Ptr& recipient, const MessagePart& message);
	void SendAnycastMessage(const Endpoint::Ptr& sender, const RequestMessage& message);
	void SendMulticastMessage(const RequestMessage& message);
	void SendMulticastMessage(const Endpoint::Ptr& sender, const RequestMessage& message);

	typedef boost::function<void(const Endpoint::Ptr, const RequestMessage&, const ResponseMessage&, bool TimedOut)> APICallback;

	void SendAPIMessage(const Endpoint::Ptr& sender, const Endpoint::Ptr& recipient, RequestMessage& message,
	    const APICallback& callback, double timeout = 30);

	void ProcessResponseMessage(const Endpoint::Ptr& sender, const ResponseMessage& message);

	boost::signals2::signal<void (const Endpoint::Ptr&)> OnNewEndpoint;

private:
	String m_Identity;
	Endpoint::Ptr m_Endpoint;

	shared_ptr<SSL_CTX> m_SSLContext;

	Timer::Ptr m_SubscriptionTimer;

	Timer::Ptr m_ReconnectTimer;

	std::set<TcpSocket::Ptr> m_Servers;
	std::set<TlsStream::Ptr> m_PendingClients;

	/**
	 * Information about a pending API request.
	 *
	 * @ingroup remoting
	 */
	struct I2_REMOTING_API PendingRequest
	{
		double Timeout;
		RequestMessage Request;
		boost::function<void(const Endpoint::Ptr, const RequestMessage&, const ResponseMessage&, bool TimedOut)> Callback;

		bool HasTimedOut(void) const
		{
			return Utility::GetTime() > Timeout;
		}
	};

	long m_NextMessageID;
	std::map<String, PendingRequest> m_Requests;
	Timer::Ptr m_RequestTimer;

	static bool RequestTimeoutLessComparer(const std::pair<String, PendingRequest>& a, const std::pair<String, PendingRequest>& b);
	void RequestTimerHandler(void);

	void SubscriptionTimerHandler(void);

	void ReconnectTimerHandler(void);

	void NewClientHandler(const Socket::Ptr& client, TlsRole role);
	void ClientConnectedHandler(const Stream::Ptr& client);
	void ClientClosedHandler(const Stream::Ptr& client);
};

}

#endif /* ENDPOINTMANAGER_H */
