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

#include "remote/httprequest.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/convert.hpp"

using namespace icinga;

HttpRequest::HttpRequest(Stream::Ptr stream)
	: CompleteHeaders(false),
	CompleteHeaderCheck(false),
	CompleteBody(false),
	ProtocolVersion(HttpVersion11),
	Headers(new Dictionary()),
	m_Stream(std::move(stream)),
	m_State(HttpRequestStart)
{ }

bool HttpRequest::ParseHeaders(StreamReadContext& src, bool may_wait)
{
	if (!m_Stream)
		return false;

	if (m_State != HttpRequestStart && m_State != HttpRequestHeaders)
		BOOST_THROW_EXCEPTION(std::runtime_error("Invalid HTTP state"));

	String line;
	StreamReadStatus srs = m_Stream->ReadLine(&line, src, may_wait);

	if (srs != StatusNewItem) {
		if (src.Size > 8 * 1024)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Line length for HTTP header exceeded"));

		return false;
	}

	if (line.GetLength() > 8 * 1024) {
#ifdef I2_DEBUG /* I2_DEBUG */
		Log(LogDebug, "HttpRequest")
			<< "Header size: " << line.GetLength() << " content: '" << line << "'.";
#endif /* I2_DEBUG */

		BOOST_THROW_EXCEPTION(std::invalid_argument("Line length for HTTP header exceeded"));
	}

	if (m_State == HttpRequestStart) {
		/* ignore trailing new-lines */
		if (line == "")
			return true;

		std::vector<String> tokens = line.Split(" ");
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
		return true;
	} else { // m_State = HttpRequestHeaders
		if (line == "") {
			m_State = HttpRequestBody;
			CompleteHeaders = true;
			return true;

		} else {
			if (Headers->GetLength() > 128)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Maximum number of HTTP request headers exceeded"));

			String::SizeType pos = line.FindFirstOf(":");
			if (pos == String::NPos)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid HTTP request"));

			String key = line.SubStr(0, pos).ToLower().Trim();
			String value = line.SubStr(pos + 1).Trim();
			Headers->Set(key, value);

			if (key == "x-http-method-override")
				RequestMethod = value;

			return true;
		}
	}
}

bool HttpRequest::ParseBody(StreamReadContext& src, bool may_wait)
{
	if (!m_Stream)
		return false;

	if (m_State != HttpRequestBody)
		BOOST_THROW_EXCEPTION(std::runtime_error("Invalid HTTP state"));

	/* we're done if the request doesn't contain a message body */
	if (!Headers->Contains("content-length") && !Headers->Contains("transfer-encoding")) {
		CompleteBody = true;
		return true;
	} else if (!m_Body)
		m_Body = new FIFO();

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
			CompleteBody = true;
		}

		return true;
	}

	if (src.Eof)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Unexpected EOF in HTTP body"));

	if (src.MustRead) {
		if (!src.FillFromStream(m_Stream, false)) {
			src.Eof = true;
			BOOST_THROW_EXCEPTION(std::invalid_argument("Unexpected EOF in HTTP body"));
		}

		src.MustRead = false;
	}

	long length_indicator_signed = Convert::ToLong(Headers->Get("content-length"));

	if (length_indicator_signed < 0)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Content-Length must not be negative."));

	size_t length_indicator = length_indicator_signed;

	if (src.Size < length_indicator) {
		src.MustRead = true;
		return false;
	}

	m_Body->Write(src.Buffer, length_indicator);
	src.DropData(length_indicator);
	CompleteBody = true;
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

void HttpRequest::FinishHeaders()
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

		/* Ensure that we use two new lines here, https://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html */
		m_Stream->Write("\r\n\r\n", 1);

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

void HttpRequest::Finish()
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

		WriteBody(nullptr, 0);
		m_Stream->Write("\r\n", 2);
	}

	m_State = HttpRequestEnd;
}

