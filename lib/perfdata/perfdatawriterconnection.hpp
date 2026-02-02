// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "base/tlsstream.hpp"
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <future>

namespace icinga {

/**
 * Class handling the connection to the various Perfdata backends.
 */
class PerfdataWriterConnection final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(PerfdataWriterConnection);

	struct Stopped : std::exception
	{
		[[nodiscard]] const char* what() const noexcept final { return "Connection stopped."; }
	};

	using HttpRequest = boost::beast::http::request<boost::beast::http::string_body>;
	using HttpResponse = boost::beast::http::response<boost::beast::http::string_body>;

	explicit PerfdataWriterConnection(
		String name,
		String host,
		String port,
		Shared<boost::asio::ssl::context>::Ptr sslContext,
		bool verifySecure = true
	);

	void Send(boost::asio::const_buffer data);
	HttpResponse Send(HttpRequest& request);

	void Disconnect();

	/**
	 * Cancels ongoing operations if the future does not receive a value within the timeout.
	 *
	 * This will disconnect and set a flag so that no further Send() requests are accepted.
	 *
	 * @param future The future to wait for
	 * @param timeout The timeout after which ongoing operations are canceled
	 */
	template<class Rep, class Period>
	void CancelAfterTimeout(const std::future<void>& future, const std::chrono::duration<Rep, Period>& timeout)
	{
		auto status = future.wait_for(timeout);
		if (status != std::future_status::ready) {
			Cancel();
		}
		Disconnect();
	}

	void Cancel();

	bool IsConnected() const;

private:
	AsioTlsOrTcpStream ResetStream();
	void EnsureConnected(boost::asio::yield_context yc);
	void Disconnect(boost::asio::yield_context yc);

	std::atomic_bool m_Stopped{};
	std::atomic_bool m_Connected{};

	bool m_VerifySecure;
	Shared<boost::asio::ssl::context>::Ptr m_SslContext;

	String m_Name;
	String m_Host;
	String m_Port;

	std::chrono::milliseconds m_RetryTimeout{1000ms};
	boost::asio::steady_timer m_DisconnectTimer;
	boost::asio::steady_timer m_ReconnectTimer;
	boost::asio::io_context::strand m_Strand;
	AsioTlsOrTcpStream m_Stream;
};

} // namespace icinga
