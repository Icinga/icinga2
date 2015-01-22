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

#include "remote/apiclient.hpp"
#include "remote/apilistener.hpp"
#include "remote/apifunction.hpp"
#include "remote/jsonrpc.hpp"
#include "base/dynamictype.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"

using namespace icinga;

static Value SetLogPositionHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);
REGISTER_APIFUNCTION(SetLogPosition, log, &SetLogPositionHandler);
static Value RequestCertificateHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);
REGISTER_APIFUNCTION(RequestCertificate, pki, &RequestCertificateHandler);

ApiClient::ApiClient(const String& identity, bool authenticated, const TlsStream::Ptr& stream, ConnectionRole role)
	: m_Identity(identity), m_Authenticated(authenticated), m_Stream(stream), m_Role(role), m_Seen(Utility::GetTime()),
	  m_NextHeartbeat(0)
{
	if (authenticated)
		m_Endpoint = Endpoint::GetByName(identity);
}

void ApiClient::Start(void)
{
	boost::thread thread(boost::bind(&ApiClient::MessageThreadProc, ApiClient::Ptr(this)));
	thread.detach();
}

String ApiClient::GetIdentity(void) const
{
	return m_Identity;
}

bool ApiClient::IsAuthenticated(void) const
{
	return m_Authenticated;
}

Endpoint::Ptr ApiClient::GetEndpoint(void) const
{
	return m_Endpoint;
}

TlsStream::Ptr ApiClient::GetStream(void) const
{
	return m_Stream;
}

ConnectionRole ApiClient::GetRole(void) const
{
	return m_Role;
}

void ApiClient::SendMessage(const Dictionary::Ptr& message)
{
	if (m_WriteQueue.GetLength() > 20000) {
		Log(LogWarning, "remote")
		    << "Closing connection for API identity '" << m_Identity << "': Too many queued messages.";
		Disconnect();
		return;
	}

	m_WriteQueue.Enqueue(boost::bind(&ApiClient::SendMessageSync, ApiClient::Ptr(this), message));
}

void ApiClient::SendMessageSync(const Dictionary::Ptr& message)
{
	try {
		ObjectLock olock(m_Stream);
		if (m_Stream->IsEof())
			return;
		JsonRpc::SendMessage(m_Stream, message);
		if (message->Get("method") != "log::SetLogPosition")
			m_Seen = Utility::GetTime();
	} catch (const std::exception& ex) {
		std::ostringstream info;
		info << "Error while sending JSON-RPC message for identity '" << m_Identity << "'";
		Log(LogWarning, "ApiClient")
		    << info.str();
		Log(LogDebug, "ApiClient")
		    << info.str() << "\n" << DiagnosticInformation(ex);

		Disconnect();
	}
}

void ApiClient::Disconnect(void)
{
	Utility::QueueAsyncCallback(boost::bind(&ApiClient::DisconnectSync, ApiClient::Ptr(this)));
}

void ApiClient::DisconnectSync(void)
{
	Log(LogWarning, "ApiClient")
	    << "API client disconnected for identity '" << m_Identity << "'";

	if (m_Endpoint)
		m_Endpoint->RemoveClient(this);
	else {
		ApiListener::Ptr listener = ApiListener::GetInstance();
		listener->RemoveAnonymousClient(this);
	}

	m_Stream->Close();
}

bool ApiClient::ProcessMessage(void)
{
	Dictionary::Ptr message;

	if (m_Stream->IsEof())
		return false;

	try {
		message = JsonRpc::ReadMessage(m_Stream);
	} catch (const openssl_error& ex) {
		const unsigned long *pe = boost::get_error_info<errinfo_openssl_error>(ex);

		if (pe && *pe == 0)
			return false; /* Connection was closed cleanly */

		throw;
	}

	if (!message)
		return false;

	if (message->Get("method") != "log::SetLogPosition")
		m_Seen = Utility::GetTime();

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

	Log(LogNotice, "ApiClient")
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
		Log(LogWarning, "ApiClient")
		    << info.str();
		Log(LogDebug, "ApiClient")
		    << info.str() << "\n" << DiagnosticInformation(ex);
	}

	if (message->Contains("id")) {
		resultMessage->Set("jsonrpc", "2.0");
		resultMessage->Set("id", message->Get("id"));
		JsonRpc::SendMessage(m_Stream, resultMessage);
	}

	return true;
}

void ApiClient::MessageThreadProc(void)
{
	Utility::SetThreadName("API Client");

	try {
		while (ProcessMessage())
			; /* empty loop body */
	} catch (const std::exception& ex) {
		Log(LogWarning, "ApiClient")
		    << "Error while reading JSON-RPC message for identity '" << m_Identity << "': " << DiagnosticInformation(ex);
	}

	Disconnect();
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

	ApiListener::Ptr listener = ApiListener::GetInstance();
	String salt = listener->GetTicketSalt();

	Dictionary::Ptr result = new Dictionary();

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
