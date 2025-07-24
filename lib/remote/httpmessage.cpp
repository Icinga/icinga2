/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "remote/httpmessage.hpp"
#include "remote/httputility.hpp"
#include "remote/url.hpp"
#include "base/json.hpp"
#include "base/logger.hpp"
#include <map>
#include <string>
#include <boost/beast/http.hpp>

using namespace icinga;

/**
 * Adapter class for writing JSON data to an HTTP response.
 *
 * This class implements the @c AsyncJsonWriter interface and writes JSON data directly to the @c HttpResponse
 * object. It uses the @c HttpResponse's output stream operator to fill the response body with JSON data.
 * It is designed to be used with the @c JsonEncoder class to indirectly fill the response body with JSON
 * data and then forward it to the underlying stream using the @c Flush() method whenever it needs to do so.
 *
 * @ingroup base
 */
class HttpResponseJsonWriter : public AsyncJsonWriter
{
public:
	explicit HttpResponseJsonWriter(HttpResponse& msg) : m_Message{msg}
	{
	}

	~HttpResponseJsonWriter() override
	{
		m_Message.Finish();
	}

	void write_character(char c) override
	{
		write_characters(&c, 1);
	}

	void write_characters(const char* s, std::size_t length) override
	{
		m_Message << std::string_view(s, length);
	}

	void MayFlush(boost::asio::yield_context& yield) override
	{
		if (m_Message.Size() >= m_MinPendingBufferSize) {
			m_Message.Flush(yield);
		}
	}

private:
	HttpResponse& m_Message;
	/**
	 * Minimum size of the pending buffer before we flush the data to the underlying stream.
	 *
	 * This magic number represents 4x the default size of the internal buffer used by @c AsioTlsStream,
	 * which is 1024 bytes. This ensures that we don't flush too often and allows for efficient streaming
	 * of the response body. The value is chosen to balance between performance and memory usage, as flushing
	 * too often can lead to excessive I/O operations and increased latency, while flushing too infrequently
	 * can lead to high memory usage.
	 */
	static constexpr std::size_t m_MinPendingBufferSize = 4096;
};

HttpRequest::HttpRequest(Shared<AsioTlsStream>::Ptr stream)
	: m_Stream(std::forward<decltype(m_Stream)>(stream)){}

void HttpRequest::ParseHeader(boost::beast::flat_buffer & buf, boost::asio::yield_context yc)
{
	boost::beast::http::async_read_header(*m_Stream, buf, m_Parser, yc);
	base() = m_Parser.get().base();
}

void HttpRequest::ParseBody(boost::beast::flat_buffer & buf, boost::asio::yield_context yc)
{
	boost::beast::http::async_read(*m_Stream, buf, m_Parser, yc);
	body() = std::move(m_Parser.release().body());
}

ApiUser::Ptr HttpRequest::User() const
{
	return m_User;
}

void HttpRequest::User(const ApiUser::Ptr& user)
{
	m_User = user;
}

Url::Ptr HttpRequest::Url() const
{
	return m_Url;
}

void HttpRequest::DecodeUrl()
{
	m_Url = new icinga::Url(std::string(target()));
}

Dictionary::Ptr HttpRequest::Params() const
{
	return m_Params;
}

void HttpRequest::DecodeParams()
{
	if (!m_Url) {
		DecodeUrl();
	}
	m_Params = HttpUtility::FetchRequestParameters(m_Url, body());
}

HttpResponse::HttpResponse(Shared<AsioTlsStream>::Ptr stream)
	: m_Stream(std::move(stream))
{
}

/**
 * Get the size of the current response body buffer.
 *
 * @return size of the response buffer
 */
std::size_t HttpResponse::Size() const
{
	return m_Buffer.size();
}

/**
 * Checks whether the HTTP response headers have been written to the stream.
 *
 * @return true if the HTTP response headers have been written to the stream, false otherwise.
 */
bool HttpResponse::IsHeaderDone() const
{
	return m_HeaderDone;
}

/**
 * Flush the response to the underlying stream.
 *
 * This function flushes the response body and headers to the underlying stream in the following two ways:
 * - If the response is chunked, it writes the body in chunks using the @c boost::beast::http::make_chunk() function.
 *   The size of the current @c m_Buffer represents the chunk size with each call to this function. If there is no
 *   data in the buffer, and the response isn't finished yet, it's a no-op and will silently return. Otherwise, if
 *   the response is finished, it writes the last chunk with a size of zero to indicate the end of the response.
 *
 * - If the response is not chunked, it writes the entire buffer at once using the @c boost::asio::async_write()
 *   function. The content length is set to the size of the current @c m_Buffer. Calling this function more than
 *   once will immediately terminate the process in debug builds, as this should never happen and indicates a
 *   programming error.
 *
 * The headers are written in the same way in both cases, using the @c boost::beast::http::response_serializer<> class.
 *
 * @param yc The yield context for asynchronous operations
 */
void HttpResponse::Flush(boost::asio::yield_context yc)
{
	if (!chunked()) {
		ASSERT(!m_HeaderDone);
		content_length(Size());
	}

	if (!m_HeaderDone) {
		m_HeaderDone = true;
		boost::beast::http::response_serializer<body_type> s(*this);
		boost::beast::http::async_write_header(*m_Stream, s, yc);
	}

	if (chunked()) {
		bool flush = false;
		// In case this is our last chunk (i.e., the response is finished), we need to write any
		// remaining data in the buffer before we terminate the response with a zero-sized last chunk.
		if (Size() > 0) {
			boost::system::error_code ec;
			boost::asio::async_write(*m_Stream, boost::beast::http::make_chunk(m_Buffer.data()), yc[ec]);
			if (ec) {
				if (yc.ec_) {
					*yc.ec_ = std::move(ec);
					return;
				}
				BOOST_THROW_EXCEPTION(boost::system::system_error(ec, "Failed to write chunked response"));
			}

			// The above write operation is guaranteed to either write the entire buffer or fail, and if it's the
			// latter, we shouldn't reach this point, so we can safely trim the entire buffer input sequence here.
			m_Buffer.consume(Size());
			flush = true;
		}

		if (m_Finished) {
			// If the response is finished, we need to write the last chunk with a size of zero.
			boost::asio::async_write(*m_Stream, boost::beast::http::make_chunk_last(), yc);
		} else if (!flush) {
			// If we haven't written any data, we don't need to trigger a flush operation as well.
			return;
		}
	} else {
		// Non-chunked response, write the entire buffer at once.
		async_write(*m_Stream, m_Buffer, yc);
		m_Buffer.consume(Size()); // Won't be reused, but we want to free the memory ASAP.
	}
	m_Stream->async_flush(yc);
}

/**
 * Start streaming the response body in chunks.
 *
 * This function sets the response to use chunked encoding and prepares it for streaming.
 * It must be called before any data is written to the response body, otherwise it will trigger an assertion failure.
 *
 * @note This function should only be called if the response body is expected to be streamed in chunks.
 */
void HttpResponse::StartStreaming()
{
	ASSERT(Size() == 0 && !m_HeaderDone);
	chunked(true);
}

JsonEncoder HttpResponse::GetJsonEncoder(bool pretty)
{
	return JsonEncoder{std::make_shared<HttpResponseJsonWriter>(*this), pretty};
}
