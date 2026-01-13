// SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "remote/httpmessage.hpp"
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
template<typename Message>
class HttpResponseJsonWriter : public AsyncJsonWriter
{
public:
	HttpResponseJsonWriter(const HttpResponseJsonWriter&) = delete;
	HttpResponseJsonWriter(HttpResponseJsonWriter&&) = delete;
	HttpResponseJsonWriter& operator=(const HttpResponseJsonWriter&) = delete;
	HttpResponseJsonWriter& operator=(HttpResponseJsonWriter&&) = delete;
	explicit HttpResponseJsonWriter(Message& msg) : m_Message{msg}
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
	Message& m_Message;
};

template<bool isRequest, typename Body, typename StreamVariant>
IncomingHttpMessage<isRequest, Body, StreamVariant>::IncomingHttpMessage(StreamVariant stream)
	: m_Stream(std::move(stream))
{
}

template<bool isRequest, typename Body, typename StreamVariant>
void IncomingHttpMessage<isRequest, Body, StreamVariant>::ParseHeader(
	boost::beast::flat_buffer& buf,
	boost::asio::yield_context yc
)
{
	std::visit([&](auto& stream) { boost::beast::http::async_read_header(*stream, buf, m_Parser, yc); }, m_Stream);
	Base::base() = m_Parser.get().base();
}

template<bool isRequest, typename Body, typename StreamVariant>
void IncomingHttpMessage<isRequest, Body, StreamVariant>::ParseBody(
	boost::beast::flat_buffer& buf,
	boost::asio::yield_context yc
)
{
	std::visit([&](auto& stream) { boost::beast::http::async_read(*stream, buf, m_Parser, yc); }, m_Stream);
	Base::body() = std::move(m_Parser.release().body());
}

template<bool isRequest, typename Body, typename StreamVariant>
void IncomingHttpMessage<isRequest, Body, StreamVariant>::Parse(boost::asio::yield_context& yc)
{
	boost::beast::flat_buffer buf;
	ParseHeader(buf, yc);
	ParseBody(buf, yc);
}

HttpApiRequest::HttpApiRequest(Shared<AsioTlsStream>::Ptr stream) : IncomingHttpMessage(std::move(stream))
{
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

template<bool isRequest, typename Body, typename StreamVariant>
OutgoingHttpMessage<isRequest, Body, StreamVariant>::OutgoingHttpMessage(StreamVariant stream)
	: m_Stream(std::move(stream))
{
}

template<bool isRequest, typename Body, typename StreamVariant>
void OutgoingHttpMessage<isRequest, Body, StreamVariant>::Clear()
{
	ASSERT(!m_SerializationStarted);
	Base::operator=({});
}

template<bool isRequest, typename Body, typename StreamVariant>
void OutgoingHttpMessage<isRequest, Body, StreamVariant>::Flush(boost::asio::yield_context yc, bool finish)
{
	if (!Base::chunked() && !Base::has_content_length()) {
		ASSERT(!m_SerializationStarted);
		Base::prepare_payload();
	}

	std::visit(
		[&](auto& stream) {
			m_SerializationStarted = true;

			if (!m_Serializer.is_header_done()) {
				boost::beast::http::write_header(*stream, m_Serializer);
			}

			if (finish) {
				Base::body().Finish();
			}

			boost::system::error_code ec;
			boost::beast::http::async_write(*stream, m_Serializer, yc[ec]);
			if (ec && ec != boost::beast::http::error::need_buffer) {
				if (yc.ec_) {
					*yc.ec_ = ec;
					return;
				}
				BOOST_THROW_EXCEPTION(boost::system::system_error{ec});
			}
			stream->async_flush(yc);

			ASSERT(m_Serializer.is_done() || !Base::body().Finished());
		},
		m_Stream
	);
}

template<bool isRequest, typename Body, typename StreamVariant>
void OutgoingHttpMessage<isRequest, Body, StreamVariant>::StartStreaming()
{
	ASSERT(Base::body().Size() == 0 && !m_SerializationStarted);
	Base::body().Start();
	Base::chunked(true);
}

HttpApiResponse::HttpApiResponse(Shared<AsioTlsStream>::Ptr stream, HttpServerConnection::Ptr server)
	: OutgoingHttpMessage(std::move(stream)), m_Server(std::move(server))
{
}

void HttpApiResponse::StartStreaming(bool checkForDisconnect)
{
	OutgoingHttpMessage::StartStreaming();

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

template<bool isRequest, typename Body, typename StreamVariant>
void OutgoingHttpMessage<isRequest, Body, StreamVariant>::SendFile(
	const String& path,
	const boost::asio::yield_context& yc
)
{
	std::ifstream fp(path.CStr(), std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
	fp.exceptions(std::ifstream::badbit | std::ifstream::eofbit);

	std::uint64_t remaining = fp.tellg();
	fp.seekg(0);

	Base::content_length(remaining);
	Base::body().Start();

	while (remaining) {
		auto maxTransfer = std::min(remaining, static_cast<std::uint64_t>(l_FlushThreshold));

		using BodyBuffer = std::decay_t<decltype(std::declval<typename Body::value_type>().Buffer())>;
		using BufferOrSequence = typename BodyBuffer::mutable_buffers_type;
		
		boost::asio::mutable_buffer buf;

		if constexpr (!std::is_same_v<BufferOrSequence, boost::asio::mutable_buffer>) {
			buf = *Base::body().Buffer().prepare(maxTransfer).begin();
		} else {
			buf = Base::body().Buffer().prepare(maxTransfer);
		}
		fp.read(static_cast<char*>(buf.data()), buf.size());
		Base::body().Buffer().commit(buf.size());

		remaining -= buf.size();
		Flush(yc);
	}
}

template<bool isRequest, typename Body, typename StreamVariant>
JsonEncoder OutgoingHttpMessage<isRequest, Body, StreamVariant>::GetJsonEncoder(bool pretty)
{
	return JsonEncoder{
		std::make_shared<HttpResponseJsonWriter<OutgoingHttpMessage<isRequest, Body, StreamVariant>>>(*this), pretty
	};
}

// More general instantiations
template class icinga::OutgoingHttpMessage<true, SerializableFlatBufferBody, AsioTlsOrTcpStream>;
template class icinga::OutgoingHttpMessage<false, SerializableFlatBufferBody, AsioTlsOrTcpStream>;
template class icinga::IncomingHttpMessage<true, boost::beast::http::string_body, AsioTlsOrTcpStream>;
template class icinga::IncomingHttpMessage<false, boost::beast::http::string_body, AsioTlsOrTcpStream>;

// Instantiations specifically for HttpApi(Request|Response)
template class icinga::IncomingHttpMessage<true, boost::beast::http::string_body, std::variant<Shared<AsioTlsStream>::Ptr>>;
template class icinga::OutgoingHttpMessage<false, SerializableMultiBufferBody, std::variant<Shared<AsioTlsStream>::Ptr>>;
