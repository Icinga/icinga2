/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/jsonrpcconnection.hpp"
#include "remote/apilistener.hpp"
#include "remote/apifunction.hpp"
#include "remote/jsonrpc.hpp"
#include "base/defer.hpp"
#include "base/configtype.hpp"
#include "base/io-engine.hpp"
#include "base/json.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include "base/tlsstream.hpp"
#include <memory>
#include <utility>
#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/system/system_error.hpp>
#include <boost/thread/once.hpp>

using namespace icinga;

static Value SetLogPositionHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
REGISTER_APIFUNCTION(SetLogPosition, log, &SetLogPositionHandler);

static RingBuffer l_TaskStats (15 * 60);

JsonRpcConnection::JsonRpcConnection(const String& identity, bool authenticated,
	const Shared<AsioTlsStream>::Ptr& stream, ConnectionRole role)
	: JsonRpcConnection(identity, authenticated, stream, role, IoEngine::Get().GetIoContext())
{
}

JsonRpcConnection::JsonRpcConnection(const String& identity, bool authenticated,
	const Shared<AsioTlsStream>::Ptr& stream, ConnectionRole role, boost::asio::io_context& io)
	: m_Identity(identity), m_Authenticated(authenticated), m_Stream(stream), m_Role(role),
	m_Timestamp(Utility::GetTime()), m_Seen(Utility::GetTime()), m_NextHeartbeat(0), m_IoStrand(io),
	m_OutgoingMessagesQueued(io), m_WriterDone(io), m_ShuttingDown(false),
	m_CheckLivenessTimer(io), m_HeartbeatTimer(io)
{
	if (authenticated)
		m_Endpoint = Endpoint::GetByName(identity);
}

void JsonRpcConnection::Start()
{
	namespace asio = boost::asio;

	JsonRpcConnection::Ptr keepAlive (this);

	IoEngine::SpawnCoroutine(m_IoStrand, [this, keepAlive](asio::yield_context yc) { HandleIncomingMessages(yc); });
	IoEngine::SpawnCoroutine(m_IoStrand, [this, keepAlive](asio::yield_context yc) { WriteOutgoingMessages(yc); });
	IoEngine::SpawnCoroutine(m_IoStrand, [this, keepAlive](asio::yield_context yc) { HandleAndWriteHeartbeats(yc); });
	IoEngine::SpawnCoroutine(m_IoStrand, [this, keepAlive](asio::yield_context yc) { CheckLiveness(yc); });
}

void JsonRpcConnection::HandleIncomingMessages(boost::asio::yield_context yc)
{
	m_Stream->next_layer().SetSeen(&m_Seen);

	for (;;) {
		String message;

		try {
			message = JsonRpc::ReadMessage(m_Stream, yc, m_Endpoint ? -1 : 1024 * 1024);
		} catch (const std::exception& ex) {
			Log(m_ShuttingDown ? LogDebug : LogNotice, "JsonRpcConnection")
				<< "Error while reading JSON-RPC message for identity '" << m_Identity
				<< "': " << DiagnosticInformation(ex);

			break;
		}

		m_Seen = Utility::GetTime();

		try {
			CpuBoundWork handleMessage (yc);
			TimeoutLog logIfSlow (LogWarning, "JsonRpcConnection");
			logIfSlow << "Handling message from " << m_Identity << " took long";
			Utility::Sleep(6);

			MessageHandler(message);

			l_TaskStats.InsertValue(Utility::GetTime(), 1);
		} catch (const std::exception& ex) {
			Log(m_ShuttingDown ? LogDebug : LogWarning, "JsonRpcConnection")
				<< "Error while processing JSON-RPC message for identity '" << m_Identity
				<< "': " << DiagnosticInformation(ex);

			break;
		}
	}

	Disconnect();
}

