/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include <utility>

#include "perfdata/perfdatawriterconnection.hpp"
#include "test/perfdata-perfdatatargetfixture.hpp"
#include "test/perfdata-perfdatawriterfixture.hpp"
#include "test/remote-certificate-fixture.hpp"
#include "test/test-ctest.hpp"
#include "test/test-thread.hpp"
#include "test/utils.hpp"

using namespace icinga;

class TlsPerfdataWriterFixture : public CertificateFixture, public PerfdataWriterTargetFixture
{
public:
	TlsPerfdataWriterFixture() : PerfdataWriterTargetFixture(MakeContext())
	{
		auto pdwCert = EnsureCertFor("client");
		m_PdwSslContext = SetupSslContext(
			pdwCert.crtFile,
			pdwCert.keyFile,
			m_CaCrtFile.string(),
			"",
			DEFAULT_TLS_CIPHERS,
			DEFAULT_TLS_PROTOCOLMIN,
			DebugInfo()
		);

		m_Conn = new PerfdataWriterConnection{"127.0.0.1", std::to_string(GetPort()), m_PdwSslContext};
	}

	auto& GetConnection() { return *m_Conn; }

private:
	Shared<boost::asio::ssl::context>::Ptr MakeContext()
	{
		auto testCert = EnsureCertFor("server");
		return SetupSslContext(
			testCert.crtFile,
			testCert.keyFile,
			m_CaCrtFile.string(),
			"",
			DEFAULT_TLS_CIPHERS,
			DEFAULT_TLS_PROTOCOLMIN,
			DebugInfo()
		);
	}

	Shared<boost::asio::ssl::context>::Ptr m_PdwSslContext;
	PerfdataWriterConnection::Ptr m_Conn;
};

BOOST_FIXTURE_TEST_SUITE(perfdatawriterconnection, TlsPerfdataWriterFixture,
	*CTestProperties("FIXTURES_REQUIRED ssl_certs")
	*boost::unit_test::label("perfdata"))

/* If there is no acceptor listening on the other side, connecting should fail.
 */
BOOST_AUTO_TEST_CASE(connection_refused)
{
	CloseAcceptor();

	TestThread mockTargetThread{[&]() { GetConnection().StartDisconnectTimeout(50ms); }};

	BOOST_REQUIRE_EXCEPTION(
		GetConnection().Send(boost::asio::const_buffer{"foobar", 7}),
		boost::system::system_error,
		[](const auto& ex) -> bool { return ex.code() == boost::asio::error::operation_aborted; }
	);

	REQUIRE_JOINS_WITHIN(mockTargetThread, 1s);
}

/* The PerfdataWriterConnection connects automatically when sending the first data.
 * In case of http we also need to support disconnecting and reconnecting.
 */
BOOST_AUTO_TEST_CASE(ensure_connected)
{
	TestThread mockTargetThread{[&]() {
		Accept();
		Handshake();
		auto ret = GetDataUntil('\0');
		BOOST_REQUIRE_EQUAL(ret, "foobar");
		Shutdown();

		/* Test a second cycle to make sure reusing the socket works.
		 */
		Accept();
		Handshake();
		ret = GetDataUntil('\0');
		BOOST_REQUIRE_EQUAL(ret, "foobar");
		Shutdown();
	}};

	GetConnection().Send(boost::asio::const_buffer{"foobar", 7});
	GetConnection().Disconnect();

	GetConnection().Send(boost::asio::const_buffer{"foobar", 7});
	GetConnection().Disconnect();

	REQUIRE_JOINS_WITHIN(mockTargetThread, 1s);
}

/* This tests a "manual" disconnect() while in the diconnection timeout, similar to what a
 * perfdata writer does if the it manages to finish the WorkQueue before the timeout runs out.
 */
BOOST_AUTO_TEST_CASE(disconnect_during_timeout)
{
	TestThread mockTargetThread{[&]() {
		Accept();
		Handshake();
		auto ret = GetDataUntil('\0');
		BOOST_REQUIRE_EQUAL(ret, "foobar");
		Shutdown();
	}};

	BOOST_REQUIRE_NO_THROW(GetConnection().Send(boost::asio::const_buffer{"foobar", 7}));
	GetConnection().StartDisconnectTimeout(50ms);
	BOOST_REQUIRE_NO_THROW(GetConnection().Disconnect());

	REQUIRE_JOINS_WITHIN(mockTargetThread, 1s);
}

