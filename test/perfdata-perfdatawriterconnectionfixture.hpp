/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#pragma once

#include <BoostTestTargetConfig.h>
#include "base/io-engine.hpp"
#include "base/json.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/string_body.hpp>

namespace icinga {

class PerfdataWriterConnectionFixture
{
public:
	PerfdataWriterConnectionFixture()
		: m_Socket(IoEngine::Get().GetIoContext()),
		  m_Acceptor(
			  IoEngine::Get().GetIoContext(),
			  boost::asio::ip::tcp::endpoint{boost::asio::ip::address_v4::loopback(), 0}
		  )
	{
		boost::asio::socket_base::receive_buffer_size option{512};
		m_Acceptor.set_option(option);
		m_Acceptor.listen();
	}

	unsigned int GetPort() { return m_Acceptor.local_endpoint().port(); }

	void Accept()
	{
		boost::system::error_code ec;
		m_Acceptor.accept(m_Socket);
		BOOST_REQUIRE_MESSAGE(!ec, ec.message());
	}

	std::string GetRequestBody()
	{
		using namespace boost::asio::ip;
		using namespace boost::beast;

		boost::system::error_code ec;
		// flat_buffer buf;
		http::request_parser<boost::beast::http::string_body> parser;
		http::read(m_Socket, m_Buffer, parser, ec);
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
		auto bytesRead = boost::asio::read_until(m_Socket, m_Buffer, std::forward<T>(delim), ec);
		BOOST_REQUIRE_MESSAGE(!ec, ec.message());

		std::string ret{
			boost::asio::buffers_begin(m_Buffer.data()), boost::asio::buffers_begin(m_Buffer.data()) + bytesRead - 1
		};
		m_Buffer.consume(bytesRead);

		return ret;
	}

	void SendResponse(boost::beast::http::status status = boost::beast::http::status::ok)
	{
		using namespace boost::asio::ip;
		using namespace boost::beast;

		boost::system::error_code ec;
		http::response<boost::beast::http::empty_body> response;
		response.result(status);
		http::write(m_Socket, response, ec);
		BOOST_REQUIRE_MESSAGE(!ec, ec.message());
	}

	void CloseConnection() { m_Socket.lowest_layer().close(); }

private:
	boost::asio::streambuf m_Buffer;
	boost::asio::ip::tcp::socket m_Socket;
	boost::asio::ip::tcp::acceptor m_Acceptor;
};

} // namespace icinga
