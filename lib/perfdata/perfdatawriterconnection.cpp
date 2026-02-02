// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "perfdata/perfdatawriterconnection.hpp"
#include "base/io-engine.hpp"
#include "base/tcpsocket.hpp"
#include <boost/asio/read.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <future>
#include <utility>

using namespace icinga;
using HttpResponse = PerfdataWriterConnection::HttpResponse;

PerfdataWriterConnection::PerfdataWriterConnection(
	String name,
	String host,
	String port,
	Shared<boost::asio::ssl::context>::Ptr sslContext,
	bool verifySecure
)
	: m_VerifySecure(verifySecure),
	  m_SslContext(std::move(sslContext)),
	  m_Name(std::move(name)),
	  m_Host(std::move(host)),
	  m_Port(std::move(port)),
	  m_DisconnectTimer(IoEngine::Get().GetIoContext()),
	  m_ReconnectTimer(IoEngine::Get().GetIoContext()),
	  m_Strand(IoEngine::Get().GetIoContext()),
	  m_Stream(ResetStream())
{
}

void PerfdataWriterConnection::Send(boost::asio::const_buffer data)
{
	std::promise<void> promise;

	IoEngine::SpawnCoroutine(m_Strand, [&, _ = Ptr(this)](boost::asio::yield_context yc) {
		while (true) {
			try {
				EnsureConnected(yc);

				std::visit(
					[&](auto& stream) {
						boost::asio::async_write(*stream, data, yc);
						stream->async_flush(yc);
					},
					m_Stream
				);

				promise.set_value();
				return;
			} catch (const std::exception& ex) {
				if (const auto* se = dynamic_cast<const boost::system::system_error*>(&ex);
					se && se->code() == boost::asio::error::operation_aborted) {
					promise.set_exception(std::current_exception());
					return;
				}

				Log(LogCritical, "PerfdataWriterConnection")
					<< "Error while sending to '" << m_Host << ":" << m_Port << "' for '" << m_Name
					<< "': " << ex.what();

				m_Stream = ResetStream();
				m_Connected = false;
				continue;
			}
		}
	});

	promise.get_future().get();
}

HttpResponse PerfdataWriterConnection::Send(HttpRequest& request)
{
	std::promise<HttpResponse> promise;

	IoEngine::SpawnCoroutine(m_Strand, [&, _ = Ptr(this)](boost::asio::yield_context yc) {
		while (true) {
			try {
				EnsureConnected(yc);

				boost::beast::http::response_parser<boost::beast::http::string_body> parser;
				std::visit(
					[&](auto& stream) {
						boost::beast::http::async_write(*stream, request, yc);
						stream->async_flush(yc);

						boost::asio::streambuf buf;
						boost::beast::http::async_read(*stream, buf, parser, yc);
					},
					m_Stream
				);

				if (!parser.get().keep_alive()) {
					Disconnect(yc);
				}

				promise.set_value(parser.release());
				return;
			} catch (const std::exception& ex) {
				if (const auto* se = dynamic_cast<const boost::system::system_error*>(&ex);
					se && se->code() == boost::asio::error::operation_aborted) {
					promise.set_exception(std::current_exception());
					return;
				}

				Log(LogCritical, "PerfdataWriterConnection")
					<< "Error while sending to '" << m_Host << ":" << m_Port << "' for '" << m_Name
					<< "': " << ex.what();

				m_Stream = ResetStream();
				m_Connected = false;
				continue;
			}
		}
	});

	return promise.get_future().get();
}

/**
 * Get the current state of the connection.
 *
 * This wraps retrieving the state in boost::asio::post() on the strand instead of making it
 * atomic, because the only defined states are the suspension points where the coroutine yields.
 */
bool PerfdataWriterConnection::IsConnected()
{
	auto isConnected = [&]() { return m_Connected; };
	return boost::asio::post(m_Strand, boost::asio::use_future(isConnected)).get();
}

void PerfdataWriterConnection::Disconnect()
{
	std::promise<void> promise;

	IoEngine::SpawnCoroutine(m_Strand, [&, _ = Ptr(this)](boost::asio::yield_context yc) {
		try {
			Disconnect(std::move(yc));
			promise.set_value();
		} catch (const std::exception& ex) {
			promise.set_exception(std::current_exception());
		}
	});

	promise.get_future().get();
}

/**
 * Cancel all outstanding operations and disconnect after a timeout.
 *
 * @param timeout The time after which to start initiating the disconnect.
 */
void PerfdataWriterConnection::StartDisconnectTimeout(std::chrono::milliseconds timeout)
{
	IoEngine::SpawnCoroutine(m_Strand, [&, timeout, _ = Ptr(this)](boost::asio::yield_context yc) {
		m_DisconnectTimer.expires_after(timeout);
		m_DisconnectTimer.async_wait(yc);

		m_Stopped = true;

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
		boost::asio::post(yc);

		try {
			/* Disconnect only does anything if the last state was connected.
			 */
			Disconnect(yc);
		} catch (const std::exception& ex) {
			Log(LogCritical, "PerfdataWriterConnection") << "Exception during disconnect timeout: " << ex.what();
		}
	});
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

void PerfdataWriterConnection::EnsureConnected(boost::asio::yield_context yc)
{
	while (!m_Connected) {
		if (m_Stopped) {
			const boost::system::error_code ec{boost::asio::error::operation_aborted, boost::system::system_category()};
			BOOST_THROW_EXCEPTION(boost::system::system_error{ec});
		}

		try {
			std::visit(
				[&](auto& stream) {
					::Connect(stream->lowest_layer(), m_Host, m_Port, yc);

					if constexpr (std::is_same_v<std::decay_t<decltype(stream)>, Shared<AsioTlsStream>::Ptr>) {
						using type = boost::asio::ssl::stream_base::handshake_type;

						stream->next_layer().async_handshake(type::client, yc);

						if (m_VerifySecure && !stream->next_layer().IsVerifyOK()) {
							BOOST_THROW_EXCEPTION(std::runtime_error{"TLS certificate validation failed"});
						}
					}
				},
				m_Stream
			);

			m_Connected = true;
			m_RetryTimeout = 1s;
		} catch (const std::exception& ex) {
			m_Connected = false;
			if (const auto* se = dynamic_cast<const boost::system::system_error*>(&ex);
				se->code() == boost::asio::error::operation_aborted) {
				throw;
			}

			Log(LogWarning, "PerfdataWriterConnection")
				<< "Failed't connect to host '" << m_Host << "' port '" << m_Port << "' for '" << m_Name
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
	if (!m_Connected) {
		return;
	}
	m_Connected = false;

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
