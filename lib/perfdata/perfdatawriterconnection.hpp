/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/tlsstream.hpp"
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/string_body.hpp>

namespace icinga {

/**
 * Class handling the connection to the various Perfdata backends.
 */
class PerfdataWriterConnection final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(PerfdataWriterConnection);

	using HttpRequest = boost::beast::http::request<boost::beast::http::string_body>;
	using HttpResponse = boost::beast::http::response<boost::beast::http::string_body>;

	explicit PerfdataWriterConnection(
		String host,
		String port,
		Shared<boost::asio::ssl::context>::Ptr sslContext,
		bool verifySecure = true
	);

	void Send(boost::asio::const_buffer data);
	HttpResponse Send(HttpRequest& request);

	void Disconnect();
	void StartDisconnectTimeout(std::chrono::milliseconds timeout);

	bool IsConnected();

private:
	AsioTlsOrTcpStream ResetStream();
	void EnsureConnected(boost::asio::yield_context yc);
	void Disconnect(boost::asio::yield_context yc);

	enum class State : std::uint8_t
	{
		initial,
		connecting,
		connected,
		disconnecting,
		disconnected,
		failed
	};
	State m_State{State::initial};
	bool m_Stopped{};

	bool m_VerifySecure;
	Shared<boost::asio::ssl::context>::Ptr m_SslContext;

	String m_Host;
	String m_Port;

	std::chrono::milliseconds m_RetryTimeout{1000ms};
	boost::asio::steady_timer m_DisconnectTimer;
	boost::asio::steady_timer m_ReconnectTimer;
	boost::asio::io_context::strand m_Strand;
	boost::asio::streambuf m_Streambuf;
	AsioTlsOrTcpStream m_Stream;
};

} // namespace icinga
