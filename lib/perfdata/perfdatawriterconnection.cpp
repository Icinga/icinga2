// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "perfdata/perfdatawriterconnection.hpp"
#include "base/tcpsocket.hpp"
#include <boost/asio/use_future.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <utility>

using namespace icinga;
using HttpResponse = PerfdataWriterConnection::HttpResponse;

PerfdataWriterConnection::PerfdataWriterConnection(
	String type,
	String name,
	String host,
	String port,
	Shared<boost::asio::ssl::context>::Ptr sslContext,
	bool verifySecure
)
	: m_VerifySecure(verifySecure),
	  m_SslContext(std::move(sslContext)),
	  m_Type(std::move(type)),
	  m_Name(std::move(name)),
	  m_Host(std::move(host)),
	  m_Port(std::move(port)),
	  m_ReconnectTimer(IoEngine::Get().GetIoContext()),
	  m_Strand(IoEngine::Get().GetIoContext()),
	  m_Stream(ResetStream())
{
}

/**
 * Get the current state of the connection.
 */
bool PerfdataWriterConnection::IsConnected() const
{
	return m_Connected;
}

bool PerfdataWriterConnection::IsStopped() const
{
	return m_Stopped;
}

void PerfdataWriterConnection::Disconnect()
{
	if (m_Stopped.exchange(true, std::memory_order_relaxed)) {
		return;
	}

	std::promise<void> promise;

	IoEngine::SpawnCoroutine(m_Strand, [&](boost::asio::yield_context yc) {
		try {
			/* Cancel any outstanding operations of the other coroutine.
			 * Since we're on the same strand we're hopefully guaranteed that all cancellations
			 * result in exceptions thrown by the yield_context, even if its already queued for
			 * completion.
			 */
			std::visit(
				[](const auto& stream) {
					if (stream->lowest_layer().is_open()) {
						stream->lowest_layer().cancel();
					}
				},
				m_Stream
			);
			m_ReconnectTimer.cancel();

			Disconnect(std::move(yc));
			promise.set_value();
		} catch (const std::exception& ex) {
			promise.set_exception(std::current_exception());
		}
	});

	promise.get_future().get();
}

AsioTlsOrTcpStream PerfdataWriterConnection::ResetStream()
{
	AsioTlsOrTcpStream ret;
	if (m_SslContext) {
		ret = Shared<AsioTlsStream>::Make(IoEngine::Get().GetIoContext(), *m_SslContext);
	} else {
		ret = Shared<AsioTcpStream>::Make(IoEngine::Get().GetIoContext());
	}

	return ret;
}

void PerfdataWriterConnection::EnsureConnected(const boost::asio::yield_context& yc)
{
	while (!m_Connected) {
		if (m_Stopped) {
			BOOST_THROW_EXCEPTION(Stopped{});
		}

		try {
			std::visit(
				[&](auto& stream) {
					::Connect(stream->lowest_layer(), m_Host, m_Port, yc);

					if constexpr (std::is_same_v<std::decay_t<decltype(stream)>, Shared<AsioTlsStream>::Ptr>) {
						using type = boost::asio::ssl::stream_base::handshake_type;

						stream->next_layer().async_handshake(type::client, yc);

						if (m_VerifySecure) {
							if (!stream->next_layer().IsVerifyOK()) {
								BOOST_THROW_EXCEPTION(
									std::runtime_error{
										"TLS certificate validation failed: " + stream->next_layer().GetVerifyError()
									}
								);
							}
						}
					}
				},
				m_Stream
			);

			m_Connected = true;
			m_RetryTimeout = 1s;
		} catch (const std::exception& ex) {
			if (m_Stopped) {
				throw;
			}

			Log(LogWarning, m_Type)
				<< "Failed to connect to host '" << m_Host << "' port '" << m_Port << "' for '" << m_Name
				<< "': " << ex.what();

			m_Stream = ResetStream();

			/* Timeout before making another attempt at connecting.
			 */
			m_ReconnectTimer.expires_after(m_RetryTimeout);
			if (m_RetryTimeout < 30s) {
				m_RetryTimeout *= 2;
			}
			m_ReconnectTimer.async_wait(yc);
		}
	}
}

void PerfdataWriterConnection::Disconnect(boost::asio::yield_context yc)
{
	if (!m_Connected.exchange(false, std::memory_order_relaxed)) {
		return;
	}

	std::visit(
		[&](auto& stream) {
			if constexpr (std::is_same_v<std::decay_t<decltype(stream)>, Shared<AsioTlsStream>::Ptr>) {
				stream->GracefulDisconnect(m_Strand, yc);
			} else {
				stream->lowest_layer().shutdown(boost::asio::socket_base::shutdown_both);
				stream->lowest_layer().close();
			}
		},
		m_Stream
	);

	m_Stream = ResetStream();
}

void PerfdataWriterConnection::WriteMessage(boost::asio::const_buffer buf, const boost::asio::yield_context& yc)
{
	std::visit(
		[&](auto& stream) {
			boost::asio::async_write(*stream, buf, yc);
			stream->async_flush(yc);
		},
		m_Stream
	);
}

HttpResponse PerfdataWriterConnection::WriteMessage(HttpRequest& request, const boost::asio::yield_context& yc)
{
	boost::beast::http::response<boost::beast::http::string_body> response;
	std::visit(
		[&](auto& stream) {
			boost::beast::http::async_write(*stream, request, yc);
			stream->async_flush(yc);

			boost::beast::flat_buffer buf;
			boost::beast::http::async_read(*stream, buf, response, yc);
		},
		m_Stream
	);

	if (!response.keep_alive()) {
		Disconnect(yc);
	}

	return response;
}
