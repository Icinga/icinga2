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
	const ConfigObject::Ptr& parent,
	String host,
	String port,
	Shared<boost::asio::ssl::context>::Ptr sslContext,
	bool verifyPeerCertificate
)
	: PerfdataWriterConnection(
		  parent->GetReflectionType()->GetName(),
		  parent->GetName(),
		  std::move(host),
		  std::move(port),
		  std::move(sslContext),
		  verifyPeerCertificate
	  ) {};

PerfdataWriterConnection::PerfdataWriterConnection(
	String logFacility,
	String parentName,
	String host,
	String port,
	Shared<boost::asio::ssl::context>::Ptr sslContext,
	bool verifyPeerCertificate
)
	: m_VerifyPeerCertificate(verifyPeerCertificate),
	  m_SslContext(std::move(sslContext)),
	  m_LogFacility(std::move(logFacility)),
	  m_ParentName(std::move(parentName)),
	  m_Host(std::move(host)),
	  m_Port(std::move(port)),
	  m_ReconnectTimer(IoEngine::Get().GetIoContext()),
	  m_Strand(IoEngine::Get().GetIoContext()),
	  m_Stream(MakeStream())
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

AsioTlsOrTcpStream PerfdataWriterConnection::MakeStream() const
{
	AsioTlsOrTcpStream ret;
	if (m_SslContext) {
		ret = Shared<AsioTlsStream>::Make(IoEngine::Get().GetIoContext(), *m_SslContext);
	} else {
		ret = Shared<AsioTcpStream>::Make(IoEngine::Get().GetIoContext());
	}

	return ret;
}

/**
 * Wait for the next attempt after an error, using a backoff algorithm.
 *
 * The waits between retries are doubled for each failure, up to a maximum of 32s, until it is
 * reset by a successful attempt.
 */
void PerfdataWriterConnection::BackoffWait(const boost::asio::yield_context& yc)
{
	m_ReconnectTimer.expires_after(m_RetryTimeout);
	if (m_RetryTimeout <= FinalRetryWait / 2) {
		m_RetryTimeout *= 2;
	}
	m_ReconnectTimer.async_wait(yc);
}

void PerfdataWriterConnection::EnsureConnected(const boost::asio::yield_context& yc)
{
	if (m_Connected) {
		return;
	}

	std::visit(
		[&](auto& stream) {
			::Connect(stream->lowest_layer(), m_Host, m_Port, yc);

			if constexpr (std::is_same_v<std::decay_t<decltype(stream)>, Shared<AsioTlsStream>::Ptr>) {
				using type = boost::asio::ssl::stream_base::handshake_type;

				stream->next_layer().async_handshake(type::client, yc);

				if (m_VerifyPeerCertificate) {
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

	m_Stream = MakeStream();
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

HttpResponse PerfdataWriterConnection::WriteMessage(const HttpRequest& request, const boost::asio::yield_context& yc)
{
	boost::beast::http::response<boost::beast::http::string_body> response;
	std::visit(
		[&](auto& stream) {
			boost::beast::http::request_serializer<boost::beast::http::string_body> sr{request};
			boost::beast::http::async_write(*stream, sr, yc);
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
