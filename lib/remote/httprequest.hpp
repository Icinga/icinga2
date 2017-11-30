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

#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include "remote/i2-remote.hpp"
#include "remote/httpchunkedencoding.hpp"
#include "remote/url.hpp"
#include "base/stream.hpp"
#include "base/fifo.hpp"
#include "base/dictionary.hpp"

namespace icinga
{

enum HttpVersion
{
	HttpVersion10,
	HttpVersion11
};

enum HttpRequestState
{
	HttpRequestStart,
	HttpRequestHeaders,
	HttpRequestBody,
	HttpRequestEnd
};

/**
 * An HTTP request.
 *
 * @ingroup remote
 */
struct I2_REMOTE_API HttpRequest
{
public:
	bool Complete;

	String RequestMethod;
	Url::Ptr RequestUrl;
	HttpVersion ProtocolVersion;

	Dictionary::Ptr Headers;

	HttpRequest(const Stream::Ptr& stream);

	bool Parse(StreamReadContext& src, bool may_wait);
	size_t ReadBody(char *data, size_t count);

	void AddHeader(const String& key, const String& value);
	void WriteBody(const char *data, size_t count);
	void Finish(void);

private:
	Stream::Ptr m_Stream;
	std::shared_ptr<ChunkReadContext> m_ChunkContext;
	HttpRequestState m_State;
	FIFO::Ptr m_Body;

	void FinishHeaders(void);
};

}

#endif /* HTTPREQUEST_H */
