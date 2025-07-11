/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#ifndef TLSSTREAM_FIXTURE_H
#define TLSSTREAM_FIXTURE_H

#include "test/base-sslcert-fixture.hpp"
#include "base/tlsstream.hpp"
#include "base/io-engine.hpp"
#include <BoostTestTargetConfig.h>

namespace icinga {

/**
 * Creates a pair of TLS Streams on a random unused port.
 */
struct TlsStreamFixture: SslCertificateFixture
{
	TlsStreamFixture()
	{
		using namespace boost::asio::ip;

		EnsureCertFor("remote");
		EnsureCertFor("local");

		auto& localContext = IoEngine::Get().GetIoContext();

		remoteIoThread = std::thread{[this]() {
			while(!shutdown){
				remoteContext.run();
			}
		}};

		remoteSslCtx = SetupSslContext(certsDir + "remote.crt", certsDir + "remote.key", caDir+"ca.crt", "", DEFAULT_TLS_CIPHERS,
			DEFAULT_TLS_PROTOCOLMIN, DebugInfo());
		remote = Shared<AsioTlsStream>::Make(remoteContext, *remoteSslCtx);

		localSslCtx = SetupSslContext(certsDir + "local.crt", certsDir + "local.key", caDir+"ca.crt",
			"", DEFAULT_TLS_CIPHERS, DEFAULT_TLS_PROTOCOLMIN, DebugInfo());
		local = Shared<AsioTlsStream>::Make(localContext, *localSslCtx);

		tcp::acceptor acceptor{localContext, tcp::endpoint{make_address_v4("127.0.0.1"), 0}};
		acceptor.listen();
		acceptor.async_accept(local->lowest_layer(), [](const boost::system::error_code& ec) {
			if (ec) {
				std::cout << "Error accepting connection: "	<< ec.message() << std::endl;
			}
		});

		remote->lowest_layer().connect(acceptor.local_endpoint());
		Utility::Sleep(1);

		using handshake_type = boost::asio::ssl::stream_base::handshake_type;
		local->next_layer().async_handshake(handshake_type::server, [](const boost::system::error_code& ec) {
			if (ec) {
				std::cout << "Handshake error: "	<< ec.message() << std::endl;
			}
		});

		remote->next_layer().handshake(handshake_type::client);
		if (!remote->next_layer().IsVerifyOK()) {
			std::cout << "Verify failed for connection" << std::endl;
			throw;
		}
	}

	~TlsStreamFixture()
	{
		shutdown = true;
		remoteIoThread.join();
	}

	std::thread remoteIoThread;
	std::atomic<bool> shutdown = false;

	boost::asio::io_context remoteContext;

	Shared<boost::asio::ssl::context>::Ptr remoteSslCtx;
	Shared<AsioTlsStream>::Ptr remote;

	Shared<boost::asio::ssl::context>::Ptr localSslCtx;
	Shared<AsioTlsStream>::Ptr local;
};

} // namespace icinga

#endif // HTTPSERVERCONNECTION_FIXTURE_H
