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

#include "remote/httpconnection.hpp"
#include "remote/httphandler.hpp"
#include "remote/apilistener.hpp"
#include "remote/apifunction.hpp"
#include "remote/jsonrpc.hpp"
#include "remote/base64.hpp"
#include "base/dynamictype.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include <boost/thread/once.hpp>

using namespace icinga;

static boost::once_flag l_HttpConnectionOnceFlag = BOOST_ONCE_INIT;
static Timer::Ptr l_HttpConnectionTimeoutTimer;

HttpConnection::HttpConnection(const String& identity, bool authenticated, const TlsStream::Ptr& stream)
	: m_Stream(stream), m_Seen(Utility::GetTime()),
	  m_CurrentRequest(m_Context), m_PendingRequests(0)
{
	boost::call_once(l_HttpConnectionOnceFlag, &HttpConnection::StaticInitialize);

	if (authenticated)
		m_ApiUser = ApiUser::GetByClientCN(identity);
}

void HttpConnection::StaticInitialize(void)
{
	l_HttpConnectionTimeoutTimer = new Timer();
	l_HttpConnectionTimeoutTimer->OnTimerExpired.connect(boost::bind(&HttpConnection::TimeoutTimerHandler));
	l_HttpConnectionTimeoutTimer->SetInterval(15);
	l_HttpConnectionTimeoutTimer->Start();
}

void HttpConnection::Start(void)
{
	m_Stream->RegisterDataHandler(boost::bind(&HttpConnection::DataAvailableHandler, this));
	if (m_Stream->IsDataAvailable())
		DataAvailableHandler();
}

ApiUser::Ptr HttpConnection::GetApiUser(void) const
{
	return m_ApiUser;
}

TlsStream::Ptr HttpConnection::GetStream(void) const
{
	return m_Stream;
}

void HttpConnection::Disconnect(void)
{
	Log(LogDebug, "HttpConnection", "Http client disconnected");

	ApiListener::Ptr listener = ApiListener::GetInstance();
	listener->RemoveHttpClient(this);

	m_Stream->Shutdown();
}

bool HttpConnection::ProcessMessage(void)
{
	bool res;

	try {
		res = m_CurrentRequest.Parse(m_Stream, m_Context, false);
	} catch (const std::exception& ex) {
		HttpResponse response(m_Stream, m_CurrentRequest);
		response.SetStatus(400, "Bad request");
		String msg = "<h1>Bad request</h1>";
		response.WriteBody(msg.CStr(), msg.GetLength());
		response.Finish();

		m_Stream->Shutdown();
		return false;
	}

	if (m_CurrentRequest.Complete) {
		m_RequestQueue.Enqueue(boost::bind(&HttpConnection::ProcessMessageAsync, HttpConnection::Ptr(this), m_CurrentRequest));

		m_Seen = Utility::GetTime();
		m_PendingRequests++;

		m_CurrentRequest.~HttpRequest();
		new (&m_CurrentRequest) HttpRequest(m_Context);

		return true;
	}

	return res;
}

void HttpConnection::ProcessMessageAsync(HttpRequest& request)
{
	Log(LogInformation, "HttpConnection", "Processing Http message");

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

		if (!user || !user->CheckPassword(password))
			user.reset();
	}

	HttpResponse response(m_Stream, request);

	if (!user) {
		response.SetStatus(401, "Unauthorized");
		response.AddHeader("WWW-Authenticate", "Basic realm=\"Icinga 2\"");
		String msg = "<h1>Unauthorized</h1>";
		response.WriteBody(msg.CStr(), msg.GetLength());
	} else {
		HttpHandler::ProcessRequest(user, request, response);
	}

	response.Finish();

	m_PendingRequests--;
}

void HttpConnection::DataAvailableHandler(void)
{
	boost::mutex::scoped_lock lock(m_DataHandlerMutex);

	try {
		while (ProcessMessage())
			; /* empty loop body */
	} catch (const std::exception& ex) {
		Log(LogWarning, "HttpConnection")
		    << "Error while reading Http request: " << DiagnosticInformation(ex);

		Disconnect();
	}
}

void HttpConnection::CheckLiveness(void)
{
	if (m_Seen < Utility::GetTime() - 10 && m_PendingRequests == 0) {
		Log(LogInformation, "HttpConnection")
		    <<  "No messages for Http connection have been received in the last 10 seconds.";
		Disconnect();
	}
}

void HttpConnection::TimeoutTimerHandler(void)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	BOOST_FOREACH(const HttpConnection::Ptr& client, listener->GetHttpClients()) {
		client->CheckLiveness();
	}
}
