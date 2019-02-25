/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef JSONRPCCONNECTION_H
#define JSONRPCCONNECTION_H

#include "remote/i2-remote.hpp"
#include "remote/endpoint.hpp"
#include "base/tlsstream.hpp"
#include "base/timer.hpp"
#include "base/workqueue.hpp"

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

	JsonRpcConnection(const String& identity, bool authenticated, TlsStream::Ptr stream, ConnectionRole role);

	void Start();

	double GetTimestamp() const;
	String GetIdentity() const;
	bool IsAuthenticated() const;
	Endpoint::Ptr GetEndpoint() const;
	TlsStream::Ptr GetStream() const;
	ConnectionRole GetRole() const;

	void Disconnect();

	void SendMessage(const Dictionary::Ptr& request);

	static void HeartbeatTimerHandler();
	static Value HeartbeatAPIHandler(const intrusive_ptr<MessageOrigin>& origin, const Dictionary::Ptr& params);

	static size_t GetWorkQueueCount();
	static size_t GetWorkQueueLength();
	static double GetWorkQueueRate();

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

	bool ProcessMessage();
	void MessageHandlerWrapper(const String& jsonString);
	void MessageHandler(const String& jsonString);
	void DataAvailableHandler();

	static void StaticInitialize();
	static void TimeoutTimerHandler();
	void CheckLiveness();

	void CertificateRequestResponseHandler(const Dictionary::Ptr& message);
};

}

#endif /* JSONRPCCONNECTION_H */
