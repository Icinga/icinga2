/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
#include "base/dynamictype.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include <boost/thread/once.hpp>

using namespace icinga;

static Value SetLogPositionHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);
REGISTER_APIFUNCTION(SetLogPosition, log, &SetLogPositionHandler);
static Value RequestCertificateHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);
REGISTER_APIFUNCTION(RequestCertificate, pki, &RequestCertificateHandler);

static boost::once_flag l_JsonRpcConnectionOnceFlag = BOOST_ONCE_INIT;
static Timer::Ptr l_JsonRpcConnectionTimeoutTimer;

JsonRpcConnection::JsonRpcConnection(const String& identity, bool authenticated, const TlsStream::Ptr& stream, ConnectionRole role)
	: m_Identity(identity), m_Authenticated(authenticated), m_Stream(stream), m_Role(role), m_Seen(Utility::GetTime()),
	  m_NextHeartbeat(0), m_HeartbeatTimeout(0)
{
	boost::call_once(l_JsonRpcConnectionOnceFlag, &JsonRpcConnection::StaticInitialize);

	if (authenticated)
		m_Endpoint = Endpoint::GetByName(identity);
}

void JsonRpcConnection::StaticInitialize(void)
{
	l_JsonRpcConnectionTimeoutTimer = new Timer();
	l_JsonRpcConnectionTimeoutTimer->OnTimerExpired.connect(boost::bind(&JsonRpcConnection::TimeoutTimerHandler));
	l_JsonRpcConnectionTimeoutTimer->SetInterval(15);
	l_JsonRpcConnectionTimeoutTimer->Start();
}

void JsonRpcConnection::Start(void)
{
	m_Stream->RegisterDataHandler(boost::bind(&JsonRpcConnection::DataAvailableHandler, this));
	if (m_Stream->IsDataAvailable())
		DataAvailableHandler();
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
	m_WriteQueue.Enqueue(boost::bind(&JsonRpcConnection::SendMessageSync, JsonRpcConnection::Ptr(this), message));
}

void JsonRpcConnection::SendMessageSync(const Dictionary::Ptr& message)
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
		    << info.str();
		Log(LogDebug, "JsonRpcConnection")
		    << info.str() << "\n" << DiagnosticInformation(ex);

		Disconnect();
	}
}

void JsonRpcConnection::Disconnect(void)
{
	Log(LogWarning, "JsonRpcConnection")
	    << "API client disconnected for identity '" << m_Identity << "'";

	if (m_Endpoint)
		m_Endpoint->RemoveClient(this);
	else {
		ApiListener::Ptr listener = ApiListener::GetInstance();
		listener->RemoveAnonymousClient(this);
	}

	m_Stream->Close();
}

bool JsonRpcConnection::ProcessMessage(void)
{
	Dictionary::Ptr message;

	StreamReadStatus srs = JsonRpc::ReadMessage(m_Stream, &message, m_Context, false);

	if (srs != StatusNewItem)
		return false;

	m_Seen = Utility::GetTime();

	if (m_HeartbeatTimeout != 0)
		m_NextHeartbeat = Utility::GetTime() + m_HeartbeatTimeout;

	if (m_Endpoint && message->Contains("ts")) {
		double ts = message->Get("ts");

		/* ignore old messages */
		if (ts < m_Endpoint->GetRemoteLogPosition())
			return true;

		m_Endpoint->SetRemoteLogPosition(ts);
	}

	MessageOrigin origin;
	origin.FromClient = this;

	if (m_Endpoint) {
		if (m_Endpoint->GetZone() != Zone::GetLocalZone())
			origin.FromZone = m_Endpoint->GetZone();
		else
			origin.FromZone = Zone::GetByName(message->Get("originZone"));
	}

	String method = message->Get("method");

	Log(LogNotice, "JsonRpcConnection")
	    << "Received '" << method << "' message from '" << m_Identity << "'";

	Dictionary::Ptr resultMessage = new Dictionary();

	try {
		ApiFunction::Ptr afunc = ApiFunction::GetByName(method);

		if (!afunc)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Function '" + method + "' does not exist."));

		resultMessage->Set("result", afunc->Invoke(origin, message->Get("params")));
	} catch (const std::exception& ex) {
		//TODO: Add a user readable error message for the remote caller
		resultMessage->Set("error", DiagnosticInformation(ex));
		std::ostringstream info;
		info << "Error while processing message for identity '" << m_Identity << "'";
		Log(LogWarning, "JsonRpcConnection")
		    << info.str();
		Log(LogDebug, "JsonRpcConnection")
		    << info.str() << "\n" << DiagnosticInformation(ex);
	}

	if (message->Contains("id")) {
		resultMessage->Set("jsonrpc", "2.0");
		resultMessage->Set("id", message->Get("id"));
		JsonRpc::SendMessage(m_Stream, resultMessage);
	}

	return true;
}

void JsonRpcConnection::DataAvailableHandler(void)
{
	boost::mutex::scoped_lock lock(m_DataHandlerMutex);

	try {
		while (ProcessMessage())
			; /* empty loop body */
	} catch (const std::exception& ex) {
		Log(LogWarning, "JsonRpcConnection")
		    << "Error while reading JSON-RPC message for identity '" << m_Identity << "': " << DiagnosticInformation(ex);

		Disconnect();
	}
}

Value SetLogPositionHandler(const MessageOrigin& origin, const Dictionary::Ptr& params)
{
	if (!params)
		return Empty;

	double log_position = params->Get("log_position");
	Endpoint::Ptr endpoint = origin.FromClient->GetEndpoint();

	if (!endpoint)
		return Empty;

	if (log_position > endpoint->GetLocalLogPosition())
		endpoint->SetLocalLogPosition(log_position);

	return Empty;
}

Value RequestCertificateHandler(const MessageOrigin& origin, const Dictionary::Ptr& params)
{
	if (!params)
		return Empty;

	Dictionary::Ptr result = new Dictionary();

	if (!origin.FromClient->IsAuthenticated()) {
		ApiListener::Ptr listener = ApiListener::GetInstance();
		String salt = listener->GetTicketSalt();

		if (salt.IsEmpty()) {
			result->Set("error", "Ticket salt is not configured.");
			return result;
		}

		String ticket = params->Get("ticket");
		String realTicket = PBKDF2_SHA1(origin.FromClient->GetIdentity(), salt, 50000);

		if (ticket != realTicket) {
			result->Set("error", "Invalid ticket.");
			return result;
		}
	}

	boost::shared_ptr<X509> cert = origin.FromClient->GetStream()->GetPeerCertificate();

	EVP_PKEY *pubkey = X509_get_pubkey(cert.get());
	X509_NAME *subject = X509_get_subject_name(cert.get());

	boost::shared_ptr<X509> newcert = CreateCertIcingaCA(pubkey, subject);
	result->Set("cert", CertificateToString(newcert));

	String cacertfile = GetIcingaCADir() + "/ca.crt";
	boost::shared_ptr<X509> cacert = GetX509Certificate(cacertfile);
	result->Set("ca", CertificateToString(cacert));

	return result;
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

	BOOST_FOREACH(const JsonRpcConnection::Ptr& client, listener->GetAnonymousClients()) {
		client->CheckLiveness();
	}

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjectsByType<Endpoint>()) {
		BOOST_FOREACH(const JsonRpcConnection::Ptr& client, endpoint->GetClients()) {
			client->CheckLiveness();
		}
	}
}
