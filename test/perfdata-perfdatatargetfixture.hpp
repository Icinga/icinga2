/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#pragma once

#include <BoostTestTargetConfig.h>
#include "base/io-engine.hpp"
#include "base/json.hpp"
#include "base/tlsstream.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/string_body.hpp>

namespace icinga {

/**
 * A fixture that provides methods to simulate a perfdata target
 */
class PerfdataWriterTargetFixture
{
public:
	PerfdataWriterTargetFixture()
		: icinga::PerfdataWriterTargetFixture(Shared<AsioTcpStream>::Make(IoEngine::Get().GetIoContext()))
	{
	}

	explicit PerfdataWriterTargetFixture(const Shared<boost::asio::ssl::context>::Ptr& sslCtx)
		: icinga::PerfdataWriterTargetFixture(Shared<AsioTlsStream>::Make(IoEngine::Get().GetIoContext(), *sslCtx))
	{
		m_SslContext = sslCtx;
	}

	explicit PerfdataWriterTargetFixture(AsioTlsOrTcpStream stream)
		: m_Stream(std::move(stream)),
		  m_Acceptor(
			  IoEngine::Get().GetIoContext(),
			  boost::asio::ip::tcp::endpoint{boost::asio::ip::address_v4::loopback(), 0}
		  )
	{
	}

	unsigned short GetPort() { return m_Acceptor.local_endpoint().port(); }

	void Listen()
	{
		boost::asio::socket_base::receive_buffer_size option{512};
		m_Acceptor.set_option(option);
		m_Acceptor.listen();
	}

	void CloseAcceptor() { m_Acceptor.close(); }

	void Accept()
	{
		BOOST_REQUIRE_NO_THROW(
			std::visit([&](auto& stream) { return m_Acceptor.accept(stream->lowest_layer()); }, m_Stream)
		);
	}

	void Handshake()
	{
		BOOST_REQUIRE(std::holds_alternative<Shared<AsioTlsStream>::Ptr>(m_Stream));
		using handshake_type = UnbufferedAsioTlsStream::handshake_type;
		auto& stream = std::get<Shared<AsioTlsStream>::Ptr>(m_Stream);
		BOOST_REQUIRE_NO_THROW(stream->next_layer().handshake(handshake_type::server));
		BOOST_REQUIRE(stream->next_layer().IsVerifyOK());
	}

	void Shutdown()
	{
		BOOST_REQUIRE(std::holds_alternative<Shared<AsioTlsStream>::Ptr>(m_Stream));
		auto& stream = std::get<Shared<AsioTlsStream>::Ptr>(m_Stream);
		try {
			stream->next_layer().shutdown();
		} catch (const std::exception& ex) {
			if (const auto* se = dynamic_cast<const boost::system::system_error*>(&ex);
				!se || se->code() != boost::asio::error::eof) {
				BOOST_FAIL("Exception in shutdown(): " << ex.what());
			}
		}
		m_Stream = Shared<AsioTlsStream>::Make(IoEngine::Get().GetIoContext(), *m_SslContext);
	}

	std::string GetRequestBody()
	{
		using namespace boost::asio::ip;
		using namespace boost::beast;

		boost::system::error_code ec;
		http::request_parser<boost::beast::http::string_body> parser;
		std::visit([&](auto& stream) { http::read(*stream, m_Buffer, parser, ec); }, m_Stream);
		BOOST_REQUIRE(!ec);

		return parser.get().body();
	}

	auto GetSplitRequestBody(char delim)
	{
		auto body = GetRequestBody();
		std::vector<std::string> result{};
		boost::split(result, body, boost::is_any_of(std::string{delim}));
		return result;
	}

	auto GetSplitDecodedRequestBody()
	{
		Array::Ptr result = new Array;
		for (const auto& line : GetSplitRequestBody('\n')) {
			if (!line.empty()) {
				result->Add(JsonDecode(line));
			}
		}
		return result;
	}

	template<typename T>
	std::string GetDataUntil(T&& delim)
	{
		using namespace boost::asio::ip;

		boost::system::error_code ec;
		auto bytesRead = std::visit(
			[&](auto& stream) { return boost::asio::read_until(*stream, m_Buffer, std::forward<T>(delim), ec); },
			m_Stream
		);
		BOOST_REQUIRE_MESSAGE(!ec, ec.message());

		std::string ret{
			boost::asio::buffers_begin(m_Buffer.data()), boost::asio::buffers_begin(m_Buffer.data()) + bytesRead - 1
		};
		m_Buffer.consume(bytesRead);

		return ret;
	}

	std::size_t ReadRemainingData()
	{
		boost::system::error_code ec;
		std::size_t bytesRead{};
		std::visit(
			[&](auto& stream) {
				while (!ec) {
					bytesRead += stream->read_some(m_Buffer.prepare(4096), ec);
					std::cout << "bytesRead: " << bytesRead << "; ec: " << ec.message() << std::endl;
				}
			},
			m_Stream
		);
		// BOOST_REQUIRE_MESSAGE(!ec, ec.message());
		return bytesRead;
	}

	void SendResponse(boost::beast::http::status status = boost::beast::http::status::ok)
	{
		using namespace boost::asio::ip;
		using namespace boost::beast;

		boost::system::error_code ec;
		http::response<boost::beast::http::empty_body> response;
		response.result(status);
		std::visit([&](auto& stream) { http::write(*stream, response, ec); }, m_Stream);
		BOOST_REQUIRE_MESSAGE(!ec, ec.message());
	}

	void CloseConnection()
	{
		std::visit([&](auto& stream) { stream->lowest_layer().close(); }, m_Stream);
	}

private:
	boost::asio::streambuf m_Buffer;
	AsioTlsOrTcpStream m_Stream;
	boost::asio::ip::tcp::acceptor m_Acceptor;
	Shared<boost::asio::ssl::context>::Ptr m_SslContext;
};

} // namespace icinga
