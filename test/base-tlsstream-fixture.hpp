/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/io-engine.hpp"
#include "base/tlsstream.hpp"
#include "test/remote-certificate-fixture.hpp"
#include <BoostTestTargetConfig.h>
#include <future>

namespace icinga {

/**
 * Creates a pair of TLS Streams on a random unused port.
 */
struct TlsStreamFixture : CertificateFixture
{
	TlsStreamFixture()
	{
		using namespace boost::asio::ip;
		using handshake_type = boost::asio::ssl::stream_base::handshake_type;

		auto serverCert = EnsureCertFor("server");
		auto clientCert = EnsureCertFor("client");

		auto& io = IoEngine::Get().GetIoContext();

		m_ClientSslContext = SetupSslContext(clientCert.crtFile, clientCert.keyFile, m_CaCrtFile.string(), "",
			DEFAULT_TLS_CIPHERS, DEFAULT_TLS_PROTOCOLMIN, DebugInfo());
		client = Shared<AsioTlsStream>::Make(io, *m_ClientSslContext);

		m_ServerSslContext = SetupSslContext(serverCert.crtFile, serverCert.keyFile, m_CaCrtFile.string(), "",
			DEFAULT_TLS_CIPHERS, DEFAULT_TLS_PROTOCOLMIN, DebugInfo());
		server = Shared<AsioTlsStream>::Make(io, *m_ServerSslContext);

		std::promise<void> p;

		tcp::acceptor acceptor{io, tcp::endpoint{address_v4::loopback(), 0}};
		acceptor.listen();
		acceptor.async_accept(server->lowest_layer(), [&](const boost::system::error_code& ec) {
			if (ec) {
				BOOST_TEST_MESSAGE("Server Accept Error: " + ec.message());
				p.set_exception(std::make_exception_ptr(boost::system::system_error{ec}));
				return;
			}
			server->next_layer().async_handshake(handshake_type::server, [&](const boost::system::error_code& ec) {
				if (ec) {
					BOOST_TEST_MESSAGE("Server Handshake Error: " + ec.message());
					p.set_exception(std::make_exception_ptr(boost::system::system_error{ec}));
					return;
				}

				if (!server->next_layer().IsVerifyOK()) {
					p.set_exception(std::make_exception_ptr(std::runtime_error{"Verify failed on server-side."}));
				}

				p.set_value();
			});
		});

		auto f = p.get_future();
		boost::system::error_code ec;
		if (client->lowest_layer().connect(acceptor.local_endpoint(), ec)) {
			BOOST_TEST_MESSAGE("Client Connect error: " + ec.message());
			f.get();
			BOOST_THROW_EXCEPTION(boost::system::system_error{ec});
		}

		if (client->next_layer().handshake(handshake_type::client, ec)) {
			BOOST_TEST_MESSAGE("Client Handshake error: " + ec.message());
			f.get();
			BOOST_THROW_EXCEPTION(boost::system::system_error{ec});
		}

		if (!client->next_layer().IsVerifyOK()) {
			f.get();
			BOOST_THROW_EXCEPTION(std::runtime_error{"Verify failed on client-side."});
		}

		f.get();
	}

	auto Shutdown(const Shared<AsioTlsStream>::Ptr& stream, std::optional<boost::asio::yield_context> yc = {})
	{
		boost::system::error_code ec;
		if (yc) {
			stream->next_layer().async_shutdown((*yc)[ec]);
		} else {
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

	Shared<AsioTlsStream>::Ptr client;
	Shared<AsioTlsStream>::Ptr server;

private:
	Shared<boost::asio::ssl::context>::Ptr m_ClientSslContext;
	Shared<boost::asio::ssl::context>::Ptr m_ServerSslContext;
};

} // namespace icinga
