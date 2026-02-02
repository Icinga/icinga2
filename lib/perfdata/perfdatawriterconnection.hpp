// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "base/io-engine.hpp"
#include "base/tlsstream.hpp"
#include <boost/asio/buffer.hpp>
#include <boost/asio/steady_timer.hpp>
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
		String type,
		String name,
		String host,
		String port,
		Shared<boost::asio::ssl::context>::Ptr sslContext,
		bool verifySecure = true
	);

	/**
	 * Send the given data buffer to the server.
	 *
	 * To support each Buffer type this function needs an overload of the WriteMessage method.
	 * If the selected WriteMessage functions returns something, Send() will return that result.
	 *
	 * @param buf The buffer to send
	 * @return the return value returned by the WriteMessage overload for Buffer, otherwise void
	 */
	template<typename Buffer>
	auto Send(Buffer&& buf)
	{
		if (m_Stopped) {
			BOOST_THROW_EXCEPTION(Stopped{});
		}

		using RetType = decltype(WriteMessage(std::declval<Buffer>(), std::declval<boost::asio::yield_context>()));
		std::promise<RetType> promise;

		IoEngine::SpawnCoroutine(m_Strand, [&](boost::asio::yield_context yc) {
			while (true) {
				try {
					EnsureConnected(yc);

					if constexpr (std::is_void_v<RetType>) {
						WriteMessage(std::forward<Buffer>(buf), yc);
						promise.set_value();
					} else {
						promise.set_value(WriteMessage(std::forward<Buffer>(buf), yc));
					}
					return;
				} catch (const std::exception& ex) {
					if (m_Stopped) {
						promise.set_exception(std::make_exception_ptr(Stopped{}));
						return;
					}

					Log(LogCritical, m_Type)
						<< "Error while sending to '" << m_Host << ":" << m_Port << "' for '" << m_Name
						<< "': " << ex.what();

					m_Stream = ResetStream();
					m_Connected = false;
				}
			}
		});

		return promise.get_future().get();
	}

	void Disconnect();

	/**
	 * Cancels ongoing operations either after a timeout or a future became ready.
	 *
	 * This will disconnect and set a flag so that no further Send() requests are accepted.
	 *
	 * @param future The future to wait for
	 * @param timeout The timeout after which ongoing operations are canceled
	 * @return true if the timeout ran out before the future was ready.
	 */
	template<class Rep, class Period>
	bool CancelAfterTimeout(const std::future<void>& future, const std::chrono::duration<Rep, Period>& timeout)
	{
		auto status = future.wait_for(timeout);
		Disconnect();
		return status == std::future_status::timeout;
	}

	bool IsConnected() const;
	bool IsStopped() const;

private:
	AsioTlsOrTcpStream ResetStream();
	void EnsureConnected(const boost::asio::yield_context& yc);
	void Disconnect(boost::asio::yield_context yc);

	void WriteMessage(boost::asio::const_buffer, const boost::asio::yield_context& yc);
	HttpResponse WriteMessage(HttpRequest& request, const boost::asio::yield_context& yc);

	std::atomic_bool m_Stopped{};
	std::atomic_bool m_Connected{};

	bool m_VerifySecure;
	Shared<boost::asio::ssl::context>::Ptr m_SslContext;

	String m_Type;
	String m_Name;
	String m_Host;
	String m_Port;

	std::chrono::milliseconds m_RetryTimeout{1000ms};
	boost::asio::steady_timer m_ReconnectTimer;
	boost::asio::io_context::strand m_Strand;
	AsioTlsOrTcpStream m_Stream;
};

} // namespace icinga
