// SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <BoostTestTargetConfig.h>
#include "base/base64.hpp"
#include "base/json.hpp"
#include "remote/httpmessage.hpp"
#include "remote/httputility.hpp"
#include "test/base-tlsstream-fixture.hpp"
#include "test/test-ctest.hpp"
#include <fstream>
#include <utility>

using namespace icinga;
using namespace boost::beast;

static std::future<void> SpawnSynchronizedCoroutine(std::function<void(boost::asio::yield_context)> fn)
{
	auto promise = std::make_unique<std::promise<void>>();
	auto future = promise->get_future();
	auto& io = IoEngine::Get().GetIoContext();
	IoEngine::SpawnCoroutine(io, [promise = std::move(promise), fn = std::move(fn)](boost::asio::yield_context yc) {
		try {
			fn(std::move(yc));
		} catch (const std::exception&) {
			promise->set_exception(std::current_exception());
			return;
		}
		promise->set_value();
	});
	return future;
}

// clang-format off
BOOST_FIXTURE_TEST_SUITE(remote_httpmessage, TlsStreamFixture,
	*CTestProperties("FIXTURES_REQUIRED ssl_certs")
	*boost::unit_test::label("http"))
// clang-format on

BOOST_AUTO_TEST_CASE(request_parse)
{
	http::request<boost::beast::http::string_body> requestOut;
	requestOut.method(http::verb::get);
	requestOut.target("https://localhost:5665/v1/test");
	requestOut.set(http::field::authorization, "Basic " + Base64::Encode("invalid:invalid"));
	requestOut.set(http::field::accept, "application/json");
	requestOut.set(http::field::connection, "close");
	requestOut.body() = "test";
	requestOut.prepare_payload();

	auto future = SpawnSynchronizedCoroutine([this, &requestOut](boost::asio::yield_context yc) {
		boost::beast::flat_buffer buf;
		HttpApiRequest request(server);
		BOOST_REQUIRE_NO_THROW(request.ParseHeader(buf, yc));

		for (const auto& field : requestOut.base()) {
			BOOST_REQUIRE(request.count(field.name()));
		}

		BOOST_REQUIRE_NO_THROW(request.ParseBody(buf, yc));
		BOOST_REQUIRE_EQUAL(request.body(), "test");

		Shutdown(server, yc);
	});

	http::write(*client, requestOut);
	client->flush();

	Shutdown(client);
	future.get();
}

BOOST_AUTO_TEST_CASE(request_params)
{
	HttpApiRequest request(client);
	// clang-format off
	request.body() = JsonEncode(
		new Dictionary{
			{"bool-in-json", true},
			{"bool-in-url-and-json", true},
			{"string-in-json", "json-value"},
			{"string-in-url-and-json", "json-value"}
		});
	request.target("https://localhost:1234/v1/test?"
		"bool-in-url-and-json=0&"
		"bool-in-url=1&"
		"string-in-url-and-json=url-value&"
		"string-only-in-url=url-value"
	);
	// clang-format on

	// Test pointer being valid after decode
	request.DecodeParams();
	auto params = request.Params();
	BOOST_REQUIRE(params);

	// Test JSON-only params being parsed as their correct type
	BOOST_REQUIRE(params->Get("bool-in-json").IsBoolean());
	BOOST_REQUIRE(params->Get("string-in-json").IsString());
	BOOST_REQUIRE(params->Get("bool-in-url-and-json").IsObjectType<Array>());
	BOOST_REQUIRE(params->Get("string-in-url-and-json").IsObjectType<Array>());

	// Test 0/1 string values from URL evaluate to true and false
	// These currently get implicitly converted to double and then to bool, but this is an
	// implementation we don't need to test for here.
	BOOST_REQUIRE_EQUAL(HttpUtility::GetLastParameter(params, "bool-in-url-and-json"), "0");
	BOOST_REQUIRE(!HttpUtility::GetLastParameter(params, "bool-in-url-and-json"));
	BOOST_REQUIRE_EQUAL(HttpUtility::GetLastParameter(params, "bool-in-url"), "1");
	BOOST_REQUIRE(HttpUtility::GetLastParameter(params, "bool-in-url"));

	// Test non-existing parameters evaluate to false
	BOOST_REQUIRE(HttpUtility::GetLastParameter(params, "does-not-exist").IsEmpty());
	BOOST_REQUIRE(!HttpUtility::GetLastParameter(params, "does-not-exist"));

	// Test precedence of URL params over JSON params
	BOOST_REQUIRE_EQUAL(HttpUtility::GetLastParameter(params, "string-in-json"), "json-value");
	BOOST_REQUIRE_EQUAL(HttpUtility::GetLastParameter(params, "string-in-url-and-json"), "url-value");
	BOOST_REQUIRE_EQUAL(HttpUtility::GetLastParameter(params, "string-only-in-url"), "url-value");
}

