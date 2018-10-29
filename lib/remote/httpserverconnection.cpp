/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#include "remote/httpserverconnection.hpp"
#include "remote/httphandler.hpp"
#include "remote/httputility.hpp"
#include "remote/apilistener.hpp"
#include "remote/apifunction.hpp"
#include "remote/jsonrpc.hpp"
#include "base/base64.hpp"
#include "base/convert.hpp"
#include "base/configtype.hpp"
#include "base/exception.hpp"
#include "base/logger.hpp"
#include "base/objectlock.hpp"
#include "base/timer.hpp"
#include "base/utility.hpp"
#include <boost/thread/once.hpp>

using namespace icinga;

static boost::once_flag l_HttpServerConnectionOnceFlag = BOOST_ONCE_INIT;
static Timer::Ptr l_HttpServerConnectionTimeoutTimer;

HttpServerConnection::HttpServerConnection(const String& identity, bool authenticated, const TlsStream::Ptr& stream)
	: m_Stream(stream), m_Seen(Utility::GetTime()), m_CurrentRequest(stream), m_PendingRequests(0)
{
	boost::call_once(l_HttpServerConnectionOnceFlag, &HttpServerConnection::StaticInitialize);

	m_RequestQueue.SetName("HttpServerConnection");

	if (authenticated)
		m_ApiUser = ApiUser::GetByClientCN(identity);

	/* Cache the peer address. */
	m_PeerAddress = "<unknown>";

	if (stream) {
		Socket::Ptr socket = m_Stream->GetSocket();

		if (socket) {
			m_PeerAddress = socket->GetPeerAddress();
		}
	}
}

void HttpServerConnection::StaticInitialize()
{
	l_HttpServerConnectionTimeoutTimer = new Timer();
	l_HttpServerConnectionTimeoutTimer->OnTimerExpired.connect(std::bind(&HttpServerConnection::TimeoutTimerHandler));
	l_HttpServerConnectionTimeoutTimer->SetInterval(5);
	l_HttpServerConnectionTimeoutTimer->Start();
}

void HttpServerConnection::Start()
{
	/* the stream holds an owning reference to this object through the callback we're registering here */
	m_Stream->RegisterDataHandler(std::bind(&HttpServerConnection::DataAvailableHandler, HttpServerConnection::Ptr(this)));
	if (m_Stream->IsDataAvailable())
		DataAvailableHandler();
}

ApiUser::Ptr HttpServerConnection::GetApiUser() const
{
	return m_ApiUser;
}

TlsStream::Ptr HttpServerConnection::GetStream() const
{
	return m_Stream;
}

void HttpServerConnection::Disconnect()
{
	boost::recursive_mutex::scoped_try_lock lock(m_DataHandlerMutex);
	if (!lock.owns_lock()) {
		Log(LogInformation, "HttpServerConnection", "Unable to disconnect Http client, I/O thread busy");
		return;
	}

	Log(LogInformation, "HttpServerConnection")
		<< "HTTP client disconnected (from " << m_PeerAddress << ")";

	ApiListener::Ptr listener = ApiListener::GetInstance();
	listener->RemoveHttpClient(this);

	m_CurrentRequest.~HttpRequest();
	new (&m_CurrentRequest) HttpRequest(nullptr);

	m_Stream->Close();
}

