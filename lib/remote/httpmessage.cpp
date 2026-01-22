/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "remote/httpmessage.hpp"
#include "base/io-engine.hpp"
#include "base/json.hpp"
#include "remote/httputility.hpp"
#include "remote/url.hpp"
#include <boost/beast/http.hpp>
#include <fstream>
#include <string>

using namespace icinga;

/**
 * This is the buffer size threshold above which to flush to the connection.
 *
 * This value was determined with a series of measurements in
 * [PR #10516](https://github.com/Icinga/icinga2/pull/10516#issuecomment-3232642284).
 */
constexpr std::size_t l_FlushThreshold = 128UL * 1024UL;

/**
 * Adapter class for Boost Beast HTTP messages body to be used with the @c JsonEncoder.
 *
 * This class implements the @c nlohmann::detail::output_adapter_protocol<> interface and provides
 * a way to write JSON data directly into the body of a @c HttpResponse.
 *
 * @ingroup base
 */
class HttpResponseJsonWriter : public AsyncJsonWriter
{
public:
	explicit HttpResponseJsonWriter(HttpApiResponse& msg) : m_Message{msg}
	{
		m_Message.body().Start();
#if BOOST_VERSION >= 107000
		// We pre-allocate more than the threshold because we always go above the threshold
		// at least once.
		m_Message.body().Buffer().reserve(l_FlushThreshold + (l_FlushThreshold / 4));
#endif /* BOOST_VERSION */
	}

	~HttpResponseJsonWriter() override { m_Message.body().Finish(); }

	void write_character(char c) override { write_characters(&c, 1); }

	void write_characters(const char* s, std::size_t length) override
	{
		auto buf = m_Message.body().Buffer().prepare(length);
		boost::asio::buffer_copy(buf, boost::asio::const_buffer{s, length});
		m_Message.body().Buffer().commit(length);
	}

	void MayFlush(boost::asio::yield_context& yield) override
	{
		if (m_Message.body().Size() >= l_FlushThreshold) {
			m_Message.Flush(yield);
		}
	}

private:
	HttpApiResponse& m_Message;
};

HttpApiRequest::HttpApiRequest(Shared<AsioTlsStream>::Ptr stream) : m_Stream(std::move(stream))
{
}

void HttpApiRequest::ParseHeader(boost::beast::flat_buffer& buf, boost::asio::yield_context yc)
{
	boost::beast::http::async_read_header(*m_Stream, buf, m_Parser, yc);
	base() = m_Parser.get().base();
}

void HttpApiRequest::ParseBody(boost::beast::flat_buffer& buf, boost::asio::yield_context yc)
{
	boost::beast::http::async_read(*m_Stream, buf, m_Parser, yc);
	body() = std::move(m_Parser.release().body());
}

ApiUser::Ptr HttpApiRequest::User() const
{
	return m_User;
}

void HttpApiRequest::User(const ApiUser::Ptr& user)
{
	m_User = user;
}

Url::Ptr HttpApiRequest::Url() const
{
	return m_Url;
}

void HttpApiRequest::DecodeUrl()
{
	m_Url = new icinga::Url(std::string(target()));
}

Dictionary::Ptr HttpApiRequest::Params() const
{
	return m_Params;
}

void HttpApiRequest::DecodeParams()
{
	if (!m_Url) {
		DecodeUrl();
	}
	m_Params = HttpUtility::FetchRequestParameters(m_Url, body());
}

HttpApiResponse::HttpApiResponse(Shared<AsioTlsStream>::Ptr stream, HttpServerConnection::Ptr server)
	: m_Server(std::move(server)), m_Stream(std::move(stream))
{
}

void HttpApiResponse::Clear()
{
	ASSERT(!m_SerializationStarted);
	boost::beast::http::response<body_type>::operator=({});
}

void HttpApiResponse::Flush(boost::asio::yield_context yc)
{
	if (!chunked() && !has_content_length()) {
		ASSERT(!m_SerializationStarted);
		prepare_payload();
	}

	m_SerializationStarted = true;

	if (!m_Serializer.is_header_done()) {
		boost::beast::http::write_header(*m_Stream, m_Serializer);
	}

	boost::system::error_code ec;
	boost::beast::http::async_write(*m_Stream, m_Serializer, yc[ec]);
	if (ec && ec != boost::beast::http::error::need_buffer) {
		if (yc.ec_) {
			*yc.ec_ = ec;
			return;
		}
		BOOST_THROW_EXCEPTION(boost::system::system_error{ec});
	}
	m_Stream->async_flush(yc);

	ASSERT(m_Serializer.is_done() || !body().Finished());
}

void HttpApiResponse::StartStreaming(bool checkForDisconnect)
{
	ASSERT(body().Size() == 0 && !m_SerializationStarted);
	body().Start();
	chunked(true);

	if (checkForDisconnect) {
		ASSERT(m_Server);
		m_Server->StartDetectClientSideShutdown();
	}
}

bool HttpApiResponse::IsClientDisconnected() const
{
	ASSERT(m_Server);
	return m_Server->Disconnected();
}

void HttpApiResponse::SendFile(const String& path, const boost::asio::yield_context& yc)
{
	std::ifstream fp(path.CStr(), std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
	fp.exceptions(std::ifstream::badbit | std::ifstream::eofbit);

	std::uint64_t remaining = fp.tellg();
	fp.seekg(0);

	content_length(remaining);
	body().Start();

	while (remaining) {
		auto maxTransfer = std::min(remaining, static_cast<std::uint64_t>(l_FlushThreshold));

		auto buf = *body().Buffer().prepare(maxTransfer).begin();
		fp.read(static_cast<char*>(buf.data()), buf.size());
		body().Buffer().commit(buf.size());

		remaining -= buf.size();
		Flush(yc);
	}
}

JsonEncoder HttpApiResponse::GetJsonEncoder(bool pretty)
{
	return JsonEncoder{std::make_shared<HttpResponseJsonWriter>(*this), pretty};
}
