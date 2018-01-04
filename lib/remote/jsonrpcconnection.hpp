/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef JSONRPCCONNECTION_H
#define JSONRPCCONNECTION_H

#include "remote/endpoint.hpp"
#include "base/tlsstream.hpp"
#include "base/timer.hpp"
#include "base/workqueue.hpp"
#include "remote/i2-remote.hpp"

namespace icinga
{

enum ClientRole
{
	ClientInbound,
	ClientOutbound
};

enum ClientType
{
	ClientJsonRpc,
	ClientHttp
};

class MessageOrigin;

/**
 * An API client connection.
 *
 * @ingroup remote
 */
class JsonRpcConnection final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(JsonRpcConnection);

	JsonRpcConnection(const String& identity, bool authenticated, const TlsStream::Ptr& stream, ConnectionRole role);

	void Start(void);

	double GetTimestamp(void) const;
	String GetIdentity(void) const;
	bool IsAuthenticated(void) const;
	Endpoint::Ptr GetEndpoint(void) const;
	TlsStream::Ptr GetStream(void) const;
	ConnectionRole GetRole(void) const;

	void Disconnect(void);

	void SendMessage(const Dictionary::Ptr& request);

	static void HeartbeatTimerHandler(void);
	static Value HeartbeatAPIHandler(const intrusive_ptr<MessageOrigin>& origin, const Dictionary::Ptr& params);

	static size_t GetWorkQueueCount(void);
	static size_t GetWorkQueueLength(void);
	static double GetWorkQueueRate(void);

	static void SendCertificateRequest(const JsonRpcConnection::Ptr& aclient, const intrusive_ptr<MessageOrigin>& origin, const String& path);

private:
	int m_ID;
	String m_Identity;
	bool m_Authenticated;
	Endpoint::Ptr m_Endpoint;
	TlsStream::Ptr m_Stream;
	ConnectionRole m_Role;
	double m_Timestamp;
	double m_Seen;
	double m_NextHeartbeat;
	double m_HeartbeatTimeout;
	boost::mutex m_DataHandlerMutex;

	StreamReadContext m_Context;

	bool ProcessMessage(void);
	void MessageHandlerWrapper(const String& jsonString);
	void MessageHandler(const String& jsonString);
	void DataAvailableHandler(void);

	static void StaticInitialize(void);
	static void TimeoutTimerHandler(void);
	void CheckLiveness(void);

	void CertificateRequestResponseHandler(const Dictionary::Ptr& message);
};

}

#endif /* JSONRPCCONNECTION_H */