void JsonRpcConnection::WriteOutgoingMessages(boost::asio::yield_context yc)
{
	Defer signalWriterDone ([this]() { m_WriterDone.Set(); });

	do {
		m_OutgoingMessagesQueued.Wait(yc);

		auto queue (std::move(m_OutgoingMessagesQueue));

		m_OutgoingMessagesQueue.clear();
		m_OutgoingMessagesQueued.Clear();

		if (!queue.empty()) {
			try {
				for (auto& message : queue) {
					size_t bytesSent = JsonRpc::SendRawMessage(m_Stream, message, yc);

					if (m_Endpoint) {
						m_Endpoint->AddMessageSent(bytesSent);
					}
				}

				m_Stream->async_flush(yc);
			} catch (const std::exception& ex) {
				Log(m_ShuttingDown ? LogDebug : LogWarning, "JsonRpcConnection")
					<< "Error while sending JSON-RPC message for identity '"
					<< m_Identity << "'\n" << DiagnosticInformation(ex);

				break;
			}
		}
	} while (!m_ShuttingDown);

	Disconnect();
}

double JsonRpcConnection::GetTimestamp() const
{
	return m_Timestamp;
}

String JsonRpcConnection::GetIdentity() const
{
	return m_Identity;
}

bool JsonRpcConnection::IsAuthenticated() const
{
	return m_Authenticated;
}

Endpoint::Ptr JsonRpcConnection::GetEndpoint() const
{
	return m_Endpoint;
}

Shared<AsioTlsStream>::Ptr JsonRpcConnection::GetStream() const
{
	return m_Stream;
}

ConnectionRole JsonRpcConnection::GetRole() const
{
	return m_Role;
}

void JsonRpcConnection::SendMessage(const Dictionary::Ptr& message)
{
	Ptr keepAlive (this);

	m_IoStrand.post([this, keepAlive, message]() { SendMessageInternal(message); });
}

void JsonRpcConnection::SendRawMessage(const String& message)
{
	Ptr keepAlive (this);

	m_IoStrand.post([this, keepAlive, message]() {
		m_OutgoingMessagesQueue.emplace_back(message);
		m_OutgoingMessagesQueued.Set();
	});
}

void JsonRpcConnection::SendMessageInternal(const Dictionary::Ptr& message)
{
	m_OutgoingMessagesQueue.emplace_back(JsonEncode(message));
	m_OutgoingMessagesQueued.Set();
}

void JsonRpcConnection::Disconnect()
{
	namespace asio = boost::asio;

	JsonRpcConnection::Ptr keepAlive (this);

	IoEngine::SpawnCoroutine(m_IoStrand, [this, keepAlive](asio::yield_context yc) {
		if (!m_ShuttingDown) {
			m_ShuttingDown = true;

			Log(LogWarning, "JsonRpcConnection")
				<< "API client disconnected for identity '" << m_Identity << "'";

			// We need to unregister the endpoint client as soon as possible not to confuse Icinga 2,
			// given that Endpoint::GetConnected() is just performing a check that the endpoint's client
			// cache is not empty, which could result in an already disconnected endpoint never trying to
			// reconnect again. See #7444.
			if (m_Endpoint) {
				m_Endpoint->RemoveClient(this);
			} else {
				ApiListener::GetInstance()->RemoveAnonymousClient(this);
			}

			m_OutgoingMessagesQueued.Set();

			m_WriterDone.Wait(yc);

			/*
			 * Do not swallow exceptions in a coroutine.
			 * https://github.com/Icinga/icinga2/issues/7351
			 * We must not catch `detail::forced_unwind exception` as
			 * this is used for unwinding the stack.
			 *
			 * Just use the error_code dummy here.
			 */
			boost::system::error_code ec;

			m_CheckLivenessTimer.cancel();
			m_HeartbeatTimer.cancel();

			m_Stream->lowest_layer().cancel(ec);

			Timeout::Ptr shutdownTimeout (new Timeout(
				m_IoStrand.context(),
				m_IoStrand,
				boost::posix_time::seconds(10),
				[this, keepAlive](asio::yield_context yc) {
					boost::system::error_code ec;
					m_Stream->lowest_layer().cancel(ec);
				}
			));

			m_Stream->next_layer().async_shutdown(yc[ec]);

			shutdownTimeout->Cancel();

			m_Stream->lowest_layer().shutdown(m_Stream->lowest_layer().shutdown_both, ec);
		}
	});
}

