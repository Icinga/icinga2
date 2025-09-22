/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include <BoostTestTargetConfig.h>
#include "base/base64.hpp"
#include "base/json.hpp"
#include "remote/httphandler.hpp"
#include "test/base-testloggerfixture.hpp"
#include "test/base-tlsstream-fixture.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/beast/http.hpp>
#include <utility>

using namespace icinga;
using namespace boost::beast;
using namespace boost::unit_test_framework;

struct HttpServerConnectionFixture : TlsStreamFixture, ConfigurationCacheDirFixture, TestLoggerFixture
{
	HttpServerConnection::Ptr m_Connection;
	StoppableWaitGroup::Ptr m_WaitGroup;

	HttpServerConnectionFixture() : m_WaitGroup(new StoppableWaitGroup) {}

	static void CreateApiListener(const String& allowOrigin)
	{
		ScriptGlobal::Set("NodeName", "server");
		ApiListener::Ptr listener = new ApiListener;
		listener->OnConfigLoaded();
		listener->SetAccessControlAllowOrigin(new Array{allowOrigin});
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
		m_Connection = new HttpServerConnection(m_WaitGroup, identity, authenticated, server);
		m_Connection->Start();
	}

	template<class Rep, class Period>
	bool AssertServerDisconnected(const std::chrono::duration<Rep, Period>& timeout)
	{
		auto iterations = timeout / std::chrono::milliseconds(50);
		for (std::size_t i = 0; i < iterations && !m_Connection->Disconnected(); i++) {
			Utility::Sleep(std::chrono::duration<double>(timeout).count() / iterations);
		}
		return m_Connection->Disconnected();
	}
};

class UnitTestHandler final : public HttpHandler
{
public:
	using TestFn = std::function<void(HttpResponse& response, const boost::asio::yield_context&)>;

	static void RegisterTestFn(std::string handle, TestFn fn) { testFns[std::move(handle)] = std::move(fn); }

private:
	bool HandleRequest(const WaitGroup::Ptr&, const HttpRequest& request, HttpResponse& response,
		boost::asio::yield_context& yc) override
	{
		response.result(boost::beast::http::status::ok);

		auto path = request.Url()->GetPath();

		if (path.size() == 3) {
			if (auto it = testFns.find(path[2].GetData()); it != testFns.end()) {
				it->second(response, yc);
				return true;
			}
		}

		response.body() << "test";
		return true;
	}

	static inline std::unordered_map<std::string, TestFn> testFns;
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
	request.target("/v1/test");
	request.set(http::field::expect, "100-continue");
	request.set(http::field::host, "localhost:5665");
	request.set(http::field::accept, "application/json");
	request.set(http::field::connection, "close");
	request.content_length(0);
	http::request_serializer<http::string_body> sr(request);
	http::write_header(*client, sr);
	client->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	BOOST_REQUIRE_NO_THROW(http::read(*client, buf, response));

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::continue_);

	http::write(*client, sr);
	client->flush();

	BOOST_REQUIRE_NO_THROW(http::read(*client, buf, response));
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
	request.target("/v1/test");
	request.set(http::field::host, "localhost:5665");
	request.set(http::field::accept, "application/json");
	request.set(http::field::connection, "close");
	request.content_length(0);
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	BOOST_REQUIRE_NO_THROW(http::read(*client, buf, response));

	BOOST_REQUIRE_EQUAL(response.result(), http::status::bad_request);
	BOOST_REQUIRE_NE(response.body().find("<h1>Bad Request</h1>"), std::string::npos);

	BOOST_REQUIRE(Shutdown(client));
}

BOOST_AUTO_TEST_CASE(error_access_control)
{
	CreateTestUsers();
	CreateApiListener("example.org");
	SetupHttpServerConnection(true);

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::options);
	request.target("/v1/test");
	request.set(http::field::origin, "example.org");
	request.set(http::field::host, "localhost:5665");
	request.set(http::field::access_control_request_method, "GET");
	request.set(http::field::connection, "close");
	request.content_length(0);
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	BOOST_REQUIRE_NO_THROW(http::read(*client, buf, response));

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::ok);
	BOOST_REQUIRE_EQUAL(response.body(), "Preflight OK");

	BOOST_REQUIRE_EQUAL(response[http::field::access_control_allow_credentials], "true");
	BOOST_REQUIRE_EQUAL(response[http::field::access_control_allow_origin], "example.org");
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
	request.target("/v1/test");
	request.set(http::field::host, "localhost:5665");
	request.set(http::field::accept, "text/html");
	request.set(http::field::connection, "close");
	request.content_length(0);
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	BOOST_REQUIRE_NO_THROW(http::read(*client, buf, response));

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::bad_request);
	BOOST_REQUIRE_EQUAL(response.body(), "<h1>Accept header is missing or not set to 'application/json'.</h1>");

	BOOST_REQUIRE(Shutdown(client));
}

