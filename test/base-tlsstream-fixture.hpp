/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#ifndef TLSSTREAM_FIXTURE_H
#define TLSSTREAM_FIXTURE_H

#include "remote-sslcert-fixture.hpp"
#include "base/tlsstream.hpp"
#include "base/io-engine.hpp"
#include <future>
#include <BoostTestTargetConfig.h>

namespace icinga {

/**
 * Creates a pair of TLS Streams on a random unused port.
 */
struct TlsStreamFixture: CertificateFixture
{
	TlsStreamFixture()
	{
		using namespace boost::asio::ip;
		using handshake_type = boost::asio::ssl::stream_base::handshake_type;

		auto serverCert = EnsureCertFor("server");
		auto clientCert = EnsureCertFor("client");

		auto& serverIoContext = IoEngine::Get().GetIoContext();

		clientSslContext = SetupSslContext(clientCert.crtFile, clientCert.keyFile,
			m_CaCrtFile.string(), "", DEFAULT_TLS_CIPHERS, DEFAULT_TLS_PROTOCOLMIN, DebugInfo());
		client = Shared<AsioTlsStream>::Make(IoEngine::Get().GetIoContext(), *clientSslContext);

		serverSslContext = SetupSslContext(serverCert.crtFile, serverCert.keyFile,
			m_CaCrtFile.string(), "", DEFAULT_TLS_CIPHERS, DEFAULT_TLS_PROTOCOLMIN, DebugInfo());
		server = Shared<AsioTlsStream>::Make(serverIoContext, *serverSslContext);

		std::promise<void> p;

		tcp::acceptor acceptor{serverIoContext, tcp::endpoint{address_v4::loopback(), 0}};
		acceptor.listen();
		acceptor.async_accept(server->lowest_layer(), [&, this](const boost::system::error_code& ec) {
			if (ec) {
				BOOST_TEST_MESSAGE("Server Accept Error: " + ec.message());
				return;
			}
			server->next_layer().async_handshake(handshake_type::server, [&, this](const boost::system::error_code& ec) {
				if (ec) {
					BOOST_TEST_MESSAGE("Server Handshake Error: " + ec.message());
					return;
				}

				p.set_value();
			});
		});

		boost::system::error_code ec;
		if (client->lowest_layer().connect(acceptor.local_endpoint(), ec)) {
			BOOST_TEST_MESSAGE("Client Connect error: " + ec.message());
		}

		if (client->next_layer().handshake(handshake_type::client, ec)) {
			BOOST_TEST_MESSAGE("Client Handshake error: " + ec.message());
		}
		if (!client->next_layer().IsVerifyOK()) {
			BOOST_TEST_MESSAGE("Verify failed for connection");
			throw;
		}

		p.get_future().wait();
	}

	auto Shutdown(const Shared<AsioTlsStream>::Ptr& stream, std::optional<boost::asio::yield_context> yc = {})
	{
		boost::system::error_code ec;
		if (yc) {
			stream->next_layer().async_shutdown((*yc)[ec]);
		}
		else{
			stream->next_layer().shutdown(ec);
		}
#if BOOST_VERSION < 107000
		/* On boost versions < 1.70, the end-of-file condition was propagated as an error,
		 * even in case of a successful shutdown. This is information can be found in the
		 * changelog for the boost Asio 1.14.0 / Boost 1.70 release.
		 */
		if (ec == boost::asio::error::eof) {
			BOOST_TEST_MESSAGE("Shutdown completed successfully with 'boost::asio::error::eof'.");
			return boost::test_tools::assertion_result{true};
		}
#endif
		boost::test_tools::assertion_result ret{!ec};
		ret.message() << "Error: " << ec.message();
		return ret;
	}

	Shared<boost::asio::ssl::context>::Ptr clientSslContext;
	Shared<AsioTlsStream>::Ptr client;

	Shared<boost::asio::ssl::context>::Ptr serverSslContext;
	Shared<AsioTlsStream>::Ptr server;
};

} // namespace icinga

#endif // TLSSTREAM_FIXTURE_H
