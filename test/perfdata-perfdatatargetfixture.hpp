// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

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
			  IoEngine::Get().GetIoContext()
		  )
	{
		boost::asio::ip::tcp::endpoint ep{boost::asio::ip::address_v4::loopback(), 0};
		m_Acceptor.open(ep.protocol());
		m_Acceptor.bind(ep);
	}

	unsigned short GetPort() { return m_Acceptor.local_endpoint().port(); }

	void Listen()
	{
		m_Acceptor.listen();
	}

	void Accept()
	{
		Listen();
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

	void Shutdown(bool wait = false)
	{
		BOOST_REQUIRE(std::holds_alternative<Shared<AsioTlsStream>::Ptr>(m_Stream));
		auto& stream = std::get<Shared<AsioTlsStream>::Ptr>(m_Stream);
		try {
			if (wait) {
				std::array<std::byte, 128> buf{};
				boost::asio::mutable_buffer readBuf (buf.data(), buf.size());
				boost::system::error_code ec;

				do {
					stream->read_some(readBuf, ec);
				} while (!ec);
			}
			stream->next_layer().shutdown();
		} catch (const std::exception& ex) {
			if (const auto* se = dynamic_cast<const boost::system::system_error*>(&ex);
				!se || se->code() != boost::asio::error::eof) {
				BOOST_FAIL("Exception in shutdown(): " << ex.what());
			}
		}

		ResetStream();
	}

	void ResetStream()
	{
		if (std::holds_alternative<Shared<AsioTlsStream>::Ptr>(m_Stream)) {
			m_Stream = Shared<AsioTlsStream>::Make(IoEngine::Get().GetIoContext(), *m_SslContext);
		} else {
			m_Stream = Shared<AsioTcpStream>::Make(IoEngine::Get().GetIoContext());
		}
	}

	/**
	 * Reads the HTTP request body from the stream, with an optional limit on the number of bytes to read.
	 *
	 * @param bytes The maximum number of bytes to read from the request body. If 0, there is no limit and the entire body will be read.
	 *
	 * @return The HTTP request read from the stream.
	 */
	boost::beast::http::request<boost::beast::http::string_body> GetRequest(std::size_t bytes = 0)
	{
		using namespace boost::beast;

		boost::beast::flat_buffer buf;
		if (bytes > 0) {
			buf = boost::beast::flat_buffer{bytes};
		}
		boost::system::error_code ec;
		http::request_parser<http::string_body> parser;
		parser.body_limit(-1);
		std::visit(
			[&](auto& stream) {
				http::read(*stream, buf, parser, ec);
			},
			m_Stream
		);
		if (bytes > 0) {
			BOOST_REQUIRE_MESSAGE(
				!ec || ec == http::error::buffer_overflow,
				"Reading request body with a buffer limit of '" << bytes <<
				"' should either succeed or fail with a buffer_overflow error, but got: " << ec.message()
			);
		} else {
			BOOST_REQUIRE_MESSAGE(!ec, "Error while reading request body: " << ec.message());
			BOOST_REQUIRE_MESSAGE(parser.is_done(), "Parser did not finish reading the request, but no error was set.");
		}
		return parser.release();
	}

	auto GetSplitRequestBody(char delim)
	{
		auto request = GetRequest();
		std::vector<std::string> result{};
		boost::split(result, request.body(), boost::is_any_of(std::string{delim}));
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

		std::size_t delimLength{1};
		if constexpr (!std::is_same_v<std::decay_t<T>, char>) {
			delimLength = std::string_view{delim}.size();
		}

		boost::asio::streambuf buf;
		boost::system::error_code ec;
		auto bytesRead = std::visit(
			[&](auto& stream) { return boost::asio::read_until(*stream, buf, std::forward<T>(delim), ec); }, m_Stream
		);
		BOOST_REQUIRE_MESSAGE(!ec, ec.message());

		std::string ret{
			boost::asio::buffers_begin(buf.data()), boost::asio::buffers_begin(buf.data()) + bytesRead - delimLength
		};
		buf.consume(bytesRead);

		return ret;
	}

	void SendResponse(boost::beast::http::status status = boost::beast::http::status::ok)
	{
		using namespace boost::asio::ip;
		using namespace boost::beast;

		boost::system::error_code ec;
		http::response<boost::beast::http::empty_body> response;
		response.result(status);
		response.prepare_payload();
		std::visit(
			[&](auto& stream) {
				http::write(*stream, response, ec);
				BOOST_REQUIRE_MESSAGE(!ec, ec.message());
				stream->flush(ec);
				BOOST_REQUIRE_MESSAGE(!ec, ec.message());
			},
			m_Stream
		);
	}

private:
	AsioTlsOrTcpStream m_Stream;
	boost::asio::ip::tcp::acceptor m_Acceptor;
	Shared<boost::asio::ssl::context>::Ptr m_SslContext;
};

} // namespace icinga
