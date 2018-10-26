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

#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include "remote/httprequest.hpp"
#include "base/stream.hpp"
#include "base/fifo.hpp"
#include <vector>

namespace icinga
{

enum HttpResponseState
{
	HttpResponseStart,
	HttpResponseHeaders,
	HttpResponseBody,
	HttpResponseEnd
};

/**
 * An HTTP response.
 *
 * @ingroup remote
 */
struct I2_REMOTE_API HttpResponse
{
public:
	bool Complete;

	HttpVersion ProtocolVersion;
	int StatusCode;
	String StatusMessage;

	Dictionary::Ptr Headers;

	HttpResponse(const Stream::Ptr& stream, const HttpRequest& request);

	bool Parse(StreamReadContext& src, bool may_wait);
	size_t ReadBody(char *data, size_t count);
	size_t GetBodySize(void) const;

	void SetStatus(int code, const String& message);
	void AddHeader(const String& key, const String& value);
	void WriteBody(const char *data, size_t count);
	void Finish(void);

	bool IsPeerConnected(void) const;

private:
	HttpResponseState m_State;
	std::shared_ptr<ChunkReadContext> m_ChunkContext;
	const HttpRequest& m_Request;
	Stream::Ptr m_Stream;
	FIFO::Ptr m_Body;
	std::vector<String> m_Headers;

	void FinishHeaders(void);
};

}

#endif /* HTTPRESPONSE_H */
