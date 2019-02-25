/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
struct HttpRequest
{
public:
	bool CompleteHeaders;
	bool CompleteHeaderCheck;
	bool CompleteBody;

	String RequestMethod;
	Url::Ptr RequestUrl;
	HttpVersion ProtocolVersion;

	Dictionary::Ptr Headers;

	HttpRequest(Stream::Ptr stream);

	bool ParseHeaders(StreamReadContext& src, bool may_wait);
	bool ParseBody(StreamReadContext& src, bool may_wait);
	size_t ReadBody(char *data, size_t count);

	void AddHeader(const String& key, const String& value);
	void WriteBody(const char *data, size_t count);
	void Finish();

private:
	Stream::Ptr m_Stream;
	std::shared_ptr<ChunkReadContext> m_ChunkContext;
	HttpRequestState m_State;
	FIFO::Ptr m_Body;

	void FinishHeaders();
};

}

#endif /* HTTPREQUEST_H */