void JsonRpcConnection::MessageHandler(const String& jsonString)
{
	Dictionary::Ptr message = JsonRpc::DecodeMessage(jsonString);

	if (m_Endpoint && message->Contains("ts")) {
		double ts = message->Get("ts");

		/* ignore old messages */
		if (ts < m_Endpoint->GetRemoteLogPosition())
			return;

		m_Endpoint->SetRemoteLogPosition(ts);
	}

	MessageOrigin::Ptr origin = new MessageOrigin();
	origin->FromClient = this;

	if (m_Endpoint) {
		if (m_Endpoint->GetZone() != Zone::GetLocalZone())
			origin->FromZone = m_Endpoint->GetZone();
		else
			origin->FromZone = Zone::GetByName(message->Get("originZone"));

		m_Endpoint->AddMessageReceived(jsonString.GetLength());
	}

	Value vmethod;

	if (!message->Get("method", &vmethod)) {
		Value vid;

		if (!message->Get("id", &vid))
			return;

		Log(LogWarning, "JsonRpcConnection",
			"We received a JSON-RPC response message. This should never happen because we're only ever sending notifications.");

		return;
	}

	String method = vmethod;

	Log(LogNotice, "JsonRpcConnection")
		<< "Received '" << method << "' message from identity '" << m_Identity << "'.";

	Dictionary::Ptr resultMessage = new Dictionary();

	try {
		ApiFunction::Ptr afunc = ApiFunction::GetByName(method);

		if (!afunc) {
			Log(LogNotice, "JsonRpcConnection")
				<< "Call to non-existent function '" << method << "' from endpoint '" << m_Identity << "'.";
		} else {
			Dictionary::Ptr params = message->Get("params");
			if (params)
				resultMessage->Set("result", afunc->Invoke(origin, params));
			else
				resultMessage->Set("result", Empty);
		}
	} catch (const std::exception& ex) {
		/* TODO: Add a user readable error message for the remote caller */
		String diagInfo = DiagnosticInformation(ex);
		resultMessage->Set("error", diagInfo);
		Log(LogWarning, "JsonRpcConnection")
			<< "Error while processing message for identity '" << m_Identity << "'\n" << diagInfo;
	}

	if (message->Contains("id")) {
		resultMessage->Set("jsonrpc", "2.0");
		resultMessage->Set("id", message->Get("id"));

		SendMessageInternal(resultMessage);
	}
}

Value SetLogPositionHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	double log_position = params->Get("log_position");
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint)
		return Empty;

	if (log_position > endpoint->GetLocalLogPosition())
		endpoint->SetLocalLogPosition(log_position);

	return Empty;
}

void JsonRpcConnection::CheckLiveness(boost::asio::yield_context yc)
{
	boost::system::error_code ec;

	if (!m_Authenticated) {
		/* Anonymous connections are normally only used for requesting a certificate and are closed after this request
		 * is received. However, the request is only sent if the child has successfully verified the certificate of its
		 * parent so that it is an authenticated connection from its perspective. In case this verification fails, both
		 * ends view it as an anonymous connection and never actually use it but attempt a reconnect after 10 seconds
		 * leaking the connection. Therefore close it after a timeout.
		 */

		m_CheckLivenessTimer.expires_from_now(boost::posix_time::seconds(10));
		m_CheckLivenessTimer.async_wait(yc[ec]);

		if (m_ShuttingDown) {
			return;
		}

		auto remote (m_Stream->lowest_layer().remote_endpoint());

		Log(LogInformation, "JsonRpcConnection")
			<< "Closing anonymous connection [" << remote.address() << "]:" << remote.port() << " after 10 seconds.";

		Disconnect();
	} else {
		for (;;) {
			m_CheckLivenessTimer.expires_from_now(boost::posix_time::seconds(30));
			m_CheckLivenessTimer.async_wait(yc[ec]);

			if (m_ShuttingDown) {
				break;
			}

			if (m_Seen < Utility::GetTime() - 60 && (!m_Endpoint || !m_Endpoint->GetSyncing())) {
				Log(LogInformation, "JsonRpcConnection")
					<<  "No messages for identity '" << m_Identity << "' have been received in the last 60 seconds.";

				Disconnect();
				break;
			}
		}
	}
}

double JsonRpcConnection::GetWorkQueueRate()
{
	return l_TaskStats.UpdateAndGetValues(Utility::GetTime(), 60) / 60.0;
}
