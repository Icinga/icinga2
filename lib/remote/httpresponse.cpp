/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "base/application.hpp"
#include "base/convert.hpp"

using namespace icinga;

HttpResponse::HttpResponse(Stream::Ptr stream, const HttpRequest& request)
	: Complete(false), m_State(HttpResponseStart), m_Request(request), m_Stream(std::move(stream))
{ }

void HttpResponse::SetStatus(int code, const String& message)
{
	ASSERT(code >= 100 && code <= 599);
	ASSERT(!message.IsEmpty());

	if (m_State != HttpResponseStart) {
		Log(LogWarning, "HttpResponse", "Tried to set Http response status after headers had already been sent.");
		return;
	}

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
	m_Headers.emplace_back(key + ": " + value + "\r\n");
}

void HttpResponse::FinishHeaders()
{
	if (m_State == HttpResponseHeaders) {
		if (m_Request.ProtocolVersion == HttpVersion11)
			AddHeader("Transfer-Encoding", "chunked");

		AddHeader("Server", "Icinga/" + Application::GetAppVersion());

		for (const String& header : m_Headers)
			m_Stream->Write(header.CStr(), header.GetLength());

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

void HttpResponse::Finish()
{
	ASSERT(m_State != HttpResponseEnd);

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
		WriteBody(nullptr, 0);
		m_Stream->Write("\r\n", 2);
	}

	m_State = HttpResponseEnd;

	if (m_Request.ProtocolVersion == HttpVersion10 || m_Request.Headers->Get("connection") == "close")
		m_Stream->Shutdown();
}

bool HttpResponse::Parse(StreamReadContext& src, bool may_wait)
{
	if (m_State != HttpResponseBody) {
		String line;
		StreamReadStatus srs = m_Stream->ReadLine(&line, src, may_wait);

		if (srs != StatusNewItem)
			return false;

		if (m_State == HttpResponseStart) {
			/* ignore trailing new-lines */
			if (line == "")
				return true;

			std::vector<String> tokens;
			boost::algorithm::split(tokens, line, boost::is_any_of(" "));
			Log(LogDebug, "HttpRequest")
				<< "line: " << line << ", tokens: " << tokens.size();
			if (tokens.size() < 2)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid HTTP response (Status line)"));

			if (tokens[0] == "HTTP/1.0")
				ProtocolVersion = HttpVersion10;
			else if (tokens[0] == "HTTP/1.1") {
				ProtocolVersion = HttpVersion11;
			} else
				BOOST_THROW_EXCEPTION(std::invalid_argument("Unsupported HTTP version"));

			StatusCode = Convert::ToLong(tokens[1]);

			if (tokens.size() >= 3)
				StatusMessage = tokens[2]; // TODO: Join tokens[2..end]

			m_State = HttpResponseHeaders;
		} else if (m_State == HttpResponseHeaders) {
			if (!Headers)
				Headers = new Dictionary();

			if (line == "") {
				m_State = HttpResponseBody;
				m_Body = new FIFO();

				return true;

			} else {
				String::SizeType pos = line.FindFirstOf(":");
				if (pos == String::NPos)
					BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid HTTP request"));
				String key = line.SubStr(0, pos).ToLower().Trim();

				String value = line.SubStr(pos + 1).Trim();
				Headers->Set(key, value);
			}
		} else {
			VERIFY(!"Invalid HTTP request state.");
		}
	} else if (m_State == HttpResponseBody) {
		if (Headers->Get("transfer-encoding") == "chunked") {
			if (!m_ChunkContext)
				m_ChunkContext = std::make_shared<ChunkReadContext>(std::ref(src));

			char *data;
			size_t size;
			StreamReadStatus srs = HttpChunkedEncoding::ReadChunkFromStream(m_Stream, &data, &size, *m_ChunkContext.get(), may_wait);

			if (srs != StatusNewItem)
				return false;

			Log(LogNotice, "HttpResponse")
				<< "Read " << size << " bytes";

			m_Body->Write(data, size);

			delete[] data;

			if (size == 0) {
				Complete = true;
				return true;
			}
		} else {
			bool hasLengthIndicator = false;
			size_t lengthIndicator = 0;
			Value contentLengthHeader;

			if (Headers->Get("content-length", &contentLengthHeader)) {
				hasLengthIndicator = true;
				lengthIndicator = Convert::ToLong(contentLengthHeader);
			}

			if (hasLengthIndicator && src.Eof)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Unexpected EOF in HTTP body"));

			if (src.MustRead) {
				if (!src.FillFromStream(m_Stream, may_wait))
					src.Eof = true;

				src.MustRead = false;
			}

			if (!hasLengthIndicator)
				lengthIndicator = src.Size;

			if (src.Size < lengthIndicator) {
				src.MustRead = true;
				return may_wait;
			}

			m_Body->Write(src.Buffer, lengthIndicator);
			src.DropData(lengthIndicator);

			if (!hasLengthIndicator && !src.Eof) {
				src.MustRead = true;
				return may_wait;
			}

			Complete = true;
			return true;
		}
	}

	return true;
}

size_t HttpResponse::ReadBody(char *data, size_t count)
{
	if (!m_Body)
		return 0;
	else
		return m_Body->Read(data, count, true);
}

size_t HttpResponse::GetBodySize() const
{
	if (!m_Body)
		return 0;
	else
		return m_Body->GetAvailableBytes();
}

bool HttpResponse::IsPeerConnected() const
{
	return !m_Stream->IsEof();
}
