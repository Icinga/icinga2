/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "test/base-tlsstream-fixture.hpp"
#include "test/icingaapplication-fixture.hpp"
#include "base/base64.hpp"
#include "base/json.hpp"
#include "config/configitem.hpp"
#include "config/configcompiler.hpp"
#include "remote/httphandler.hpp"
#include <boost/beast/http.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/container/flat_set.hpp>
#include <set>
#include <BoostTestTargetConfig.h>

using namespace icinga;
using namespace boost::beast;
using namespace boost::unit_test_framework;

struct HttpServerConnectionFixture :
	TlsStreamFixture, IcingaApplicationFixture, ConfigurationCacheDirCleanupFixture
{
	HttpServerConnectionFixture()
		: wg(new StoppableWaitGroup)
	{
		Logger::SetConsoleLogSeverity(icinga::LogDebug);
		timeout.expires_from_now(boost::posix_time::seconds(2));
		timeout.async_wait([this](const boost::system::error_code& ec) {
			if (!ec) {
				remote->lowest_layer().close();
				local->lowest_layer().close();
			} else {
				BOOST_TEST_MESSAGE(ec.message());
			}
		});
	}

	~HttpServerConnectionFixture() {};

	static void CreateApiListener(){
		ConfigItem::RunWithActivationContext(new Function("CreateTestUser", [](){
		String config = R"CONFIG(
const NodeName = "local"
object Endpoint "local" {}
object Zone "local" {
endpoints = [ "local" ]
}
object ApiListener "api" {
  accept_config = false
  accept_commands = false

  bind_host = "localhost"
  bind_port = 123456
  ticket_salt = "test"
  access_control_allow_origin = ["127.0.0.1"]
}
)CONFIG";
			std::unique_ptr<Expression> expr = ConfigCompiler::CompileText("<test>", config);
			expr->Evaluate(*ScriptFrame::GetCurrentFrame());
		}));
	}

	static void CreateTestUsers()
	{
		ConfigItem::RunWithActivationContext(new Function("CreateTestUser", [](){
			String config = R"CONFIG(
object ApiUser "remote" {
  client_cn = "remote"
  permissions = [ "*" ]
}
object ApiUser "test" {
  password = "test"
  permissions = [ "*" ]
}
)CONFIG";

			std::unique_ptr<Expression> expr = ConfigCompiler::CompileText("<test>", config);
			expr->Evaluate(*ScriptFrame::GetCurrentFrame());
		}));
	}

	void SetupHttpServerConnection(bool authenticated)
	{
		String identity = authenticated ? "remote" : "invalid";
		conn = new HttpServerConnection(wg, identity, authenticated, local);
		conn->Start();
	}

	void ShutdownTlsConn()
	{
		boost::system::error_code ec;
		if (remote->next_layer().shutdown(ec)) {
			BOOST_TEST_MESSAGE(ec.message());
			throw;
		}
	}

	boost::asio::deadline_timer timeout{IoEngine::Get().GetIoContext()};
	HttpServerConnection::Ptr conn;
	StoppableWaitGroup::Ptr wg;
};

class UnitTestHandler final: public HttpHandler
{
	bool HandleRequest(const WaitGroup::Ptr& waitGroup, HttpRequest& request, HttpResponse& response,
		boost::asio::yield_context& yc) override
	{
		response.result(boost::beast::http::status::ok);
		response.body() << "test";
		return true;
	}
};

REGISTER_URLHANDLER("/v1/test", UnitTestHandler);

BOOST_FIXTURE_TEST_SUITE(remote_httpserverconnection, HttpServerConnectionFixture)

BOOST_FIXTURE_TEST_CASE(setup_certs, SslCertificateFixture)
{
	EnsureCertFor("local");
	EnsureCertFor("remote");
}

BOOST_AUTO_TEST_CASE(expect_100_continue)
{
	CreateTestUsers();
	SetupHttpServerConnection(true);

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::get);
	request.version(11);
	request.target("https://localhost:5665/v1/test");
	request.set(http::field::expect, "100-continue");
	request.set(http::field::accept, "application/json");
	request.set(http::field::connection, "close");
	request.content_length(0);
	http::request_serializer<http::string_body> sr(request);
	http::write_header(*remote, sr);
	remote->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	http::read(*remote, buf, response);

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::continue_);

	http::write(*remote, sr);
	remote->flush();

	http::read(*remote, buf, response);
	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::ok);
	BOOST_REQUIRE_EQUAL(response.body(), "test");

	BOOST_REQUIRE_NO_THROW(ShutdownTlsConn());
}

BOOST_AUTO_TEST_CASE(bad_request)
{
	CreateTestUsers();
	SetupHttpServerConnection(true);

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::get);
	request.version(12);
	request.target("https://localhost:5665/v1/test");
	request.set(http::field::accept, "application/json");
	request.set(http::field::connection, "close");
	request.content_length(0);
	http::write(*remote, request);
	remote->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	http::read(*remote, buf, response);

	BOOST_REQUIRE_EQUAL(response.result(), http::status::bad_request);
	BOOST_REQUIRE_NE(response.body().find("<h1>Bad Request</h1>"), std::string::npos);

	BOOST_REQUIRE_NO_THROW(ShutdownTlsConn());
}

