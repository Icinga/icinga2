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

#include "remote/httpresponse.hpp"
#include "remote/httpchunkedencoding.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/convert.hpp"

using namespace icinga;

HttpResponse::HttpResponse(const Stream::Ptr& stream, const HttpRequest& request)
    : m_State(HttpResponseStart), m_Request(request), m_Stream(stream)
{ }

void HttpResponse::SetStatus(int code, const String& message)
{
	ASSERT(m_State == HttpResponseStart);
	ASSERT(code >= 100 && code <= 599);
	ASSERT(!message.IsEmpty());

	String status = "HTTP/";

	if (m_Request.ProtocolVersion == HttpVersion10)
		status += "1.0";
	else
		status += "1.1";

	status += " " + Convert::ToString(code) + " " + message + "\r\n";

	m_Stream->Write(status.CStr(), status.GetLength());

	m_State = HttpResponseHeaders;
}

void HttpResponse::AddHeader(const String& key, const String& value)
{
	ASSERT(m_State = HttpResponseHeaders);
	String header = key + ": " + value + "\r\n";
	m_Stream->Write(header.CStr(), header.GetLength());
}

void HttpResponse::FinishHeaders(void)
{
	if (m_State == HttpResponseHeaders) {
		if (m_Request.ProtocolVersion == HttpVersion11)
			AddHeader("Transfer-Encoding", "chunked");
 
		AddHeader("Server", "Icinga/" + Application::GetVersion());
		m_Stream->Write("\r\n", 2);
		m_State = HttpResponseBody;
	}
}

void HttpResponse::WriteBody(const char *data, size_t count)
{
	ASSERT(m_State == HttpResponseHeaders || m_State == HttpResponseBody);

	if (m_Request.ProtocolVersion == HttpVersion10) {
		if (!m_Body)
			m_Body = new FIFO();

		m_Body->Write(data, count);
	} else {
		FinishHeaders();

		HttpChunkedEncoding::WriteChunkToStream(m_Stream, data, count);
	}
}

void HttpResponse::Finish(void)
{
	ASSERT(m_State != HttpResponseEnd);
	m_State = HttpResponseEnd;

	if (m_Request.ProtocolVersion == HttpVersion10) {
		if (m_Body)
			AddHeader("Content-Length", Convert::ToString(m_Body->GetAvailableBytes()));

		FinishHeaders();

		while (m_Body && m_Body->IsDataAvailable()) {
			char buffer[1024];
			size_t rc = m_Body->Read(buffer, sizeof(buffer), true);
			m_Stream->Write(buffer, rc);
		}
	} else {
		WriteBody(NULL, 0);
		m_Stream->Write("\r\n", 2);
	}

	if (m_Request.ProtocolVersion == HttpVersion10 || m_Request.Headers->Get("connection") == "close")
		m_Stream->Shutdown();
}