BOOST_AUTO_TEST_CASE(authenticate_cn)
{
	CreateTestUsers();
	SetupHttpServerConnection(true);

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::get);
	request.target("/v1/test");
	request.set(http::field::host, "localhost:5665");
	request.set(http::field::accept, "application/json");
	request.set(http::field::connection, "close");
	request.content_length(0);
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	BOOST_REQUIRE_NO_THROW(http::read(*client, buf, response));

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::ok);

	BOOST_REQUIRE(Shutdown(client));
}

BOOST_AUTO_TEST_CASE(authenticate_passwd)
{
	CreateTestUsers();
	SetupHttpServerConnection(false);

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::get);
	request.target("/v1/test");
	request.set(http::field::authorization, "Basic " + Base64::Encode("test:test"));
	request.set(http::field::host, "localhost:5665");
	request.set(http::field::accept, "application/json");
	request.set(http::field::connection, "close");
	request.content_length(0);
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	BOOST_REQUIRE_NO_THROW(http::read(*client, buf, response));

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::ok);

	BOOST_REQUIRE(Shutdown(client));
}

BOOST_AUTO_TEST_CASE(authenticate_error_wronguser)
{
	CreateTestUsers();
	SetupHttpServerConnection(false);

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::get);
	request.target("/v1/test");
	request.set(http::field::authorization, "Basic " + Base64::Encode("invalid:invalid"));
	request.set(http::field::host, "localhost:5665");
	request.set(http::field::accept, "application/json");
	request.set(http::field::connection, "close");
	request.content_length(0);
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	BOOST_REQUIRE_NO_THROW(http::read(*client, buf, response));

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::unauthorized);
	Dictionary::Ptr body = JsonDecode(response.body());
	BOOST_REQUIRE(body);
	BOOST_REQUIRE_EQUAL(body->Get("error"), 401);

	BOOST_REQUIRE(Shutdown(client));
}

BOOST_AUTO_TEST_CASE(authenticate_error_wrongpasswd)
{
	CreateTestUsers();
	SetupHttpServerConnection(false);

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::get);
	request.target("/v1/test");
	request.set(http::field::authorization, "Basic " + Base64::Encode("test:invalid"));
	request.set(http::field::host, "localhost:5665");
	request.set(http::field::accept, "application/json");
	request.set(http::field::connection, "close");
	request.content_length(0);
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	BOOST_REQUIRE_NO_THROW(http::read(*client, buf, response));

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
	request.target("/v1/test");
	request.set(http::field::host, "localhost:5665");
	request.set(http::field::accept, "application/json");
	request.keep_alive(true);
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	BOOST_REQUIRE_NO_THROW(http::read(*client, buf, response));

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::ok);
	BOOST_REQUIRE_EQUAL(response.body(), "test");

	request.keep_alive(false);
	http::write(*client, request);
	client->flush();

	boost::system::error_code ec;
	http::response_parser<http::string_body> parser;
	BOOST_REQUIRE_NO_THROW(http::read(*client, buf, parser));

	BOOST_REQUIRE(parser.is_header_done());
	BOOST_REQUIRE(parser.is_done());
	BOOST_REQUIRE_EQUAL(parser.get().version(), 11);
	BOOST_REQUIRE_EQUAL(parser.get().result(), http::status::ok);
	BOOST_REQUIRE_EQUAL(parser.get().body(), "test");

	// Second read to get the end of stream error;
	http::read(*client, buf, response, ec);
	BOOST_REQUIRE_EQUAL(ec, boost::system::error_code{boost::beast::http::error::end_of_stream});

	BOOST_REQUIRE(AssertServerDisconnected(std::chrono::seconds(5)));
	BOOST_REQUIRE(Shutdown(client));
	BOOST_REQUIRE(ExpectLogPattern("HTTP client disconnected .*", std::chrono::seconds(5)));
}

BOOST_AUTO_TEST_CASE(wg_abort)
{
	CreateTestUsers();
	SetupHttpServerConnection(true);

	UnitTestHandler::RegisterTestFn("wgjoin", [this](HttpResponse& response, const boost::asio::yield_context&) {
		response.body() << "test";
		m_WaitGroup->Join();
	});

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::get);
	request.target("/v1/test/wgjoin");
	request.set(http::field::host, "localhost:5665");
	request.set(http::field::accept, "application/json");
	request.keep_alive(true);
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response_parser<http::string_body> parser;
	BOOST_REQUIRE_NO_THROW(http::read(*client, buf, parser));

	BOOST_REQUIRE(parser.is_header_done());
	BOOST_REQUIRE(parser.is_done());
	BOOST_REQUIRE_EQUAL(parser.get().version(), 11);
	BOOST_REQUIRE_EQUAL(parser.get().result(), http::status::ok);
	BOOST_REQUIRE_EQUAL(parser.get().body(), "test");

	// Second read to get the end of stream error;
	http::response<http::string_body> response{};
	boost::system::error_code ec;
	http::read(*client, buf, response, ec);
	BOOST_REQUIRE_EQUAL(ec, boost::system::error_code{boost::beast::http::error::end_of_stream});

	BOOST_REQUIRE(AssertServerDisconnected(std::chrono::seconds(5)));
	BOOST_REQUIRE(Shutdown(client));
	BOOST_REQUIRE(ExpectLogPattern("HTTP client disconnected .*", std::chrono::seconds(5)));
}

