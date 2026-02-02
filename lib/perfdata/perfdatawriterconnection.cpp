/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "perfdata/perfdatawriterconnection.hpp"

#include "base/io-engine.hpp"
#include "base/tcpsocket.hpp"
#include <boost/asio/read.hpp>
#include <future>
#include <utility>

using namespace icinga;
using HttpResponse = PerfdataWriterConnection::HttpResponse;

PerfdataWriterConnection::
	PerfdataWriterConnection(String host, String port, Shared<boost::asio::ssl::context>::Ptr sslContext, bool verifySecure)
	: m_VerifySecure(verifySecure), m_SslContext(std::move(sslContext)), m_Host(std::move(host)),
	  m_Port(std::move(port)), m_DisconnectTimer(IoEngine::Get().GetIoContext()),
	  m_ReconnectTimer(IoEngine::Get().GetIoContext()), m_Strand(IoEngine::Get().GetIoContext()), m_Stream(ResetStream())
{
}

void PerfdataWriterConnection::Send(boost::asio::const_buffer data)
{
	std::promise<void> promise;

	IoEngine::SpawnCoroutine(m_Strand, [&, keepAlive = PerfdataWriterConnection::Ptr(this)](boost::asio::yield_context yc) {
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
		} catch (const std::exception&) {
			promise.set_exception(std::current_exception());
		}
	});

	promise.get_future().get();
}

HttpResponse PerfdataWriterConnection::Send(HttpRequest& request)
{
	std::promise<HttpResponse> promise;

	IoEngine::SpawnCoroutine(m_Strand, [&, keepAlive = PerfdataWriterConnection::Ptr(this)](boost::asio::yield_context yc) {
		try {
			EnsureConnected(yc);

			std::visit(
				[&](auto& stream) {
					boost::beast::http::async_write(*stream, request, yc);
					stream->async_flush(yc);
				},
				m_Stream
			);

			boost::beast::http::response_parser<boost::beast::http::string_body> parser;
			std::visit([&](auto& stream) { boost::beast::http::async_read(*stream, m_Streambuf, parser, yc); }, m_Stream);

			if (!parser.get().keep_alive()) {
				Disconnect(yc);
			}

			promise.set_value(parser.release());
		} catch (const std::exception&) {
			promise.set_exception(std::current_exception());
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
	std::promise<bool> p;
	boost::asio::post(m_Strand, [&]() { p.set_value(m_State == State::connected); });
	return p.get_future().get();
}

void PerfdataWriterConnection::Disconnect()
{
	std::promise<void> promise;

	IoEngine::SpawnCoroutine(m_Strand, [&, keepAlive = PerfdataWriterConnection::Ptr(this)](boost::asio::yield_context yc) {
		try {
			Disconnect(std::move(yc));
			promise.set_value();
		} catch (const std::exception& ex) {
			promise.set_exception(std::current_exception());
		}
	});

	promise.get_future().get();
}

void PerfdataWriterConnection::StartDisconnectTimeout(std::chrono::milliseconds timeout)
{
	constexpr auto cancelStreamIfOpen = [](auto& stream) {
		if (stream->lowest_layer().is_open()) {
			stream->lowest_layer().cancel();
		}
	};

	IoEngine::SpawnCoroutine(
		m_Strand, [&, timeout, keepAlive = PerfdataWriterConnection::Ptr(this)](boost::asio::yield_context yc) {
			try {
				if (m_State != State::initial) {
					m_DisconnectTimer.expires_after(timeout);
					m_DisconnectTimer.async_wait(yc);
				} else {
					m_Stopped = true;
					return;
				}

				m_Stopped = true;

				/* This needs to be done in a loop, because ASIO's cancel isn't guaranteed to
				 * cancel anything. For example, a connect() operation may already be queued for
				 * completion after this coroutine yields, so we need to attempt another
				 * cancellation for a potential handshake.
				 */
				while (m_State == State::connecting) {
					std::visit(cancelStreamIfOpen, m_Stream);
					boost::asio::post(yc);
				}

				/* From here on, the other coroutine is either in failed or connected state.
				 */
				if (m_State == State::failed) {
					m_ReconnectTimer.cancel();
				} else if (m_State == State::connected) {
					std::visit(cancelStreamIfOpen, m_Stream);
				}

				/* We can now be sure that the other coroutine will throw an operation_aborted
				 * error_code. All we need to do is yield to give it a chance to throw it.
				 */
				boost::asio::post(yc);

				/* Disconnect only does anything if the last state was connected.
				 */
				Disconnect(yc);
			} catch (const std::exception& ex) {
				Log(LogCritical, "PerfdataWriterConnection") << "Exception during disconnect timeout: " << ex.what();
			}
		}
	);
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
	while (m_State != State::connected) {
		if (m_Stopped) {
			const boost::system::error_code ec{boost::asio::error::operation_aborted, boost::system::system_category()};
			BOOST_THROW_EXCEPTION(boost::system::system_error{ec});
		}

		m_State = State::connecting;

		try {
			std::visit(
				[&](auto& stream) {
					::Connect(stream->lowest_layer(), m_Host, m_Port, yc);

					if constexpr (std::is_same_v<std::remove_reference_t<decltype(stream)>, Shared<AsioTlsStream>::Ptr>) {
						using type = boost::asio::ssl::stream_base::handshake_type;

						stream->next_layer().async_handshake(type::client, yc);

						if (m_VerifySecure && !stream->next_layer().IsVerifyOK()) {
							BOOST_THROW_EXCEPTION(std::runtime_error{"TLS certificate validation failed"});
						}
					}
				},
				m_Stream
			);

			m_State = State::connected;
			m_RetryTimeout = 1s;
		} catch (const std::exception& ex) {
			if (m_State == State::connecting) {
				m_State = State::failed;
				if (const auto* se = dynamic_cast<const boost::system::system_error*>(&ex);
					se->code() == boost::asio::error::operation_aborted) {
					throw;
				}

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
}

void PerfdataWriterConnection::Disconnect(boost::asio::yield_context yc)
{
	if (m_State != State::connected) {
		return;
	}

	m_State = State::disconnecting;

	std::visit(
		[&](auto& stream) {
			if constexpr (std::is_same_v<std::remove_reference_t<decltype(stream)>, Shared<AsioTlsStream>::Ptr>) {
				stream->GracefulDisconnect(m_Strand, yc);
			} else {
				stream->next_layer().shutdown(boost::asio::socket_base::shutdown_both);
				stream->lowest_layer().close();
			}
		},
		m_Stream
	);

	m_Stream = ResetStream();
	m_State = State::disconnected;
}