/* While the grace-period given through StartDisconnectTimeout is active, data can still be sent,
 * assuming we had already connected to the server.
 */
BOOST_AUTO_TEST_CASE(finish_during_timeout)
{
	TestThread mockTargetThread{[&]() {
		Accept();
		Handshake();
		auto ret = GetDataUntil('\0');
		BOOST_REQUIRE_EQUAL(ret, "foobar");
		ret = GetDataUntil('\0');
		BOOST_REQUIRE_EQUAL(ret, "foobar");
		Shutdown();
	}};

	GetConnection().Send(boost::asio::const_buffer{"foobar", 7});
	GetConnection().StartDisconnectTimeout(50ms);
	GetConnection().Send(boost::asio::const_buffer{"foobar", 7});

	REQUIRE_JOINS_WITHIN(mockTargetThread, 1s);
}

/* Stops the connection before any attempt to connect anywhere has been made. This is needed to
 * speed up quick successive reloads of the daemon.
 */
BOOST_AUTO_TEST_CASE(stop_immediately)
{
	auto start = std::chrono::steady_clock::now();

	GetConnection().StartDisconnectTimeout(50ms);
	BOOST_REQUIRE_EXCEPTION(
		GetConnection().Send(boost::asio::const_buffer{"foobar", 7}),
		boost::system::system_error,
		[](const auto& ex) -> bool { return ex.code() == boost::asio::error::operation_aborted; }
	);

	// Ensure that the Send() has actually returned before the timeout has elapsed.
	BOOST_REQUIRE(std::chrono::steady_clock::now() - start < 40ms);
}

/* For the client, even a hanging server will accept the connection immediately, since it's done
 * in the kernel. But in that case the TLS handshake will be stuck, so we need to verify that a
 * handshake can be interrupted by StartDisconnectTimeout().
 */
BOOST_AUTO_TEST_CASE(stuck_in_handshake)
{
	TestThread mockTargetThread{[&]() {
		Accept();
		GetConnection().StartDisconnectTimeout(50ms);
	}};

	BOOST_REQUIRE_EXCEPTION(
		GetConnection().Send(boost::asio::const_buffer{"foobar", 7}),
		boost::system::system_error,
		[&](const auto& ex) { return ex.code() == boost::asio::error::operation_aborted; }
	);

	REQUIRE_JOINS_WITHIN(mockTargetThread, 1s);
}

/* When the disconnect timeout runs out while sending something to a slow or blocking server, we
 * expect the send to be aborted immediately with an 'operation cancelled' exception, in order to
 * not prolong the shutdown of a perfdata writer.
 * No regular shutdown can be performed in this case, because the stream has been truncated. The
 * server will need to handle this one on their own.
 */
BOOST_AUTO_TEST_CASE(stuck_sending)
{
	TestThread mockTargetThread{[&]() {
		Accept();
		Handshake();
		auto ret = GetDataUntil("#");
		BOOST_REQUIRE_EQUAL(ret, "foobar");

		/* This is necessary so ultry-slow machines (like the Github-workflow building the
		 * container image) don't interrupt the connection process with the successive 50ms
		 * disconnect timeout.
		 */
		auto start = std::chrono::steady_clock::now();
		while (!GetConnection().IsConnected() && std::chrono::steady_clock::now() - start < 1s) {
			std::this_thread::sleep_for(10ms);
		}

		GetConnection().StartDisconnectTimeout(50ms);
	}};

	// Allocate a large string that will fill the buffers on both sides of the connection, in
	// order to make Send() block.
	auto randomData = GetRandomString("foobar#", 4UL * 1024 * 1024);
	BOOST_REQUIRE_EXCEPTION(
		GetConnection().Send(boost::asio::const_buffer{randomData.data(), randomData.size()}),
		boost::system::system_error,
		[&](const auto& ex) {
			BOOST_TEST_INFO("Exception: " << ex.what());
			return ex.code() == boost::asio::error::operation_aborted;
		}
	);

	REQUIRE_JOINS_WITHIN(mockTargetThread, 1s);
}

BOOST_AUTO_TEST_SUITE_END()
