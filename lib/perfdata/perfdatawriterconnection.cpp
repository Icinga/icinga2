/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "perfdata/perfdatawriterconnection.hpp"

#include "base/io-engine.hpp"
#include "base/tcpsocket.hpp"
#include <boost/asio/read.hpp>
#include <future>
#include <utility>

using namespace icinga;
using HttpResponse = PerfdataWriterConnection::HttpResponse;

PerfdataWriterConnection::PerfdataWriterConnection(
	String host,
	String port,
	Shared<boost::asio::ssl::context>::Ptr sslContext,
	bool verifySecure
)
	: m_VerifySecure(verifySecure), m_SslContext(std::move(sslContext)), m_Host(std::move(host)),
	  m_Port(std::move(port)), m_DisconnectTimer(IoEngine::Get().GetIoContext()),
	  m_ReconnectTimer(IoEngine::Get().GetIoContext()), m_Strand(IoEngine::Get().GetIoContext()),
	  m_Stream(ResetStream())
{
}

void PerfdataWriterConnection::Send(boost::asio::const_buffer data)
{
	std::promise<void> promise;

	IoEngine::SpawnCoroutine(
		m_Strand, [&, keepAlive = PerfdataWriterConnection::Ptr(this)](boost::asio::yield_context yc) {
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
		}
	);

	promise.get_future().get();
}

// TODO: Test Disconnect/Reconnect logic
HttpResponse PerfdataWriterConnection::Send(HttpRequest& request)
{
	std::promise<HttpResponse> promise;

	// TODO: keepAlive may be unnecessary since this will live at least as long as the workqueue
	IoEngine::SpawnCoroutine(
		m_Strand, [&, keepAlive = PerfdataWriterConnection::Ptr(this)](boost::asio::yield_context yc) {
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
				std::visit(
					[&](auto& stream) { boost::beast::http::async_read(*stream, m_Streambuf, parser, yc); }, m_Stream
				);

				if (!parser.get().keep_alive()) {
					Disconnect(yc);
				}

				promise.set_value(parser.release());
			} catch (const std::exception&) {
				promise.set_exception(std::current_exception());
			}
		}
	);

	return promise.get_future().get();
}

bool PerfdataWriterConnection::IsConnected()
{
	return m_State == State::connected;
}

void PerfdataWriterConnection::StartDisconnectTimeout(std::chrono::milliseconds timeout)
{
	IoEngine::SpawnCoroutine(
		m_Strand, [&, timeout, keepAlive = PerfdataWriterConnection::Ptr(this)](boost::asio::yield_context yc) {
			try {
				// Is this even needed?
				if (m_State == State::stopped) {
					return;
				}

				if (m_State == State::initial) {
					m_State = State::stopped;
				}

				m_DisconnectTimer.expires_after(timeout);
				m_DisconnectTimer.async_wait(yc);

				/* There may be ongoing connect or disconnect events, in which case
				 * we will want to wait until those are complete and we're in a defined
				 * state again.
				 * TODO: Is it guaranteed that Connecting either succeeds or fails after a
				 * certain amount of time? What in case of SSL connections, if the TCP
				 * connection idles after SSL handshake has started (or before).
				 */
				while (m_State == State::connecting || m_State == State::disconnecting) {
					std::visit([](auto& stream) { stream->lowest_layer().cancel(); }, m_Stream);
					m_DisconnectTimer.expires_after(10ms);
					m_DisconnectTimer.async_wait(yc);
				}

				m_State = State::stopped;
				std::visit([](auto& stream) { stream->lowest_layer().cancel(); }, m_Stream);

				// Yield to give the other coroutine a chance to throw.
				boost::asio::post(yc);

				if (m_State == State::connected) {
					Disconnect(yc);
				}
			} catch (const std::exception& ex) {
				Log(LogCritical, "PerfdataWriterConnection") << "Ex during disconnect timeout: " << ex.what();
			}
		}
	);
}

PerfdataWriterConnection::Stream PerfdataWriterConnection::ResetStream()
{
	Stream ret;
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
		if (m_State == State::stopped) {
			const boost::system::error_code ec{
				boost::system::errc::operation_canceled, boost::system::system_category()
			};
			BOOST_THROW_EXCEPTION(boost::system::system_error{ec});
		}

		m_State = State::connecting;

		try {
			std::visit(
				[&](auto& stream) {
					::Connect(stream->lowest_layer(), m_Host, m_Port, yc);

					if constexpr (std::is_same_v<std::decay_t<decltype(stream)>, TlsStream>) {
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
		} catch (const std::exception& ex) {
			if (m_State == State::connecting) {
				if (const auto* se = dynamic_cast<const boost::system::system_error*>(&ex);
					se->code() == boost::system::errc::operation_canceled) {

					m_State = State::initial;
					throw;
				}

				m_Stream = ResetStream();

				/* Timeout before making another attempt at connecting.
				 * TODO: Make this configurable somewhere
				 * TODO: Do we want to abort at some point?
				 */
				m_ReconnectTimer.expires_after(1000ms);
			}
		}
	}
}

void PerfdataWriterConnection::Disconnect(boost::asio::yield_context yc)
{
	if (m_State == State::connected) {
		m_State = State::disconnecting;

		if (std::holds_alternative<Shared<AsioTlsStream>::Ptr>(m_Stream)) {
			std::get<Shared<AsioTlsStream>::Ptr>(m_Stream)->GracefulDisconnect(m_Strand, yc);
		} else {
			std::get<Shared<AsioTcpStream>::Ptr>(m_Stream)->next_layer().shutdown(
				boost::asio::socket_base::shutdown_both
			);
			std::get<Shared<AsioTcpStream>::Ptr>(m_Stream)->lowest_layer().close();
		}

		m_Stream = ResetStream();
		m_State = State::initial;
	}
}
