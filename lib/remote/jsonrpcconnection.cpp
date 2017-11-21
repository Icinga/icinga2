/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "remote/jsonrpcconnection.hpp"
#include "remote/apilistener.hpp"
#include "remote/apifunction.hpp"
#include "remote/jsonrpc.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include <boost/thread/once.hpp>

using namespace icinga;

static Value SetLogPositionHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
REGISTER_APIFUNCTION(SetLogPosition, log, &SetLogPositionHandler);

static boost::once_flag l_JsonRpcConnectionOnceFlag = BOOST_ONCE_INIT;
static Timer::Ptr l_JsonRpcConnectionTimeoutTimer;
static WorkQueue *l_JsonRpcConnectionWorkQueues;
static size_t l_JsonRpcConnectionWorkQueueCount;
static int l_JsonRpcConnectionNextID;

JsonRpcConnection::JsonRpcConnection(const String& identity, bool authenticated,
    const TlsStream::Ptr& stream, ConnectionRole role)
	: m_ID(l_JsonRpcConnectionNextID++), m_Identity(identity), m_Authenticated(authenticated), m_Stream(stream),
	  m_Role(role), m_Timestamp(Utility::GetTime()), m_Seen(Utility::GetTime()),
	  m_NextHeartbeat(0), m_HeartbeatTimeout(0)
{
	boost::call_once(l_JsonRpcConnectionOnceFlag, &JsonRpcConnection::StaticInitialize);

	if (authenticated)
		m_Endpoint = Endpoint::GetByName(identity);
}

void JsonRpcConnection::StaticInitialize(void)
{
	l_JsonRpcConnectionTimeoutTimer = new Timer();
	l_JsonRpcConnectionTimeoutTimer->OnTimerExpired.connect(std::bind(&JsonRpcConnection::TimeoutTimerHandler));
	l_JsonRpcConnectionTimeoutTimer->SetInterval(15);
	l_JsonRpcConnectionTimeoutTimer->Start();

	l_JsonRpcConnectionWorkQueueCount = Application::GetConcurrency();
	l_JsonRpcConnectionWorkQueues = new WorkQueue[l_JsonRpcConnectionWorkQueueCount];

	for (size_t i = 0; i < l_JsonRpcConnectionWorkQueueCount; i++) {
		l_JsonRpcConnectionWorkQueues[i].SetName("JsonRpcConnection, #" + Convert::ToString(i));
	}
}

void JsonRpcConnection::Start(void)
{
	/* the stream holds an owning reference to this object through the callback we're registering here */
	m_Stream->RegisterDataHandler(std::bind(&JsonRpcConnection::DataAvailableHandler, JsonRpcConnection::Ptr(this)));
	if (m_Stream->IsDataAvailable())
		DataAvailableHandler();
}

double JsonRpcConnection::GetTimestamp(void) const
{
	return m_Timestamp;
}

String JsonRpcConnection::GetIdentity(void) const
{
	return m_Identity;
}

bool JsonRpcConnection::IsAuthenticated(void) const
{
	return m_Authenticated;
}

Endpoint::Ptr JsonRpcConnection::GetEndpoint(void) const
{
	return m_Endpoint;
}

TlsStream::Ptr JsonRpcConnection::GetStream(void) const
{
	return m_Stream;
}

ConnectionRole JsonRpcConnection::GetRole(void) const
{
	return m_Role;
}

void JsonRpcConnection::SendMessage(const Dictionary::Ptr& message)
{
	try {
		ObjectLock olock(m_Stream);
		if (m_Stream->IsEof())
			return;
		JsonRpc::SendMessage(m_Stream, message);
	} catch (const std::exception& ex) {
		std::ostringstream info;
		info << "Error while sending JSON-RPC message for identity '" << m_Identity << "'";
		Log(LogWarning, "JsonRpcConnection")
		    << info.str() << "\n" << DiagnosticInformation(ex);

		Disconnect();
	}
}

void JsonRpcConnection::Disconnect(void)
{
	Log(LogWarning, "JsonRpcConnection")
	    << "API client disconnected for identity '" << m_Identity << "'";

	m_Stream->Close();

	if (m_Endpoint)
		m_Endpoint->RemoveClient(this);
	else {
		ApiListener::Ptr listener = ApiListener::GetInstance();
		listener->RemoveAnonymousClient(this);
	}
}

