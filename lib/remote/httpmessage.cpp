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
 * Adapter class for Boost Beast HTTP messages body to be used with the @c JsonEncoder.
 *
 * This class implements the @c nlohmann::detail::output_adapter_protocol<> interface and provides
 * a way to write JSON data directly into the body of a Boost Beast HTTP message. The adapter is designed
 * to work with Boost Beast HTTP messages that conform to the Beast HTTP message interface and must provide
 * a body type that has a publicly accessible `reader` type that satisfies the Beast BodyReader [^1] requirements.
 *
 * @ingroup base
 *
 * [^1]: https://www.boost.org/doc/libs/1_85_0/libs/beast/doc/html/beast/concepts/BodyReader.html
 */
class HttpResponseJsonWriter : public AsyncJsonWriter
{
public:
	explicit HttpResponseJsonWriter(HttpResponse& msg) : m_Reader(msg.base(), msg.body()), m_Message{msg}
	{
		boost::system::error_code ec;
		// This never returns an actual error, except when overflowing the max
		// buffer size, which we don't do here.
		m_Reader.init(m_MinPendingBufferSize, ec);
		ASSERT(!ec);
	}

	~HttpResponseJsonWriter() override
	{
		boost::system::error_code ec;
		// Same here as in the constructor, all the standard Beast HTTP message reader implementations
		// never return an error here, it's just there to satisfy the interface requirements.
		m_Reader.finish(ec);
		ASSERT(!ec);
	}

	void write_character(char c) override
	{
		write_characters(&c, 1);
	}

	void write_characters(const char* s, std::size_t length) override
	{
		boost::system::error_code ec;
		boost::asio::const_buffer buf{s, length};
		while (buf.size()) {
			std::size_t w = m_Reader.put(buf, ec);
			ASSERT(!ec);
			buf += w;
		}
		m_PendingBufferSize += length;
	}

	void MayFlush(boost::asio::yield_context& yield) override
	{
		if (m_PendingBufferSize >= m_MinPendingBufferSize) {
			m_Message.Flush(yield);
			m_PendingBufferSize = 0;
		}
	}

private:
	HttpResponse::body_type::reader m_Reader;
	HttpResponse& m_Message;
	// The size of the pending buffer to avoid unnecessary writes
	std::size_t m_PendingBufferSize{0};
	// Minimum size of the pending buffer before we flush the data to the underlying stream
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

void HttpResponse::Flush(boost::asio::yield_context yc)
{
	if (!chunked()) {
		ASSERT(!m_Serializer.is_header_done());
		prepare_payload();
	}

	boost::system::error_code ec;
	boost::beast::http::async_write(*m_Stream, m_Serializer, yc[ec]);
	if (ec && ec != boost::beast::http::error::need_buffer) {
		if (yc.ec_) {
			*yc.ec_ = ec;
			return;
		} else {
			BOOST_THROW_EXCEPTION(boost::system::system_error{ec});
		}
	}
	m_Stream->async_flush(yc);

	ASSERT(chunked() || m_Serializer.is_done());
}

void HttpResponse::StartStreaming()
{
	ASSERT(body().Size() == 0 && !m_Serializer.is_header_done());
	body().Start();
	chunked(true);
}

JsonEncoder HttpResponse::GetJsonEncoder(bool pretty)
{
	auto adapter = std::make_shared<HttpResponseJsonWriter>(*this);
	return JsonEncoder{adapter, pretty};
}
