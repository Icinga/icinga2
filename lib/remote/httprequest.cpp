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

#include "remote/httprequest.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/convert.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace icinga;

HttpRequest::HttpRequest(const Stream::Ptr& stream)
    : Complete(false),
      ProtocolVersion(HttpVersion11),
      Headers(new Dictionary()),
      m_Stream(stream),
      m_State(HttpRequestStart)
{ }

bool HttpRequest::Parse(StreamReadContext& src, bool may_wait)
{
	if (!m_Stream)
		return false;

	if (m_State != HttpRequestBody) {
		String line;
		StreamReadStatus srs = m_Stream->ReadLine(&line, src, may_wait);

		if (srs != StatusNewItem)
			return false;

		if (m_State == HttpRequestStart) {
			/* ignore trailing new-lines */
			if (line == "")
				return true;

			std::vector<String> tokens;
			boost::algorithm::split(tokens, line, boost::is_any_of(" "));
			Log(LogDebug, "HttpRequest")
			    << "line: " << line << ", tokens: " << tokens.size();
			if (tokens.size() != 3)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid HTTP request"));

			RequestMethod = tokens[0];
			RequestUrl = new class Url(tokens[1]);

			if (tokens[2] == "HTTP/1.0")
				ProtocolVersion = HttpVersion10;
			else if (tokens[2] == "HTTP/1.1") {
				ProtocolVersion = HttpVersion11;
			} else
				BOOST_THROW_EXCEPTION(std::invalid_argument("Unsupported HTTP version"));

			m_State = HttpRequestHeaders;
		} else if (m_State == HttpRequestHeaders) {
			if (line == "") {
				m_State = HttpRequestBody;

				/* we're done if the request doesn't contain a message body */
				if (!Headers->Contains("content-length") && !Headers->Contains("transfer-encoding"))
					Complete = true;
				else
					m_Body = new FIFO();

				return true;

			} else {
				String::SizeType pos = line.FindFirstOf(":");
				if (pos == String::NPos)
					BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid HTTP request"));

				String key = line.SubStr(0, pos).ToLower().Trim();
				String value = line.SubStr(pos + 1).Trim();
				Headers->Set(key, value);

				if (key == "x-http-method-override")
					RequestMethod = value;
			}
		} else {
			VERIFY(!"Invalid HTTP request state.");
		}
	} else if (m_State == HttpRequestBody) {
		if (Headers->Get("transfer-encoding") == "chunked") {
			if (!m_ChunkContext)
				m_ChunkContext = std::make_shared<ChunkReadContext>(std::ref(src));

			char *data;
			size_t size;
			StreamReadStatus srs = HttpChunkedEncoding::ReadChunkFromStream(m_Stream, &data, &size, *m_ChunkContext.get(), may_wait);

			if (srs != StatusNewItem)
				return false;

			m_Body->Write(data, size);

			delete [] data;

			if (size == 0) {
				Complete = true;
				return true;
			}
		} else {
			if (src.Eof)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Unexpected EOF in HTTP body"));

			if (src.MustRead) {
				if (!src.FillFromStream(m_Stream, false)) {
					src.Eof = true;
					BOOST_THROW_EXCEPTION(std::invalid_argument("Unexpected EOF in HTTP body"));
				}

				src.MustRead = false;
			}

			size_t length_indicator = Convert::ToLong(Headers->Get("content-length"));

			if (src.Size < length_indicator) {
				src.MustRead = true;
				return false;
			}

			m_Body->Write(src.Buffer, length_indicator);
			src.DropData(length_indicator);
			Complete = true;
			return true;
		}
	}

	return true;
}

size_t HttpRequest::ReadBody(char *data, size_t count)
{
	if (!m_Body)
		return 0;
	else
		return m_Body->Read(data, count, true);
}

void HttpRequest::AddHeader(const String& key, const String& value)
{
	ASSERT(m_State == HttpRequestStart || m_State == HttpRequestHeaders);
	Headers->Set(key.ToLower(), value);
}

void HttpRequest::FinishHeaders(void)
{
	if (m_State == HttpRequestStart) {
		String rqline = RequestMethod + " " + RequestUrl->Format(true) + " HTTP/1." + (ProtocolVersion == HttpVersion10 ? "0" : "1") + "\n";
		m_Stream->Write(rqline.CStr(), rqline.GetLength());
		m_State = HttpRequestHeaders;
	}

	if (m_State == HttpRequestHeaders) {
		AddHeader("User-Agent", "Icinga/" + Application::GetAppVersion());

		if (ProtocolVersion == HttpVersion11) {
			AddHeader("Transfer-Encoding", "chunked");
			if (!Headers->Contains("Host"))
				AddHeader("Host", RequestUrl->GetHost() + ":" + RequestUrl->GetPort());
		}

		ObjectLock olock(Headers);
		for (const Dictionary::Pair& kv : Headers)
		{
			String header = kv.first + ": " + kv.second + "\n";
			m_Stream->Write(header.CStr(), header.GetLength());
		}

		m_Stream->Write("\n", 1);

		m_State = HttpRequestBody;
	}
}

void HttpRequest::WriteBody(const char *data, size_t count)
{
	ASSERT(m_State == HttpRequestStart || m_State == HttpRequestHeaders || m_State == HttpRequestBody);

	if (ProtocolVersion == HttpVersion10) {
		if (!m_Body)
			m_Body = new FIFO();

		m_Body->Write(data, count);
	} else {
		FinishHeaders();

		HttpChunkedEncoding::WriteChunkToStream(m_Stream, data, count);
	}
}

void HttpRequest::Finish(void)
{
	ASSERT(m_State != HttpRequestEnd);

	if (ProtocolVersion == HttpVersion10) {
		if (m_Body)
			AddHeader("Content-Length", Convert::ToString(m_Body->GetAvailableBytes()));

		FinishHeaders();

		while (m_Body && m_Body->IsDataAvailable()) {
			char buffer[1024];
			size_t rc = m_Body->Read(buffer, sizeof(buffer), true);
			m_Stream->Write(buffer, rc);
		}
	} else {
		if (m_State == HttpRequestStart || m_State == HttpRequestHeaders)
			FinishHeaders();

		WriteBody(NULL, 0);
		m_Stream->Write("\r\n", 2);
	}

	m_State = HttpRequestEnd;
}

