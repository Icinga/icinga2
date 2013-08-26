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
	DECLARE_PTR_TYPEDEFS(EndpointManager);

	EndpointManager(void);

	static EndpointManager *GetInstance(void);

	void SetIdentity(const String& identity);
	String GetIdentity(void) const;

	void SetSSLContext(const shared_ptr<SSL_CTX>& sslContext);
	shared_ptr<SSL_CTX> GetSSLContext(void) const;

	void AddListener(const String& service);
	void AddConnection(const String& node, const String& service);

	//void SendUnicastMessage(const Endpoint::Ptr& recipient, const Dictionary::Ptr& message);
	//void SendUnicastMessage(const Endpoint::Ptr& sender, const Endpoint::Ptr& recipient, const MessagePart& message);
	//void SendAnycastMessage(const Endpoint::Ptr& sender, const RequestMessage& message);
	//void SendMulticastMessage(const RequestMessage& message);
	//void SendMulticastMessage(const Endpoint::Ptr& sender, const RequestMessage& message);

	boost::signals2::signal<void (const Endpoint::Ptr&)> OnNewEndpoint;

private:
	String m_Identity;
	Endpoint::Ptr m_Endpoint;

	shared_ptr<SSL_CTX> m_SSLContext;

	Timer::Ptr m_ReconnectTimer;

	std::set<TcpSocket::Ptr> m_Servers;

	void ReconnectTimerHandler(void);

	void NewClientHandler(const Socket::Ptr& client, TlsRole role);

	void ListenerThreadProc(const Socket::Ptr& server);
};

}

#endif /* ENDPOINTMANAGER_H */