bool HttpServerConnection::ProcessMessage()
{
	bool res;
	HttpResponse response(m_Stream, m_CurrentRequest);

	if (!m_CurrentRequest.CompleteHeaders) {
		try {
			res = m_CurrentRequest.ParseHeaders(m_Context, false);
		} catch (const std::invalid_argument& ex) {
			response.SetStatus(400, "Bad Request");
			String msg = String("<h1>Bad Request</h1><p><pre>") + ex.what() + "</pre></p>";
			response.WriteBody(msg.CStr(), msg.GetLength());
			response.Finish();

			m_CurrentRequest.~HttpRequest();
			new (&m_CurrentRequest) HttpRequest(m_Stream);

			m_Stream->Shutdown();

			return false;
		} catch (const std::exception& ex) {
			response.SetStatus(500, "Internal Server Error");
			String msg = "<h1>Internal Server Error</h1><p><pre>" + DiagnosticInformation(ex) + "</pre></p>";
			response.WriteBody(msg.CStr(), msg.GetLength());
			response.Finish();

			m_CurrentRequest.~HttpRequest();
			new (&m_CurrentRequest) HttpRequest(m_Stream);

			m_Stream->Shutdown();

			return false;
		}
		return res;
	}

	if (!m_CurrentRequest.CompleteHeaderCheck) {
		m_CurrentRequest.CompleteHeaderCheck = true;
		if (!ManageHeaders(response)) {
			m_CurrentRequest.~HttpRequest();
			new (&m_CurrentRequest) HttpRequest(m_Stream);

			m_Stream->Shutdown();

			return false;
		}
	}

	if (!m_CurrentRequest.CompleteBody) {
		try {
			res = m_CurrentRequest.ParseBody(m_Context, false);
		} catch (const std::invalid_argument& ex) {
			response.SetStatus(400, "Bad Request");
			String msg = String("<h1>Bad Request</h1><p><pre>") + ex.what() + "</pre></p>";
			response.WriteBody(msg.CStr(), msg.GetLength());
			response.Finish();

			m_CurrentRequest.~HttpRequest();
			new (&m_CurrentRequest) HttpRequest(m_Stream);

			m_Stream->Shutdown();

			return false;
		} catch (const std::exception& ex) {
			response.SetStatus(500, "Internal Server Error");
			String msg = "<h1>Internal Server Error</h1><p><pre>" + DiagnosticInformation(ex) + "</pre></p>";
			response.WriteBody(msg.CStr(), msg.GetLength());
			response.Finish();

			m_CurrentRequest.~HttpRequest();
			new (&m_CurrentRequest) HttpRequest(m_Stream);

			m_Stream->Shutdown();

			return false;
		}
		return res;
	}

	m_RequestQueue.Enqueue(std::bind(&HttpServerConnection::ProcessMessageAsync,
		HttpServerConnection::Ptr(this), m_CurrentRequest, response, m_AuthenticatedUser));

	m_Seen = Utility::GetTime();
	m_PendingRequests++;

	m_CurrentRequest.~HttpRequest();
	new (&m_CurrentRequest) HttpRequest(m_Stream);

	return false;
}