BOOST_AUTO_TEST_CASE(error_access_control)
{
	CreateTestUsers();
	CreateApiListener();
	SetupHttpServerConnection(true);

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::options);
	request.target("https://localhost:5665/v1/test");
	request.set(http::field::origin, "127.0.0.1");
	request.set(http::field::access_control_request_method, "GET");
	request.set(http::field::connection, "close");
	request.content_length(0);
	http::write(*remote, request);
	remote->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	http::read(*remote, buf, response);

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::ok);
	BOOST_REQUIRE_EQUAL(response.body(), "Preflight OK");

	boost::container::flat_set<std::string> sv;
	boost::container::flat_set options{"GET", "POST", "PUT", "DELETE", "PUSH"};

	BOOST_REQUIRE_NE(response[http::field::access_control_allow_methods], "");
	BOOST_REQUIRE_NE(response[http::field::access_control_allow_headers], "");

	BOOST_REQUIRE_NO_THROW(ShutdownTlsConn());
}

BOOST_AUTO_TEST_CASE(error_accept_header)
{
	CreateTestUsers();
	SetupHttpServerConnection(true);

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::post);
	request.target("https://localhost:5665/v1/test");
	request.set(http::field::accept, "text/html");
	request.set(http::field::connection, "close");
	request.content_length(0);
	http::write(*remote, request);
	remote->flush();

	flat_buffer buf;
	http::response<http::string_body> response;

	http::read(*remote, buf, response);

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::bad_request);
	BOOST_REQUIRE_EQUAL(response.body(), "<h1>Accept header is missing or not set to 'application/json'.</h1>\r\n");

	BOOST_REQUIRE_NO_THROW(ShutdownTlsConn());
}

BOOST_AUTO_TEST_CASE(error_authenticate_cn)
{
	CreateTestUsers();
	SetupHttpServerConnection(true);

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::get);
	request.target("https://localhost:5665/v1/test");
	request.set(http::field::accept, "application/json");
	request.set(http::field::connection, "close");
	request.content_length(0);
	http::write(*remote, request);
	remote->flush();

	flat_buffer buf;
	http::response<http::string_body> response;

	http::read(*remote, buf, response);

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::ok);

	BOOST_REQUIRE_NO_THROW(ShutdownTlsConn());
}

BOOST_AUTO_TEST_CASE(error_authenticate_passwd)
{
	CreateTestUsers();
	SetupHttpServerConnection(false);

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::get);
	request.target("https://localhost:5665/v1/test");
	request.set(http::field::authorization, "Basic " + Base64::Encode("test:test"));
	request.set(http::field::accept, "application/json");
	request.set(http::field::connection, "close");
	request.content_length(0);
	http::write(*remote, request);
	remote->flush();

	flat_buffer buf;
	http::response<http::string_body> response;

	http::read(*remote, buf, response);

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::ok);

	BOOST_REQUIRE_NO_THROW(ShutdownTlsConn());
}

BOOST_AUTO_TEST_CASE(error_authenticate_wronguser)
{
	CreateTestUsers();
	SetupHttpServerConnection(false);

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::get);
	request.target("https://localhost:5665/v1/test");
	request.set(http::field::authorization, "Basic " + Base64::Encode("invalid:invalid"));
	request.set(http::field::accept, "application/json");
	request.set(http::field::connection, "close");
	request.content_length(0);
	http::write(*remote, request);
	remote->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	http::read(*remote, buf, response);

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::unauthorized);
	Dictionary::Ptr body = JsonDecode(response.body());
	BOOST_REQUIRE(body);
	BOOST_REQUIRE_EQUAL(body->Get("error"), 401);

	BOOST_REQUIRE_NO_THROW(ShutdownTlsConn());
}

BOOST_AUTO_TEST_CASE(error_authenticate_wrongpasswd)
{
	CreateTestUsers();
	SetupHttpServerConnection(false);

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::get);
	request.target("https://localhost:5665/v1/test");
	request.set(http::field::authorization, "Basic " + Base64::Encode("test:invalid"));
	request.set(http::field::accept, "application/json");
	request.set(http::field::connection, "close");
	request.content_length(0);
	http::write(*remote, request);
	remote->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	http::read(*remote, buf, response);

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::unauthorized);
	Dictionary::Ptr body = JsonDecode(response.body());
	BOOST_REQUIRE(body);
	BOOST_REQUIRE_EQUAL(body->Get("error"), 401);

	BOOST_REQUIRE_NO_THROW(ShutdownTlsConn());
}

BOOST_AUTO_TEST_CASE(reuse_connection_and_wg_abort)
{
	CreateTestUsers();
	SetupHttpServerConnection(true);

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::get);
	request.target("https://localhost:5665/v1/test");
	request.set(http::field::accept, "application/json");
	// request.keep_alive(true);
	http::write(*remote, request);
	remote->flush();

	flat_buffer buf;
	http::response<http::string_body> response;

	http::read(*remote, buf, response);
	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::ok);
	BOOST_REQUIRE_EQUAL(response.body(), "test");

	http::write(*remote, request);
	remote->flush();

	wg->Join();

	http::read(*remote, buf, response);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::ok);
	BOOST_REQUIRE_EQUAL(response.body(), "test");

	BOOST_REQUIRE_NO_THROW(ShutdownTlsConn());
}

BOOST_AUTO_TEST_CASE(liveness_disconnect)
{
	SetupHttpServerConnection(false);

	Utility::Sleep(11);
	BOOST_REQUIRE(conn->Disconnected());

	BOOST_REQUIRE_NO_THROW(ShutdownTlsConn());
}

BOOST_FIXTURE_TEST_CASE(cleanup_certs, ConfigurationDataDirCleanupFixture) {}

BOOST_AUTO_TEST_SUITE_END()
