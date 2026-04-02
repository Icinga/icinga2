// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "perfdata/perfdatawriterconnection.hpp"
#include "test/perfdata-perfdatatargetfixture.hpp"
#include "test/remote-certificate-fixture.hpp"
#include "test/test-ctest.hpp"
#include "test/test-thread.hpp"
#include "test/utils.hpp"

using namespace icinga;

class TlsPerfdataWriterFixture : public CertificateFixture, public PerfdataWriterTargetFixture
{
public:
	TlsPerfdataWriterFixture() : PerfdataWriterTargetFixture(MakeContext("server"))
	{
		m_PdwSslContext = MakeContext("client");

		m_Conn = new PerfdataWriterConnection{"Test", "test", "127.0.0.1", std::to_string(GetPort()), m_PdwSslContext};
	}

	auto& GetConnection() { return *m_Conn; }

	static inline const std::vector<String> RequiredCerts{"client", "server"};

private:
	Shared<boost::asio::ssl::context>::Ptr MakeContext(const std::string& name)
	{
		auto testCert = GetCertFor(name);
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

BOOST_FIXTURE_TEST_SUITE(perfdata_connection, TlsPerfdataWriterFixture,
	*RequiresCertificate(TlsPerfdataWriterFixture::RequiredCerts)
	*boost::unit_test::label("perfdata")
	*boost::unit_test::label("network")
)

/* If there is no acceptor listening on the other side, connecting should fail.
 */
BOOST_AUTO_TEST_CASE(connection_refused)
{
	std::promise<void> p;
	TestThread timeoutThread{[&]() {
		auto f = p.get_future();
		GetConnection().CancelAfterTimeout(f, 50ms);
	}};

	BOOST_REQUIRE_THROW(
		GetConnection().Send(boost::asio::const_buffer{"foobar", 7}), PerfdataWriterConnection::Stopped
	);

	REQUIRE_JOINS_WITHIN(timeoutThread, 1s);
}

/* The PerfdataWriterConnection connects automatically when sending the first data.
 * In case of http we also need to support disconnecting and reconnecting.
 */
BOOST_AUTO_TEST_CASE(ensure_connected)
{
	std::promise<void> disconnectedPromise;

	TestThread mockTargetThread{[&]() {
		Accept();
		Handshake();
		auto ret = GetDataUntil('\0');
		Shutdown();
		disconnectedPromise.get_future().get();
		BOOST_REQUIRE_EQUAL(ret, "foobar");
	}};

	BOOST_REQUIRE_NO_THROW(GetConnection().Send(boost::asio::const_buffer{"foobar", 7}));
	BOOST_REQUIRE_NO_THROW(GetConnection().Disconnect());
	disconnectedPromise.set_value();

	REQUIRE_JOINS_WITHIN(mockTargetThread, 1s);
}

/* Verify that data can still be sent while CancelAfterTimeout is waiting and the timeout
 * can be aborted when all data has been sent successfully.
 */
BOOST_AUTO_TEST_CASE(finish_during_timeout)
{
	std::promise<void> p;

	TestThread mockTargetThread{[&]() {
		Accept();
		Handshake();
		auto ret = GetDataUntil('\0');
		BOOST_REQUIRE_EQUAL(ret, "foobar");
		ret = GetDataUntil('\0');
		BOOST_REQUIRE_EQUAL(ret, "foobar");
		// This is done here instead of the main thread after send, because we need to
		// synchronize the asserts done in the timeoutThread after this point.
		p.set_value();
		Shutdown();
	}};

	GetConnection().Send(boost::asio::const_buffer{"foobar", 7});

	TestThread timeoutThread{[&]() {
		auto f = p.get_future();
		GetConnection().CancelAfterTimeout(f, 50ms);
		BOOST_REQUIRE(f.wait_for(0ms) == std::future_status::ready);
		BOOST_REQUIRE(!GetConnection().IsConnected());
	}};

	GetConnection().Send(boost::asio::const_buffer{"foobar", 7});

	REQUIRE_JOINS_WITHIN(timeoutThread, 1s);
	REQUIRE_JOINS_WITHIN(mockTargetThread, 1s);
}

/* For the client, even a hanging server will accept the connection immediately, since it's done
 * in the kernel. But in that case the TLS handshake will be stuck, so we need to verify that a
 * handshake can be interrupted by CancelAfterTimeout().
 */
BOOST_AUTO_TEST_CASE(stuck_in_handshake)
{
	TestThread mockTargetThread{[&]() { Accept(); }};

	std::promise<void> p;
	TestThread timeoutThread{[&]() {
		auto f = p.get_future();
		GetConnection().CancelAfterTimeout(f, 50ms);
		BOOST_REQUIRE(f.wait_for(0ms) == std::future_status::timeout);
	}};

	BOOST_REQUIRE_THROW(
		GetConnection().Send(boost::asio::const_buffer{"foobar", 7}), PerfdataWriterConnection::Stopped
	);

	REQUIRE_JOINS_WITHIN(timeoutThread, 1s);
	REQUIRE_JOINS_WITHIN(mockTargetThread, 1s);
}

/* When the disconnect timeout runs out while sending something to a slow or blocking server, we
 * expect the send to be aborted after a timeout with an 'operation cancelled' exception, in
 * order to not delay the shutdown of a perfdata writer indefinitely.
 * No orderly TLS shutdown can be performed in this case, because the stream has been truncated.
 * The server will need to handle this one on their own.
 */
BOOST_AUTO_TEST_CASE(stuck_sending)
{
	std::promise<void> shutdownPromise;
	std::promise<void> dataReadPromise;

	TestThread mockTargetThread{[&]() {
		Accept();
		Handshake();
		auto ret = GetDataUntil("#");
		BOOST_REQUIRE_EQUAL(ret, "foobar");
		dataReadPromise.set_value();

		// There's still a full buffer waiting to be read, but we're pretending to be dead and
		// close the socket at this point.
		shutdownPromise.get_future().get();
		ResetStream();
	}};

	TestThread timeoutThread{[&]() {
		// Synchronize with when mockTargetThread has read the initial data.
		// This should especially help with timing on slow machines like the ARM GHA runners.
		dataReadPromise.get_future().get();
		BOOST_REQUIRE(GetConnection().IsConnected());
		BOOST_REQUIRE_NO_THROW(GetConnection().Disconnect());
		BOOST_REQUIRE(!GetConnection().IsConnected());
	}};

	// Allocate a large string that will fill the buffers on both sides of the connection, in
	// order to make Send() block.
	auto randomData = GetRandomString("foobar#", 4UL * 1024 * 1024);
	auto buf = boost::asio::const_buffer{randomData.data(), randomData.size()};
	BOOST_REQUIRE_THROW(GetConnection().Send(buf), PerfdataWriterConnection::Stopped);
	shutdownPromise.set_value();

	REQUIRE_JOINS_WITHIN(timeoutThread, 1s);
	REQUIRE_JOINS_WITHIN(mockTargetThread, 1s);
}

/* This simulates a server that is stuck after receiving a HTTP request and before sending their
 * response. Here, the simulated server is polite and still responds to a shutdown request, but
 * in reality a server might not even do that. That case should be handled by our
 * AsioTlsStream::GracefulDisconnect() function with an additional 10s timeout.
 */
BOOST_AUTO_TEST_CASE(stuck_reading_response)
{
	std::promise<void> shutdownPromise;
	std::promise<void> requestReadPromise;

	TestThread mockTargetThread{[&]() {
		Accept();
		Handshake();
		auto ret = GetRequest();
		BOOST_REQUIRE_EQUAL(ret.body(), "bar");
		requestReadPromise.set_value();
		// Do not send a response but react to the shutdown to be polite.
		shutdownPromise.get_future().get();
		Shutdown();
	}};

	TestThread timeoutThread{[&]() {
		// Synchronize with after mockTargetThread has read the request
		requestReadPromise.get_future().get();
		BOOST_REQUIRE(GetConnection().IsConnected());
		BOOST_REQUIRE_NO_THROW(GetConnection().Disconnect());
		BOOST_REQUIRE(!GetConnection().IsConnected());
	}};

	boost::beast::http::request<boost::beast::http::string_body> request;
	request.body() = "bar";
	request.method(boost::beast::http::verb::get);
	request.target("foo");
	request.prepare_payload();
	BOOST_REQUIRE_THROW(GetConnection().Send(request), PerfdataWriterConnection::Stopped);
	shutdownPromise.set_value();

	REQUIRE_JOINS_WITHIN(timeoutThread, 1s);
	REQUIRE_JOINS_WITHIN(mockTargetThread, 1s);
}

/* This test simulates a server that closes the connection and reappears at a later time.
 * PerfdataWriterConnection should detect the disconnect, catch the exception and attempt to
 * reconnect without exiting Send().
 */
BOOST_AUTO_TEST_CASE(reconnect_failed)
{
	TestThread mockTargetThread{[&]() {
		Accept();
		Handshake();
		auto ret = GetDataUntil("#");
		BOOST_REQUIRE_EQUAL(ret, "foobar");

		ResetStream();

		Accept();
		Handshake();

		ret = GetDataUntil("#");
		BOOST_REQUIRE_EQUAL(ret, "foobar");
		ret = GetDataUntil("\n");

		Shutdown();
	}};

	// Allocate a large string that will fill the buffers on both sides of the connection, in
	// order to make Send() block.
	auto randomData = GetRandomString("foobar#", 4UL * 1024 * 1024);
	randomData.push_back('\n');
	BOOST_REQUIRE_NO_THROW(GetConnection().Send(boost::asio::const_buffer{randomData.data(), randomData.size()}));
	BOOST_REQUIRE_NO_THROW(GetConnection().Disconnect());

	REQUIRE_JOINS_WITHIN(mockTargetThread, 1s);
}

/* This tests if retrying an http send will reproducibly lead to the exact same message being
 * received. Normally this us guaranteed by the interface only accepting a const reference, but
 * since on older boost versions the async_write() functions also accept non-const references, it
 * doesn't hurt to ensure this with a test-case.
 */
BOOST_AUTO_TEST_CASE(http_send_retry)
{
	TestThread mockTargetThread{[&] {
		Accept();
		Handshake();

		/* Read only the first 512 bytes of the request body, since we don't want to unblock the client yet.
		 */
		auto request = GetRequest(512);
		BOOST_REQUIRE_MESSAGE(
			request.method() == boost::beast::http::verb::post,
			"Request method is not POST: " << request.method_string()
		);
		BOOST_REQUIRE_MESSAGE(request.target() == "foo", "Request target is not 'foo': " << request.target());
		BOOST_REQUIRE_MESSAGE(
			request.body().compare(0, 7, "foobar#") == 0,
			"Request body does not start with 'foobar#': " << request.body().substr(0, 7)
		);

		ResetStream();
		Accept();
		Handshake();

		/* Read the entire response now and verify that we still get the expected body,
		 * even though the first read was only partial.
		 */
		request = GetRequest();
		BOOST_REQUIRE_MESSAGE(
			request.method() == boost::beast::http::verb::post,
			"Request method is not POST: " << request.method_string()
		);
		BOOST_REQUIRE_MESSAGE(request.target() == "foo", "Request target is not 'foo': " << request.target());
		BOOST_REQUIRE_MESSAGE(
			request.body().compare(0, 7, "foobar#") == 0,
			"Request body does not start with 'foobar#': " << request.body().substr(0, 7)
		);

		/* The body size is 4MB + 7 bytes (7 bytes for the "foobar#" prefix of the generated message)
		 */
		BOOST_REQUIRE_MESSAGE(
			request.body().size() == (4UL * 1024 * 1024) + 7,
			"Request body is not the expected size: " << request.body().size()
		);

		SendResponse();

		Shutdown();
	}};

	boost::beast::http::request<boost::beast::http::string_body> request{boost::beast::http::verb::post, "foo", 10};
	request.set(boost::beast::http::field::host, "localhost:" + std::to_string(GetPort()));

	/* Allocate a large string that will fill the buffers on both sides of the connection, in
	 * order to make Send() block.
	 */
	request.body() = GetRandomString("foobar#", 4UL * 1024 * 1024);
	request.prepare_payload();
	BOOST_REQUIRE_NO_THROW(GetConnection().Send(request));
	BOOST_REQUIRE_NO_THROW(GetConnection().Disconnect());

	REQUIRE_JOINS_WITHIN(mockTargetThread, 1s);
}

BOOST_AUTO_TEST_SUITE_END()