bool HttpServerConnection::ManageHeaders(HttpResponse& response)
{
	if (m_CurrentRequest.Headers->Get("expect") == "100-continue") {
		String continueResponse = "HTTP/1.1 100 Continue\r\n\r\n";
		m_Stream->Write(continueResponse.CStr(), continueResponse.GetLength());
	}

	/* client_cn matched. */
	if (m_ApiUser)
		m_AuthenticatedUser = m_ApiUser;
	else
		m_AuthenticatedUser = ApiUser::GetByAuthHeader(m_CurrentRequest.Headers->Get("authorization"));

	String requestUrl = m_CurrentRequest.RequestUrl->Format();

	Log(LogInformation, "HttpServerConnection")
		<< "Request: " << m_CurrentRequest.RequestMethod << " " << requestUrl
		<< " (from " << m_PeerAddress << ")"
		<< ", user: " << (m_AuthenticatedUser ? m_AuthenticatedUser->GetName() : "<unauthenticated>") << ")";

	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return false;

	Array::Ptr headerAllowOrigin = listener->GetAccessControlAllowOrigin();

	if (headerAllowOrigin && headerAllowOrigin->GetLength() != 0) {
		String origin = m_CurrentRequest.Headers->Get("origin");
		{
			ObjectLock olock(headerAllowOrigin);

			for (const String& allowedOrigin : headerAllowOrigin) {
				if (allowedOrigin == origin)
					response.AddHeader("Access-Control-Allow-Origin", origin);
			}
		}

		response.AddHeader("Access-Control-Allow-Credentials", "true");

		String accessControlRequestMethodHeader = m_CurrentRequest.Headers->Get("access-control-request-method");

		if (m_CurrentRequest.RequestMethod == "OPTIONS" && !accessControlRequestMethodHeader.IsEmpty()) {
			response.SetStatus(200, "OK");

			response.AddHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
			response.AddHeader("Access-Control-Allow-Headers", "Authorization, X-HTTP-Method-Override");

			String msg = "Preflight OK";
			response.WriteBody(msg.CStr(), msg.GetLength());

			response.Finish();
			return false;
		}
	}

	if (m_CurrentRequest.RequestMethod != "GET" && m_CurrentRequest.Headers->Get("accept") != "application/json") {
		response.SetStatus(400, "Wrong Accept header");
		response.AddHeader("Content-Type", "text/html");
		String msg = "<h1>Accept header is missing or not set to 'application/json'.</h1>";
		response.WriteBody(msg.CStr(), msg.GetLength());
		response.Finish();
		return false;
	}

	if (!m_AuthenticatedUser) {
		Log(LogWarning, "HttpServerConnection")
			<< "Unauthorized request: " << m_CurrentRequest.RequestMethod << " " << requestUrl;

		response.SetStatus(401, "Unauthorized");
		response.AddHeader("WWW-Authenticate", "Basic realm=\"Icinga 2\"");

		if (m_CurrentRequest.Headers->Get("accept") == "application/json") {
			Dictionary::Ptr result = new Dictionary({
				{ "error", 401 },
				{ "status", "Unauthorized. Please check your user credentials." }
			});

			HttpUtility::SendJsonBody(response, nullptr, result);
		} else {
			response.AddHeader("Content-Type", "text/html");
			String msg = "<h1>Unauthorized. Please check your user credentials.</h1>";
			response.WriteBody(msg.CStr(), msg.GetLength());
		}

		response.Finish();
		return false;
	}

	static const size_t defaultContentLengthLimit = 1 * 1024 * 1024;
	size_t maxSize = defaultContentLengthLimit;

	Array::Ptr permissions = m_AuthenticatedUser->GetPermissions();

	if (permissions) {
		ObjectLock olock(permissions);

		for (const Value& permissionInfo : permissions) {
			String permission;

			if (permissionInfo.IsObjectType<Dictionary>())
				permission = static_cast<Dictionary::Ptr>(permissionInfo)->Get("permission");
			else
				permission = permissionInfo;

			static std::vector<std::pair<String, size_t>> specialContentLengthLimits {
				  { "config/modify", 512 * 1024 * 1024 }
			};

			for (const auto& limitInfo : specialContentLengthLimits) {
				if (limitInfo.second <= maxSize)
					continue;

				if (Utility::Match(permission, limitInfo.first))
					maxSize = limitInfo.second;
			}
		}
	}

	size_t contentLength = m_CurrentRequest.Headers->Get("content-length");

	if (contentLength > maxSize) {
		response.SetStatus(400, "Bad Request");
		String msg = String("<h1>Content length exceeded maximum</h1>");
		response.WriteBody(msg.CStr(), msg.GetLength());
		response.Finish();

		return false;
	}

	return true;
}

void HttpServerConnection::ProcessMessageAsync(HttpRequest& request, HttpResponse& response, const ApiUser::Ptr& user)
{
	response.RebindRequest(request);

	try {
		HttpHandler::ProcessRequest(user, request, response);
	} catch (const std::exception& ex) {
		Log(LogCritical, "HttpServerConnection")
			<< "Unhandled exception while processing Http request: " << DiagnosticInformation(ex);
		HttpUtility::SendJsonError(response, nullptr, 503, "Unhandled exception" , DiagnosticInformation(ex));
	}

	response.Finish();
	m_PendingRequests--;
}

void HttpServerConnection::DataAvailableHandler()
{
	bool close = false;

	if (!m_Stream->IsEof()) {
		boost::recursive_mutex::scoped_lock lock(m_DataHandlerMutex);

		try {
			while (ProcessMessage())
				; /* empty loop body */
		} catch (const std::exception& ex) {
			Log(LogWarning, "HttpServerConnection")
				<< "Error while reading Http request: " << DiagnosticInformation(ex);

			close = true;
		}

		/* Request finished, decide whether to explicitly close the connection. */
		if (m_CurrentRequest.ProtocolVersion == HttpVersion10 ||
			m_CurrentRequest.Headers->Get("connection") == "close") {
			m_Stream->Shutdown();
			close = true;
		}
	} else
		close = true;

	if (close)
		Disconnect();
}

void HttpServerConnection::CheckLiveness()
{
	if (m_Seen < Utility::GetTime() - 10 && m_PendingRequests == 0 && m_Stream->IsEof()) {
		Log(LogInformation, "HttpServerConnection")
			<<  "No messages for Http connection have been received in the last 10 seconds.";
		Disconnect();
	}
}

void HttpServerConnection::TimeoutTimerHandler()
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	for (const HttpServerConnection::Ptr& client : listener->GetHttpClients()) {
		client->CheckLiveness();
	}
}

