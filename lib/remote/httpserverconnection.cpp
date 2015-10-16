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

#include "remote/httpserverconnection.hpp"
#include "remote/httphandler.hpp"
#include "remote/apilistener.hpp"
#include "remote/apifunction.hpp"
#include "remote/jsonrpc.hpp"
#include "remote/base64.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include <boost/thread/once.hpp>

using namespace icinga;

static boost::once_flag l_HttpServerConnectionOnceFlag = BOOST_ONCE_INIT;
static Timer::Ptr l_HttpServerConnectionTimeoutTimer;

HttpServerConnection::HttpServerConnection(const String& identity, bool authenticated, const TlsStream::Ptr& stream)
	: m_Stream(stream), m_CurrentRequest(stream), m_Seen(Utility::GetTime()), m_PendingRequests(0)
{
	boost::call_once(l_HttpServerConnectionOnceFlag, &HttpServerConnection::StaticInitialize);

	if (authenticated)
		m_ApiUser = ApiUser::GetByClientCN(identity);
}

void HttpServerConnection::StaticInitialize(void)
{
	l_HttpServerConnectionTimeoutTimer = new Timer();
	l_HttpServerConnectionTimeoutTimer->OnTimerExpired.connect(boost::bind(&HttpServerConnection::TimeoutTimerHandler));
	l_HttpServerConnectionTimeoutTimer->SetInterval(15);
	l_HttpServerConnectionTimeoutTimer->Start();
}

void HttpServerConnection::Start(void)
{
	m_Stream->RegisterDataHandler(boost::bind(&HttpServerConnection::DataAvailableHandler, this));
	if (m_Stream->IsDataAvailable())
		DataAvailableHandler();
}

ApiUser::Ptr HttpServerConnection::GetApiUser(void) const
{
	return m_ApiUser;
}

TlsStream::Ptr HttpServerConnection::GetStream(void) const
{
	return m_Stream;
}

void HttpServerConnection::Disconnect(void)
{
	Log(LogDebug, "HttpServerConnection", "Http client disconnected");

	ApiListener::Ptr listener = ApiListener::GetInstance();
	listener->RemoveHttpClient(this);

	m_Stream->Shutdown();
}

bool HttpServerConnection::ProcessMessage(void)
{
	bool res;

	try {
		res = m_CurrentRequest.Parse(m_Context, false);
	} catch (const std::exception& ex) {
		HttpResponse response(m_Stream, m_CurrentRequest);
		response.SetStatus(400, "Bad request");
		String msg = "<h1>Bad request</h1><p><pre>" + DiagnosticInformation(ex) + "</pre></p>";
		response.WriteBody(msg.CStr(), msg.GetLength());
		response.Finish();

		m_Stream->Shutdown();
		return false;
	}

	if (m_CurrentRequest.Complete) {
		m_RequestQueue.Enqueue(boost::bind(&HttpServerConnection::ProcessMessageAsync,
		    HttpServerConnection::Ptr(this), m_CurrentRequest));

		m_Seen = Utility::GetTime();
		m_PendingRequests++;

		m_CurrentRequest.~HttpRequest();
		new (&m_CurrentRequest) HttpRequest(m_Stream);

		return true;
	}

	return res;
}

void HttpServerConnection::ProcessMessageAsync(HttpRequest& request)
{
	String auth_header = request.Headers->Get("authorization");

	String::SizeType pos = auth_header.FindFirstOf(" ");
	String username, password;

	if (pos != String::NPos && auth_header.SubStr(0, pos) == "Basic") {
		String credentials_base64 = auth_header.SubStr(pos + 1);
		String credentials = Base64::Decode(credentials_base64);

		String::SizeType cpos = credentials.FindFirstOf(":");

		if (cpos != String::NPos) {
			username = credentials.SubStr(0, cpos);
			password = credentials.SubStr(cpos + 1);
		}
	}

	ApiUser::Ptr user;

	if (m_ApiUser)
		user = m_ApiUser;
	else {
		user = ApiUser::GetByName(username);

		if (user && user->GetPassword() != password)
			user.reset();
	}

        String requestUrl = request.RequestUrl->Format();

	Log(LogInformation, "HttpServerConnection")
	    << "Request: " << request.RequestMethod << " " << requestUrl
	    << " (" << (user ? user->GetName() : "<unauthenticated>") << ")";

	HttpResponse response(m_Stream, request);

	if (!user) {
		Log(LogWarning, "HttpServerConnection")
		    << "Unauthorized request: " << request.RequestMethod << " " << requestUrl;
		response.SetStatus(401, "Unauthorized");
		response.AddHeader("Content-Type", "text/html");
		response.AddHeader("WWW-Authenticate", "Basic realm=\"Icinga 2\"");
		String msg = "<h1>Unauthorized</h1>";
		response.WriteBody(msg.CStr(), msg.GetLength());
	} else {
		try {
			HttpHandler::ProcessRequest(user, request, response);
		} catch (const std::exception& ex) {
			Log(LogCritical, "HttpServerConnection")
			    << "Unhandled exception while processing Http request: " << DiagnosticInformation(ex);
			response.SetStatus(503, "Unhandled exception");
			response.AddHeader("Content-Type", "text/plain");
			String errorInfo = DiagnosticInformation(ex);
			response.WriteBody(errorInfo.CStr(), errorInfo.GetLength());
		}
	}

	response.Finish();

	m_PendingRequests--;
}

void HttpServerConnection::DataAvailableHandler(void)
{
	boost::mutex::scoped_lock lock(m_DataHandlerMutex);

	try {
		while (ProcessMessage())
			; /* empty loop body */
	} catch (const std::exception& ex) {
		Log(LogWarning, "HttpServerConnection")
		    << "Error while reading Http request: " << DiagnosticInformation(ex);

		Disconnect();
	}
}

void HttpServerConnection::CheckLiveness(void)
{
	if (m_Seen < Utility::GetTime() - 10 && m_PendingRequests == 0) {
		Log(LogInformation, "HttpServerConnection")
		    <<  "No messages for Http connection have been received in the last 10 seconds.";
		Disconnect();
	}
}

void HttpServerConnection::TimeoutTimerHandler(void)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	BOOST_FOREACH(const HttpServerConnection::Ptr& client, listener->GetHttpClients()) {
		client->CheckLiveness();
	}
}

