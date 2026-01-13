/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include <utility>

#include "perfdata/perfdatawriterconnection.hpp"
#include "test/perfdata-perfdatatargetfixture.hpp"
#include "test/perfdata-perfdatawriterfixture.hpp"
#include "test/remote-certificate-fixture.hpp"
#include "test/test-ctest.hpp"
#include "test/utils.hpp"

using namespace icinga;

class TlsPerfdataWriterFixture : public CertificateFixture, public PerfdataWriterTargetFixture
{
public:
	// TODO: Improve initialization by constructing the stream fully here.
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

BOOST_AUTO_TEST_CASE(tls_connection)
{
	Logger::SetConsoleLogSeverity(LogDebug);
	Logger::EnableConsoleLog();
	BOOST_REQUIRE(!GetConnection().IsConnected());

	std::thread mockTargetThread{[&]() {
		Accept();
		Handshake();
		auto ret = GetDataUntil('\0');
		BOOST_REQUIRE_EQUAL(ret, "foobar");
	}};

	GetConnection().Send(boost::asio::const_buffer{"foobar", 7});

	mockTargetThread.join();
}

BOOST_AUTO_TEST_CASE(stuck_in_handshake)
{
	BOOST_REQUIRE(!GetConnection().IsConnected());

	std::thread mockTargetThread{[&]() {
		Accept();
		GetConnection().StartDisconnectTimeout(50ms);
	}};

	BOOST_REQUIRE_EXCEPTION(
		GetConnection().Send(boost::asio::const_buffer{"foobar", 7}), boost::system::system_error, [&](const auto& ex) {
			return ex.code() == boost::system::errc::operation_canceled;
		}
	);

	mockTargetThread.join();
}

BOOST_AUTO_TEST_CASE(stuck_sending)
{
	BOOST_REQUIRE(!GetConnection().IsConnected());

	std::thread mockTargetThread{[&]() {
		Accept();
		Handshake();
		auto ret = GetDataUntil("#");
		BOOST_REQUIRE_EQUAL(ret, "foobar");
		GetConnection().StartDisconnectTimeout(50ms);
	}};

	auto randomData = GetRandomString("foobar#", 1024UL * 1024);
	BOOST_REQUIRE_EXCEPTION(
		GetConnection().Send(boost::asio::const_buffer{randomData.data(), randomData.size()}),
		boost::system::system_error,
		[&](const auto& ex) { return ex.code() == boost::system::errc::operation_canceled; }
	);

	mockTargetThread.join();
}

BOOST_AUTO_TEST_SUITE_END()