BOOST_AUTO_TEST_CASE(response_clear)
{
	HttpApiResponse response(server);
	response.result(http::status::bad_request);
	response.version(10);
	response.set(http::field::content_type, "text/html");
	response.body() << "test";

	response.Clear();

	BOOST_REQUIRE(response[http::field::content_type].empty());
	BOOST_REQUIRE_EQUAL(response.result(), http::status::ok);
	BOOST_REQUIRE_EQUAL(response.version(), 11);
	BOOST_REQUIRE_EQUAL(response.body().Size(), 0);
}

BOOST_AUTO_TEST_CASE(response_flush_nothrow)
{
	auto future = SpawnSynchronizedCoroutine([this](const boost::asio::yield_context& yc) {
		HttpApiResponse response(server);
		response.result(http::status::ok);

		server->lowest_layer().close();

		boost::beast::error_code ec;
		BOOST_REQUIRE_NO_THROW(response.Flush(yc[ec]));
		BOOST_REQUIRE_EQUAL(ec, boost::system::errc::bad_file_descriptor);
	});

	auto status = future.wait_for(std::chrono::seconds(1));
	BOOST_REQUIRE(status == std::future_status::ready);
}

BOOST_AUTO_TEST_CASE(response_flush_throw)
{
	auto future = SpawnSynchronizedCoroutine([this](const boost::asio::yield_context& yc) {
		HttpApiResponse response(server);
		response.result(http::status::ok);

		server->lowest_layer().close();

		BOOST_REQUIRE_EXCEPTION(response.Flush(yc), std::exception, [](const std::exception& ex) {
			auto se = dynamic_cast<const boost::system::system_error*>(&ex);
			return se && se->code() == boost::system::errc::bad_file_descriptor;
		});
	});

	auto status = future.wait_for(std::chrono::seconds(1));
	BOOST_REQUIRE(status == std::future_status::ready);
}

BOOST_AUTO_TEST_CASE(response_write_empty)
{
	auto future = SpawnSynchronizedCoroutine([this](boost::asio::yield_context yc) {
		HttpApiResponse response(server);
		response.result(http::status::ok);

		BOOST_REQUIRE_NO_THROW(response.Flush(yc));

		Shutdown(server, yc);
	});

	http::response_parser<http::string_body> parser;
	flat_buffer buf;
	boost::system::error_code ec;
	http::read(*client, buf, parser, ec);

	Shutdown(client);

	future.get();

	BOOST_REQUIRE(!ec);
	BOOST_REQUIRE_EQUAL(parser.get().result(), http::status::ok);
	BOOST_REQUIRE_EQUAL(parser.get().chunked(), false);
	BOOST_REQUIRE_EQUAL(parser.get().body(), "");
}

BOOST_AUTO_TEST_CASE(response_write_fixed)
{
	auto future = SpawnSynchronizedCoroutine([this](boost::asio::yield_context yc) {
		HttpApiResponse response(server);
		response.result(http::status::ok);
		response.body() << "test";

		BOOST_REQUIRE_NO_THROW(response.Flush(yc));

		Shutdown(server, yc);
	});

	http::response_parser<http::string_body> parser;
	flat_buffer buf;
	boost::system::error_code ec;
	http::read(*client, buf, parser, ec);

	Shutdown(client);

	future.get();

	BOOST_REQUIRE(!ec);
	BOOST_REQUIRE_EQUAL(parser.get().result(), http::status::ok);
	BOOST_REQUIRE_EQUAL(parser.get().chunked(), false);
	BOOST_REQUIRE_EQUAL(parser.get().body(), "test");
}