void JsonRpcConnection::MessageHandlerWrapper(const String& jsonString)
{
	if (m_Stream->IsEof())
		return;

	try {
		MessageHandler(jsonString);
	} catch (const std::exception& ex) {
		Log(LogWarning, "JsonRpcConnection")
		    << "Error while reading JSON-RPC message for identity '" << m_Identity
		    << "': " << DiagnosticInformation(ex);

		Disconnect();

		return;
	}
}

void JsonRpcConnection::MessageHandler(const String& jsonString)
{
	Dictionary::Ptr message = JsonRpc::DecodeMessage(jsonString);

	m_Seen = Utility::GetTime();

	if (m_HeartbeatTimeout != 0)
		m_NextHeartbeat = Utility::GetTime() + m_HeartbeatTimeout;

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

		if (!afunc)
			Log(LogWarning, "JsonRpcConnection")
			    << "Call to non-existent function '" << method << "' from endpoint '" << m_Identity << "'.";
		else
			resultMessage->Set("result", afunc->Invoke(origin, message->Get("params")));
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
		SendMessage(resultMessage);
	}
}

bool JsonRpcConnection::ProcessMessage(void)
{
	String message;

	StreamReadStatus srs = JsonRpc::ReadMessage(m_Stream, &message, m_Context, false);

	if (srs != StatusNewItem)
		return false;

	l_JsonRpcConnectionWorkQueues[m_ID % l_JsonRpcConnectionWorkQueueCount].Enqueue(std::bind(&JsonRpcConnection::MessageHandlerWrapper, JsonRpcConnection::Ptr(this), message));

	return true;
}

void JsonRpcConnection::DataAvailableHandler(void)
{
	bool close = false;

	if (!m_Stream)
		return;

	if (!m_Stream->IsEof()) {
		boost::mutex::scoped_lock lock(m_DataHandlerMutex);

		try {
			while (ProcessMessage())
				; /* empty loop body */
		} catch (const std::exception& ex) {
			Log(LogWarning, "JsonRpcConnection")
			    << "Error while reading JSON-RPC message for identity '" << m_Identity
			    << "': " << DiagnosticInformation(ex);

			Disconnect();

			return;
		}
	} else
		close = true;

	if (close)
		Disconnect();
}

Value SetLogPositionHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	if (!params)
		return Empty;

	double log_position = params->Get("log_position");
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint)
		return Empty;

	if (log_position > endpoint->GetLocalLogPosition())
		endpoint->SetLocalLogPosition(log_position);

	return Empty;
}

void JsonRpcConnection::CheckLiveness(void)
{
	if (m_Seen < Utility::GetTime() - 60 && (!m_Endpoint || !m_Endpoint->GetSyncing())) {
		Log(LogInformation, "JsonRpcConnection")
		    <<  "No messages for identity '" << m_Identity << "' have been received in the last 60 seconds.";
		Disconnect();
	}
}

void JsonRpcConnection::TimeoutTimerHandler(void)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	for (const JsonRpcConnection::Ptr& client : listener->GetAnonymousClients()) {
		client->CheckLiveness();
	}

	for (const Endpoint::Ptr& endpoint : ConfigType::GetObjectsByType<Endpoint>()) {
		for (const JsonRpcConnection::Ptr& client : endpoint->GetClients()) {
			client->CheckLiveness();
		}
	}
}

int JsonRpcConnection::GetWorkQueueCount(void)
{
	return l_JsonRpcConnectionWorkQueueCount;
}

int JsonRpcConnection::GetWorkQueueLength(void)
{
	int itemCount = 0;

	for (int i = 0; i < GetWorkQueueCount(); i++) {
		itemCount += l_JsonRpcConnectionWorkQueues[i].GetLength();
	}

	return itemCount;
}

double JsonRpcConnection::GetWorkQueueRate(void)
{
	double rate = 0.0;
	int count = GetWorkQueueCount();

	/* If this is a standalone environment, we don't have any queues. */
	if (count == 0)
		return 0.0;

	for (int i = 0; i < count; i++) {
		rate += l_JsonRpcConnectionWorkQueues[i].GetTaskCount(60) / 60.0;
	}

	return rate / count;
}

