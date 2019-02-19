/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/jsonrpcconnection.hpp"
#include "remote/apilistener.hpp"
#include "remote/apifunction.hpp"
#include "remote/jsonrpc.hpp"
#include "base/configtype.hpp"
#include "base/io-engine.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include "base/tlsstream.hpp"
#include <memory>
#include <utility>
#include <boost/asio/spawn.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/thread/once.hpp>

using namespace icinga;

static Value SetLogPositionHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
REGISTER_APIFUNCTION(SetLogPosition, log, &SetLogPositionHandler);

JsonRpcConnection::JsonRpcConnection(const String& identity, bool authenticated,
	const std::shared_ptr<AsioTlsStream>& stream, ConnectionRole role)
	: m_Identity(identity), m_Authenticated(authenticated), m_Stream(stream),
	m_Role(role), m_Timestamp(Utility::GetTime()), m_IoStrand(stream->get_io_service()),
	m_OutgoingMessagesQueued(stream->get_io_service()), m_ReaderHasError(false), m_RunningCoroutines(0)
{
	if (authenticated)
		m_Endpoint = Endpoint::GetByName(identity);

	m_OutgoingMessagesQueued.expires_at(boost::posix_time::pos_infin);
}

void JsonRpcConnection::Start()
{
	namespace asio = boost::asio;

	m_RunningCoroutines = 2;

	asio::spawn(m_IoStrand, [this](asio::yield_context yc) { HandleIncomingMessages(yc); });
	asio::spawn(m_IoStrand, [this](asio::yield_context yc) { WriteOutgoingMessages(yc); });
}

void JsonRpcConnection::HandleIncomingMessages(boost::asio::yield_context yc)
{
	Defer shutdownStreamOnce ([this, &yc]() {
		m_ReaderHasError = true;
		m_OutgoingMessagesQueued.expires_at(boost::posix_time::neg_infin);

		ShutdownStreamOnce(yc);
	});

	for (;;) {
		String message;

		try {
			message = JsonRpc::ReadMessage(m_Stream, yc, m_Endpoint ? -1 : 1024 * 1024);
		} catch (const std::exception& ex) {
			Log(LogWarning, "JsonRpcConnection")
				<< "Error while reading JSON-RPC message for identity '" << m_Identity
				<< "': " << DiagnosticInformation(ex);

			break;
		}

		try {
			CpuBoundWork handleMessage (yc);

			MessageHandler(message);
		} catch (const std::exception& ex) {
			Log(LogWarning, "JsonRpcConnection")
				<< "Error while processing JSON-RPC message for identity '" << m_Identity
				<< "': " << DiagnosticInformation(ex);

			break;
		}
	}
}

void JsonRpcConnection::WriteOutgoingMessages(boost::asio::yield_context yc)
{
	Defer shutdownStreamOnce ([this, &yc]() { ShutdownStreamOnce(yc); });

	do {
		try {
			m_OutgoingMessagesQueued.async_wait(yc);
		} catch (...) {
		}

		auto queue (std::move(m_OutgoingMessagesQueue));

		m_OutgoingMessagesQueue.clear();
		m_OutgoingMessagesQueued.expires_at(boost::posix_time::pos_infin);

		if (!queue.empty()) {
			try {
				for (auto& message : queue) {
					size_t bytesSent = JsonRpc::SendMessage(m_Stream, message, yc);

					if (m_Endpoint) {
						m_Endpoint->AddMessageSent(bytesSent);
					}
				}

				m_Stream->async_flush(yc);
			} catch (const std::exception& ex) {
				std::ostringstream info;
				info << "Error while sending JSON-RPC message for identity '" << m_Identity << "'";
				Log(LogWarning, "JsonRpcConnection")
					<< info.str() << "\n" << DiagnosticInformation(ex);

				break;
			}
		}
	} while (!m_ReaderHasError);
}

void JsonRpcConnection::ShutdownStreamOnce(boost::asio::yield_context& yc)
{
	if (!--m_RunningCoroutines) {
		try {
			m_Stream->next_layer().async_shutdown(yc);
		} catch (...) {
			// https://stackoverflow.com/questions/130117/throwing-exceptions-out-of-a-destructor
		}

		Log(LogWarning, "JsonRpcConnection")
			<< "API client disconnected for identity '" << m_Identity << "'";

		if (m_Endpoint) {
			m_Endpoint->RemoveClient(this);
		} else {
			auto listener (ApiListener::GetInstance());
			listener->RemoveAnonymousClient(this);
		}
	}
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

std::shared_ptr<AsioTlsStream> JsonRpcConnection::GetStream() const
{
	return m_Stream;
}

ConnectionRole JsonRpcConnection::GetRole() const
{
	return m_Role;
}

void JsonRpcConnection::SendMessage(const Dictionary::Ptr& message)
{
	m_IoStrand.post([this, message]() {
		m_OutgoingMessagesQueue.emplace_back(message);
		m_OutgoingMessagesQueued.expires_at(boost::posix_time::neg_infin);
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
		<< "Received '" << method << "' message from '" << m_Identity << "'";

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

		m_OutgoingMessagesQueue.emplace_back(resultMessage);
		m_OutgoingMessagesQueued.expires_at(boost::posix_time::neg_infin);
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

