/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
struct HttpResponse
{
public:
	bool Complete;

	HttpVersion ProtocolVersion;
	int StatusCode;
	String StatusMessage;

	Dictionary::Ptr Headers;

	HttpResponse(Stream::Ptr stream, const HttpRequest& request);

	bool Parse(StreamReadContext& src, bool may_wait);
	size_t ReadBody(char *data, size_t count);
	size_t GetBodySize() const;

	void SetStatus(int code, const String& message);
	void AddHeader(const String& key, const String& value);
	void WriteBody(const char *data, size_t count);
	void Finish();

	bool IsPeerConnected() const;

	void RebindRequest(const HttpRequest& request);

private:
	HttpResponseState m_State;
	std::shared_ptr<ChunkReadContext> m_ChunkContext;
	const HttpRequest *m_Request;
	Stream::Ptr m_Stream;
	FIFO::Ptr m_Body;
	std::vector<String> m_Headers;

	void FinishHeaders();
};

}

#endif /* HTTPRESPONSE_H */
