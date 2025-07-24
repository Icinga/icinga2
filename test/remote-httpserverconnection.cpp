/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "base-tlsstream-fixture.hpp"
#include "icingaapplication-fixture.hpp"
#include "base-testloggerfixture.hpp"
#include "base/base64.hpp"
#include "base/json.hpp"
#include "remote/httphandler.hpp"
#include "remote/httputility.hpp"
#include <boost/beast/http.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/version.hpp>
#include <BoostTestTargetConfig.h>

using namespace icinga;
using namespace boost::beast;
// using namespace boost::unit_test;
using namespace boost::unit_test_framework;

struct HttpServerConnectionFixture : TlsStreamFixture,
									 IcingaApplicationFixture,
									 ConfigurationCacheDirFixture,
									 TestLoggerFixture
{
	HttpServerConnection::Ptr conn;
	StoppableWaitGroup::Ptr wg;

	HttpServerConnectionFixture()
		: wg(new StoppableWaitGroup)
	{
	}

	static void CreateApiListener()
	{
		ScriptGlobal::Set("NodeName", "server");
		ApiListener::Ptr listener = new ApiListener;
		listener->OnConfigLoaded();
		listener->SetAccessControlAllowOrigin(new Array{"127.0.0.1"});
	}

	static void CreateTestUsers()
	{
		ApiUser::Ptr user = new ApiUser;
		user->SetName("client");
		user->SetClientCN("client");
		user->SetPermissions(new Array{"*"});
		user->Register();

		user = new ApiUser;
		user->SetName("test");
		user->SetPassword("test");
		user->SetPermissions(new Array{"*"});
		user->Register();
	}

	void SetupHttpServerConnection(bool authenticated)
	{
		String identity = authenticated ? "client" : "invalid";
		conn = new HttpServerConnection(wg, identity, authenticated, server);
		conn->Start();
	}

	template <class Rep, class Period>
	bool AssertServerDisconnected(const std::chrono::duration<Rep, Period>& timeout)
	{
		auto iterations = timeout / std::chrono::milliseconds(50);
		for(std::size_t i=0; i<iterations && !conn->Disconnected(); i++){
			Utility::Sleep(std::chrono::duration<double>(timeout).count()/iterations);
		}
		return conn->Disconnected();
	}
};

class UnitTestHandler final: public HttpHandler
{

	bool HandleRequest(const WaitGroup::Ptr& waitGroup, const HttpRequest& request, HttpResponse& response,
		boost::asio::yield_context& yc) override
	{
		response.result(boost::beast::http::status::ok);

		if (HttpUtility::GetLastParameter(request.Params(), "stream")) {
			response.StartStreaming();
			response.Flush(yc);
			for (;;) {
				response << "test";
				response.Flush(yc);
			}

			return true;
		}

		if (HttpUtility::GetLastParameter(request.Params(), "throw")) {
			response.StartStreaming();
			response << "test";
			response.Flush(yc);
			throw std::runtime_error{"The front fell off"};
		}

		response << "test";
		return true;
	}
};

REGISTER_URLHANDLER("/v1/test", UnitTestHandler);

BOOST_FIXTURE_TEST_SUITE(remote_httpserverconnection, HttpServerConnectionFixture)

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
	http::write_header(*client, sr);
	client->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	http::read(*client, buf, response);

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::continue_);

	http::write(*client, sr);
	client->flush();

	http::read(*client, buf, response);
	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::ok);
	BOOST_REQUIRE_EQUAL(response.body(), "test");

	BOOST_REQUIRE(Shutdown(client));
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
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	http::read(*client, buf, response);

	BOOST_REQUIRE_EQUAL(response.result(), http::status::bad_request);
	BOOST_REQUIRE_NE(response.body().find("<h1>Bad Request</h1>"), std::string::npos);

	BOOST_REQUIRE(Shutdown(client));
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
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	http::read(*client, buf, response);

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::ok);
	BOOST_REQUIRE_EQUAL(response.body(), "Preflight OK");

	boost::container::flat_set<std::string> sv;
	boost::container::flat_set options{"GET", "POST", "PUT", "DELETE", "PUSH"};

	BOOST_REQUIRE_NE(response[http::field::access_control_allow_methods], "");
	BOOST_REQUIRE_NE(response[http::field::access_control_allow_headers], "");

	BOOST_REQUIRE(Shutdown(client));
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
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	http::read(*client, buf, response);

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::bad_request);
	BOOST_REQUIRE_EQUAL(response.body(), "<h1>Accept header is missing or not set to 'application/json'.</h1>\r\n");

	BOOST_REQUIRE(Shutdown(client));
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
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	http::read(*client, buf, response);

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::ok);

	BOOST_REQUIRE(Shutdown(client));
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
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	http::read(*client, buf, response);

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::ok);

	BOOST_REQUIRE(Shutdown(client));
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
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	http::read(*client, buf, response);

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::unauthorized);
	Dictionary::Ptr body = JsonDecode(response.body());
	BOOST_REQUIRE(body);
	BOOST_REQUIRE_EQUAL(body->Get("error"), 401);

	BOOST_REQUIRE(Shutdown(client));
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
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	http::read(*client, buf, response);

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::unauthorized);
	Dictionary::Ptr body = JsonDecode(response.body());
	BOOST_REQUIRE(body);
	BOOST_REQUIRE_EQUAL(body->Get("error"), 401);

	BOOST_REQUIRE(Shutdown(client));
}

