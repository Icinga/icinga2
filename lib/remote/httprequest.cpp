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

#include "remote/httprequest.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace icinga;

HttpRequest::HttpRequest(StreamReadContext& src)
    : Complete(false),
      ProtocolVersion(HttpVersion10),
      Headers(new Dictionary()),
      m_Context(src),
      m_ChunkContext(m_Context),
      m_State(HttpRequestStart)
{ }

bool HttpRequest::Parse(const Stream::Ptr& stream, StreamReadContext& src, bool may_wait)
{
	if (m_State != HttpRequestBody) {
		String line;
		StreamReadStatus srs = stream->ReadLine(&line, src, may_wait);

		if (srs != StatusNewItem)
			return false;

		if (m_State == HttpRequestStart) {
			/* ignore trailing new-lines */
			if (line == "")
				return true;

			std::vector<String> tokens;
			boost::algorithm::split(tokens, line, boost::is_any_of(" "));
			Log(LogWarning, "HttpRequest")
			    << "line: " << line << ", tokens: " << tokens.size();
			if (tokens.size() != 3)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid HTTP request"));
			RequestMethod = tokens[0];
			Url = new class Url(tokens[1]);

			if (tokens[2] == "HTTP/1.0")
				ProtocolVersion = HttpVersion10;
			else if (tokens[2] == "HTTP/1.1") {
				ProtocolVersion = HttpVersion11;
			} else
				BOOST_THROW_EXCEPTION(std::invalid_argument("Unsupported HTTP version"));

			m_State = HttpRequestHeaders;
			Log(LogWarning, "HttpRequest")
			    << "Method: " << RequestMethod << ", Url: " << Url;
		} else if (m_State == HttpRequestHeaders) {
			if (line == "") {
				m_State = HttpRequestBody;

				/* we're done if the request doesn't contain a message body */
				if (!Headers->Contains("content-length") && !Headers->Contains("transfer-encoding"))
					Complete = true;
				else
					m_Body = new FIFO();

				Log(LogWarning, "HttpRequest", "Waiting for message body");
				return true;

			} else {
				String::SizeType pos = line.FindFirstOf(":");
				if (pos == String::NPos)
					BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid HTTP request"));
				String key = line.SubStr(0, pos);
				boost::algorithm::to_lower(key);
				key.Trim();
				String value = line.SubStr(pos + 1);
				value.Trim();
				Headers->Set(key, value);
			}
		} else {
			VERIFY(!"Invalid HTTP request state.");
		}
	} else if (m_State == HttpRequestBody) {
		if (Headers->Get("transfer-encoding") == "chunked") {
			char *data;
			size_t size;
			StreamReadStatus srs = HttpChunkedEncoding::ReadChunkFromStream(stream, &data, &size, m_ChunkContext, false);

			if (srs != StatusNewItem)
				return false;

			Log(LogInformation, "HttpRequest")
			    << "Read " << size << " bytes";

			m_Body->Write(data, size);

			delete [] data;

			if (size == 0) {
				Complete = true;
				return true;
			}
		} else {
			if (m_Context.Eof)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Unexpected EOF in HTTP body"));

			if (m_Context.MustRead) {
				if (!m_Context.FillFromStream(stream, false)) {
					m_Context.Eof = true;
					BOOST_THROW_EXCEPTION(std::invalid_argument("Unexpected EOF in HTTP body"));
				}

				m_Context.MustRead = false;
			}

			size_t length_indicator = Convert::ToLong(Headers->Get("content-length"));

			if (m_Context.Size < length_indicator) {
				m_Context.MustRead = true;
				return false;
			}

			m_Body->Write(m_Context.Buffer, length_indicator);
			m_Context.DropData(length_indicator);
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

