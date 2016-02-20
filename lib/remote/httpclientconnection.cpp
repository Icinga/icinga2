/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "remote/httpclientconnection.hpp"
#include "remote/base64.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include "base/tcpsocket.hpp"
#include "base/tlsstream.hpp"
#include "base/networkstream.hpp"
#include <boost/smart_ptr/make_shared.hpp>

using namespace icinga;

HttpClientConnection::HttpClientConnection(const String& host, const String& port, bool tls)
	: m_Host(host), m_Port(port), m_Tls(tls)
{ }

void HttpClientConnection::Start(void)
{
	/* Nothing to do here atm. */
}

void HttpClientConnection::Reconnect(void)
{
	if (m_Stream)
		m_Stream->Close();

	m_Context.~StreamReadContext();
	new (&m_Context) StreamReadContext();

	m_Requests.clear();
	m_CurrentResponse.reset();

	TcpSocket::Ptr socket = new TcpSocket();
	socket->Connect(m_Host, m_Port);

	if (m_Tls)
		m_Stream = new TlsStream(socket, m_Host, RoleClient);
	else
		ASSERT(!"Non-TLS HTTP connections not supported.");
		/* m_Stream = new NetworkStream(socket);
		   -- does not currently work because the NetworkStream class doesn't support async I/O */

	/* the stream holds an owning reference to this object through the callback we're registering here */
	m_Stream->RegisterDataHandler(boost::bind(&HttpClientConnection::DataAvailableHandler, HttpClientConnection::Ptr(this), _1));
	if (m_Stream->IsDataAvailable())
		DataAvailableHandler(m_Stream);
}

Stream::Ptr HttpClientConnection::GetStream(void) const
{
	return m_Stream;
}

String HttpClientConnection::GetHost(void) const
{
	return m_Host;
}

String HttpClientConnection::GetPort(void) const
{
	return m_Port;
}

bool HttpClientConnection::GetTls(void) const
{
	return m_Tls;
}

void HttpClientConnection::Disconnect(void)
{
	Log(LogDebug, "HttpClientConnection", "Http client disconnected");

	m_Stream->Shutdown();
}

bool HttpClientConnection::ProcessMessage(void)
{
	bool res;

	if (m_Requests.empty()) {
		m_Stream->Close();
		return false;
	}

	const std::pair<boost::shared_ptr<HttpRequest>, HttpCompletionCallback>& currentRequest = *m_Requests.begin();
	HttpRequest& request = *currentRequest.first.get();
	const HttpCompletionCallback& callback = currentRequest.second;

	if (!m_CurrentResponse)
		m_CurrentResponse = boost::make_shared<HttpResponse>(m_Stream, request);

	boost::shared_ptr<HttpResponse> currentResponse = m_CurrentResponse;
	HttpResponse& response = *currentResponse.get();

	try {
		res = response.Parse(m_Context, false);
	} catch (const std::exception& ex) {
		callback(request, response);

		m_Stream->Shutdown();
		return false;
	}

	if (response.Complete) {
		callback(request, response);

		m_Requests.pop_front();
		m_CurrentResponse.reset();

		return true;
	}

	return res;
}

void HttpClientConnection::DataAvailableHandler(const Stream::Ptr& stream)
{
	ASSERT(stream == m_Stream);

	bool close = false;

	if (!m_Stream->IsEof()) {
		boost::mutex::scoped_lock lock(m_DataHandlerMutex);

		try {
			while (ProcessMessage())
				; /* empty loop body */
		} catch (const std::exception& ex) {
			Log(LogWarning, "HttpClientConnection")
			    << "Error while reading Http response: " << DiagnosticInformation(ex);

			close = true;
			Disconnect();
		}
	} else
		close = true;

	if (close)
		m_Stream->Close();
}

boost::shared_ptr<HttpRequest> HttpClientConnection::NewRequest(void)
{
	Reconnect();
	return boost::make_shared<HttpRequest>(m_Stream);
}

void HttpClientConnection::SubmitRequest(const boost::shared_ptr<HttpRequest>& request,
    const HttpCompletionCallback& callback)
{
	m_Requests.push_back(std::make_pair(request, callback));
	request->Finish();
}