BOOST_AUTO_TEST_CASE(reuse_connection)
{
	CreateTestUsers();
	SetupHttpServerConnection(true);

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::get);
	request.target("https://localhost:5665/v1/test");
	request.set(http::field::accept, "application/json");
	request.keep_alive(true);
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	http::read(*client, buf, response);

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::ok);
	BOOST_REQUIRE_EQUAL(response.body(), "test");

	request.keep_alive(false);
	http::write(*client, request);
	client->flush();

	response.body() = "";
	http::read(*client, buf, response);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::ok);
	BOOST_REQUIRE_EQUAL(response.body(), "test");

	BOOST_REQUIRE(AssertServerDisconnected(std::chrono::seconds(5)));
	BOOST_REQUIRE(Shutdown(client));
	BOOST_REQUIRE(ExpectLogPattern("HTTP client disconnected .*", std::chrono::seconds(5)));
}

BOOST_AUTO_TEST_CASE(wg_abort)
{
	CreateTestUsers();
	SetupHttpServerConnection(true);

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::get);
	request.target("https://localhost:5665/v1/test");
	request.set(http::field::accept, "application/json");
	request.keep_alive(true);
	http::write(*client, request);
	client->flush();

	wg->Join();

	flat_buffer buf;
	http::response<http::string_body> response;
	boost::system::error_code ec;
	http::read(*client, buf, response, ec);

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::ok);
	BOOST_REQUIRE_EQUAL(response.body(), "test");

	if (!ec) {
		/* Sometimes the call to read above already catches the end_of_stream error, sometimes
		 * it stops just shy of that byte and we'll want to do another dummy read to catch it
		 * here.
		 */
		http::read(*client, buf, response, ec);
	}
	BOOST_REQUIRE_EQUAL(ec, boost::system::error_code{boost::beast::http::error::end_of_stream});

	BOOST_REQUIRE(AssertServerDisconnected(std::chrono::seconds(5)));
	BOOST_REQUIRE(Shutdown(client));
	BOOST_REQUIRE(ExpectLogPattern("HTTP client disconnected .*", std::chrono::seconds(5)));
}

BOOST_AUTO_TEST_CASE(client_shutdown)
{
	CreateTestUsers();
	SetupHttpServerConnection(true);

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::get);
	request.target("https://localhost:5665/v1/test");
	request.set(http::field::accept, "application/json");
	request.keep_alive(true);

	/* Instruct the test HttpHandler defined above to stream an endless response.
	 */
	request.body() = JsonEncode(new Dictionary{{"stream", true}});
	request.prepare_payload();
	http::write(*client, request);
	client->flush();

	/* Unlike the other test cases we don't require success here, because with the request
	 * above, UnitTestHandler simulates a HttpHandler that is constantly writing.
	 * That will cause the shutdown to fail on the client-side with "application data after
	 * close notify", but the important part is that HttpServerConnection actually closes
	 * the connection on its own side, which we check with the BOOST_REQUIRE() below.
	 */
	BOOST_WARN(Shutdown(client));

	BOOST_REQUIRE(AssertServerDisconnected(std::chrono::seconds(5)));
	BOOST_REQUIRE(ExpectLogPattern("HTTP client disconnected.*", std::chrono::seconds(5)));
	BOOST_REQUIRE(ExpectLogPattern("Detected shutdown from client: .*"));
}

BOOST_AUTO_TEST_CASE(handler_throw)
{
	CreateTestUsers();
	SetupHttpServerConnection(true);

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::get);
	request.target("https://localhost:5665/v1/test");
	request.set(http::field::accept, "application/json");
	request.keep_alive(true);

	/* Instruct the test TestHandler to throw an exception while streaming, which the
	 * HttpHandler base should propagate up to HttpServerConnection.
	 * The correct response is to shutdown the connection instead of trying to send a JSON
	 * error message.
	 */
	request.body() = JsonEncode(new Dictionary{{"throw", true}});
	request.prepare_payload();
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response_parser<http::string_body> parser;
	boost::system::error_code ec;
	http::read(*client, buf, parser, ec);

	/* Since the handler threw in the middle of sending the message we shouldn't be able
	 * to read a complete message here.
	 */
	BOOST_REQUIRE_EQUAL(ec, boost::system::error_code{boost::beast::http::error::partial_message});

	/* The body should only contain the single "test" the handler has written, without any
	 * attempts made to additionally write some json error message.
	 */
	BOOST_REQUIRE_EQUAL(parser.get().body(), "test");

	/* We then expect the server to initiate a shutdown, which we then complete below.
	 */
	BOOST_REQUIRE(AssertServerDisconnected(std::chrono::seconds(5)));
	BOOST_REQUIRE(Shutdown(client));
	BOOST_REQUIRE(ExpectLogPattern("HTTP client disconnected.*", std::chrono::seconds(5)));
	BOOST_REQUIRE(ExpectLogPattern("Exception while processing HTTP request from .+?: The front fell off"));
}

BOOST_AUTO_TEST_CASE(liveness_disconnect)
{
	SetupHttpServerConnection(false);

	BOOST_REQUIRE(AssertServerDisconnected(std::chrono::seconds(11)));
	BOOST_REQUIRE(ExpectLogPattern("HTTP client disconnected .*"));
	BOOST_REQUIRE(Shutdown(client));
}

BOOST_AUTO_TEST_SUITE_END()
