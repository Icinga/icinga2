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
	static constexpr auto InitialRetryWait = 50ms;
	static constexpr auto FinalRetryWait = 32s;

public:
	DECLARE_PTR_TYPEDEFS(PerfdataWriterConnection);

	struct Stopped : std::exception
	{
		[[nodiscard]] const char* what() const noexcept final { return "Connection stopped."; }
	};

	using HttpRequest = boost::beast::http::request<boost::beast::http::string_body>;
	using HttpResponse = boost::beast::http::response<boost::beast::http::string_body>;

	PerfdataWriterConnection(
		const ConfigObject::Ptr& parent,
		String host,
		String port,
		Shared<boost::asio::ssl::context>::Ptr sslContext = nullptr,
		bool verifyPeerCertificate = true
	);

	PerfdataWriterConnection(
		String logFacility,
		String parentName,
		String host,
		String port,
		Shared<boost::asio::ssl::context>::Ptr sslContext = nullptr,
		bool verifyPeerCertificate = true
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
		Log(LogDebug, m_LogFacility) << "PerfdataWriterConnection::Send::1";
		if (m_Stopped) {
			Log(LogDebug, m_LogFacility) << "PerfdataWriterConnection::Send::1.1";
			BOOST_THROW_EXCEPTION(Stopped{});
		}
		Log(LogDebug, m_LogFacility) << "PerfdataWriterConnection::Send::2";

		using RetType = decltype(WriteMessage(std::declval<Buffer>(), std::declval<boost::asio::yield_context>()));
		std::promise<RetType> promise;

		IoEngine::SpawnCoroutine(m_Strand, [&](boost::asio::yield_context yc) {
			Log(LogDebug, m_LogFacility) << "PerfdataWriterConnection::Send::3";

			while (true) {
				try {
					Log(LogDebug, m_LogFacility) << "PerfdataWriterConnection::Send::5";
					EnsureConnected(yc);
					Log(LogDebug, m_LogFacility) << "PerfdataWriterConnection::Send::6";

					if constexpr (std::is_void_v<RetType>) {
						WriteMessage(std::forward<Buffer>(buf), yc);
						Log(LogDebug, m_LogFacility) << "PerfdataWriterConnection::Send::7";
						promise.set_value();
					} else {
						promise.set_value(WriteMessage(std::forward<Buffer>(buf), yc));
					}

					m_RetryTimeout = InitialRetryWait;
					return;
				} catch (const std::exception& ex) {
					Log(LogDebug, m_LogFacility) << "PerfdataWriterConnection::Send::8";
					if (m_Stopped) {
						Log(LogDebug, m_LogFacility) << "PerfdataWriterConnection::Send::8.1";
						promise.set_exception(std::make_exception_ptr(Stopped{}));
						return;
					}

					Log(LogCritical, m_LogFacility)
						<< "Error while " << (m_Connected ? "sending" : "connecting") << " to '" << m_Host << ":"
						<< m_Port << "' for '" << m_ParentName << "': " << ex.what();

					m_Stream = MakeStream();
					m_Connected = false;

					try {
						Log(LogDebug, m_LogFacility) << "PerfdataWriterConnection::Send::9";
						BackoffWait(yc);
						Log(LogDebug, m_LogFacility) << "PerfdataWriterConnection::Send::10";
					} catch (const std::exception&) {
						Log(LogDebug, m_LogFacility) << "PerfdataWriterConnection::Send::11";
						promise.set_exception(std::make_exception_ptr(Stopped{}));
						return;
					}
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
	 */
	template<class Rep, class Period>
	void CancelAfterTimeout(const std::future<void>& future, const std::chrono::duration<Rep, Period>& timeout)
	{
		future.wait_for(timeout);
		Disconnect();
	}

	bool IsConnected() const;
	bool IsStopped() const;

private:
	AsioTlsOrTcpStream MakeStream() const;
	void BackoffWait(const boost::asio::yield_context& yc);
	void EnsureConnected(const boost::asio::yield_context& yc);
	void Disconnect(boost::asio::yield_context yc);

	void WriteMessage(boost::asio::const_buffer, const boost::asio::yield_context& yc);
	HttpResponse WriteMessage(const HttpRequest& request, const boost::asio::yield_context& yc);

	std::atomic_bool m_Stopped{false};
	std::atomic_bool m_Connected{false};

	bool m_VerifyPeerCertificate;
	Shared<boost::asio::ssl::context>::Ptr m_SslContext;

	String m_LogFacility;
	String m_ParentName;
	String m_Host;
	String m_Port;

	std::chrono::milliseconds m_RetryTimeout{InitialRetryWait};
	boost::asio::steady_timer m_ReconnectTimer;
	boost::asio::io_context::strand m_Strand;
	AsioTlsOrTcpStream m_Stream;
};

} // namespace icinga