BOOST_AUTO_TEST_CASE(response_write_chunked)
{
	// NOLINTNEXTLINE(readability-function-cognitive-complexity)
	auto future = SpawnSynchronizedCoroutine([this](boost::asio::yield_context yc) {
		HttpApiResponse response(server);
		response.result(http::status::ok);

		response.StartStreaming(false);
		BOOST_REQUIRE_NO_THROW(response.Flush(yc));
		BOOST_REQUIRE(response.HasSerializationStarted());

		response.body() << "test" << 1;
		BOOST_REQUIRE_NO_THROW(response.Flush(yc));

		response.body() << "test" << 2;
		BOOST_REQUIRE_NO_THROW(response.Flush(yc));

		response.body() << "test" << 3;
		response.body().Finish();
		BOOST_REQUIRE_NO_THROW(response.Flush(yc));

		Shutdown(server, yc);
	});

	http::response_parser<http::string_body> parser;
	flat_buffer buf;
	boost::system::error_code ec;
	http::read(*client, buf, parser, ec);

	Shutdown(client);

	future.get();

	BOOST_REQUIRE(!ec);
	BOOST_REQUIRE_EQUAL(parser.get().result(), http::status::ok);
	BOOST_REQUIRE_EQUAL(parser.get().chunked(), true);
	BOOST_REQUIRE_EQUAL(parser.get().body(), "test1test2test3");
}

BOOST_AUTO_TEST_CASE(response_sendjsonbody)
{
	auto future = SpawnSynchronizedCoroutine([this](boost::asio::yield_context yc) {
		HttpApiResponse response(server);
		response.result(http::status::ok);

		HttpUtility::SendJsonBody(response, nullptr, new Dictionary{{"test", 1}});

		BOOST_REQUIRE_NO_THROW(response.Flush(yc));

		Shutdown(server, yc);
	});

	http::response_parser<http::string_body> parser;
	flat_buffer buf;
	boost::system::error_code ec;
	http::read(*client, buf, parser, ec);

	Shutdown(client);

	future.get();

	BOOST_REQUIRE(!ec);
	BOOST_REQUIRE_EQUAL(parser.get().result(), http::status::ok);
	BOOST_REQUIRE_EQUAL(parser.get().chunked(), false);
	Dictionary::Ptr body = JsonDecode(parser.get().body());
	BOOST_REQUIRE_EQUAL(body->Get("test"), 1);
}

BOOST_AUTO_TEST_CASE(response_sendjsonerror)
{
	auto future = SpawnSynchronizedCoroutine([this](boost::asio::yield_context yc) {
		HttpApiResponse response(server);

		// This has to be overwritten in SendJsonError.
		response.result(http::status::ok);

		HttpUtility::SendJsonError(response, nullptr, 404, "Not found.");

		BOOST_REQUIRE_NO_THROW(response.Flush(yc));

		Shutdown(server, yc);
	});

	http::response_parser<http::string_body> parser;
	flat_buffer buf;
	boost::system::error_code ec;
	http::read(*client, buf, parser, ec);

	Shutdown(client);

	future.get();

	BOOST_REQUIRE(!ec);
	BOOST_REQUIRE_EQUAL(parser.get().result(), http::status::not_found);
	BOOST_REQUIRE_EQUAL(parser.get().chunked(), false);
	Dictionary::Ptr body = JsonDecode(parser.get().body());
	BOOST_REQUIRE_EQUAL(body->Get("error"), 404);
	BOOST_REQUIRE_EQUAL(body->Get("status"), "Not found.");
}

BOOST_AUTO_TEST_CASE(response_sendfile)
{
	auto future = SpawnSynchronizedCoroutine([this](boost::asio::yield_context yc) {
		HttpApiResponse response(server);

		response.result(http::status::ok);
		BOOST_REQUIRE_NO_THROW(response.SendFile(m_CaCrtFile.string(), yc));
		BOOST_REQUIRE_NO_THROW(response.Flush(yc));

		Shutdown(server, yc);
	});

	http::response_parser<http::string_body> parser;
	flat_buffer buf;
	boost::system::error_code ec;
	http::read(*client, buf, parser, ec);

	Shutdown(client);

	future.get();

	BOOST_REQUIRE(!ec);
	BOOST_REQUIRE_EQUAL(parser.get().result(), http::status::ok);
	BOOST_REQUIRE_EQUAL(parser.get().chunked(), false);

	std::ifstream fp(m_CaCrtFile.string(), std::ifstream::in | std::ifstream::binary);
	fp.exceptions(std::ifstream::badbit);
	std::stringstream ss;
	ss << fp.rdbuf();
	BOOST_REQUIRE_EQUAL(ss.str(), parser.get().body());
}

BOOST_AUTO_TEST_SUITE_END()