BOOST_AUTO_TEST_CASE(client_shutdown)
{
	CreateTestUsers();
	SetupHttpServerConnection(true);

	UnitTestHandler::RegisterTestFn("stream", [](HttpResponse& response, const boost::asio::yield_context& yc) {
		response.StartStreaming();
		response.Flush(yc);

		boost::asio::deadline_timer dt{IoEngine::Get().GetIoContext()};
		for (;;) {
			dt.expires_from_now(boost::posix_time::seconds(1));
			dt.async_wait(yc);

			if (!response.IsClientDisconnected()) {
				return;
			}

			response.body() << "test";
			response.Flush(yc);
		}
	});

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::get);
	request.target("/v1/test/stream");
	request.set(http::field::host, "localhost:5665");
	request.set(http::field::accept, "application/json");
	request.keep_alive(true);
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response_parser<http::string_body> parser;
	BOOST_REQUIRE_NO_THROW(http::read_header(*client, buf, parser));
	BOOST_REQUIRE(parser.is_header_done());

	/* Unlike the other test cases we don't require success here, because with the request
	 * above, UnitTestHandler simulates a HttpHandler that is constantly writing.
	 * That may cause the shutdown to fail on the client-side with "application data after
	 * close notify", but the important part is that HttpServerConnection actually closes
	 * the connection on its own side, which we check with the BOOST_REQUIRE() below.
	 */
	BOOST_WARN(Shutdown(client));

	BOOST_REQUIRE(AssertServerDisconnected(std::chrono::seconds(5)));
	BOOST_REQUIRE(ExpectLogPattern("HTTP client disconnected .*", std::chrono::seconds(5)));
}

BOOST_AUTO_TEST_CASE(handler_throw_error)
{
	CreateTestUsers();
	SetupHttpServerConnection(true);

	UnitTestHandler::RegisterTestFn("throw", [](HttpResponse& response, const boost::asio::yield_context&) {
		response.StartStreaming();
		response.body() << "test";

		boost::system::error_code ec{};
		throw boost::system::system_error(ec);
	});

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::get);
	request.target("/v1/test/throw");
	request.set(http::field::host, "localhost:5665");
	request.set(http::field::accept, "application/json");
	request.keep_alive(false);
	http::write(*client, request);
	client->flush();

	flat_buffer buf;
	http::response<http::string_body> response;
	BOOST_REQUIRE_NO_THROW(http::read(*client, buf, response));

	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.result(), http::status::internal_server_error);
	Dictionary::Ptr body = JsonDecode(response.body());
	BOOST_REQUIRE(body);
	BOOST_REQUIRE_EQUAL(body->Get("error"), 500);
	BOOST_REQUIRE_EQUAL(body->Get("status"), "Unhandled exception");

	BOOST_REQUIRE(Shutdown(client));
	BOOST_REQUIRE(ExpectLogPattern("HTTP client disconnected .*", std::chrono::seconds(5)));
	BOOST_REQUIRE(!ExpectLogPattern("Exception while processing HTTP request.*"));
}

BOOST_AUTO_TEST_CASE(handler_throw_streaming)
{
	CreateTestUsers();
	SetupHttpServerConnection(true);

	UnitTestHandler::RegisterTestFn("throw", [](HttpResponse& response, const boost::asio::yield_context& yc) {
		response.StartStreaming();
		response.body() << "test";

		response.Flush(yc);

		boost::system::error_code ec{};
		throw boost::system::system_error(ec);
	});

	http::request<boost::beast::http::string_body> request;
	request.method(http::verb::get);
	request.target("/v1/test/throw");
	request.set(http::field::host, "localhost:5665");
	request.set(http::field::accept, "application/json");
	request.keep_alive(true);

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
	BOOST_REQUIRE(ExpectLogPattern("HTTP client disconnected .*", std::chrono::seconds(5)));
	BOOST_REQUIRE(ExpectLogPattern("Exception while processing HTTP request.*"));
}

BOOST_AUTO_TEST_CASE(liveness_disconnect)
{
	SetupHttpServerConnection(false);

	BOOST_REQUIRE(AssertServerDisconnected(std::chrono::seconds(11)));
	BOOST_REQUIRE(ExpectLogPattern("HTTP client disconnected .*"));
	BOOST_REQUIRE(ExpectLogPattern("No messages for HTTP connection have been received in the last 10 seconds."));
	BOOST_REQUIRE(Shutdown(client));
}

BOOST_AUTO_TEST_SUITE_END()
