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
#include <set>
#include <BoostTestTargetConfig.h>

using namespace icinga;
using namespace boost::beast;

struct HttpMessageFixture: TlsStreamFixture
{
};

BOOST_FIXTURE_TEST_SUITE(remote_httpmessage, HttpMessageFixture)

BOOST_AUTO_TEST_CASE(request_parse)
{
	std::atomic<bool> done = false;

	http::request<boost::beast::http::string_body> requestOut;
	requestOut.method(http::verb::get);
	requestOut.target("https://localhost:5665/v1/test");
	requestOut.set(http::field::authorization, "Basic " + Base64::Encode("invalid:invalid"));
	requestOut.set(http::field::accept, "application/json");
	requestOut.set(http::field::connection, "close");
	requestOut.content_length(0);

	auto & io = IoEngine::Get();
	io.SpawnCoroutine(io.GetIoContext(), [local = local, &done, &requestOut](boost::asio::yield_context yc) {
		boost::beast::flat_buffer buf;
		HttpRequest request(local);
		local->async_fill(yc);
		BOOST_REQUIRE_NO_THROW(request.ParseHeader(buf, yc));

		for (auto & field : requestOut.base()) {
			BOOST_REQUIRE(request.count(field.name()));
		}

		request.ParseBody(buf, yc);

		local->next_layer().async_shutdown(yc);
	});

	http::write(*remote, requestOut);
	remote->flush();

	BOOST_REQUIRE_NO_THROW(remote->next_layer().shutdown());
}

BOOST_AUTO_TEST_CASE(request_params)
{
	HttpRequest request(remote);
	request.body() = JsonEncode(new Dictionary{{"test1", false}, {"test2", true}});
	std::cout << request.body();
	request.target("https://localhost:1234/v1/test?test1&test3&test3");
	request.DecodeParams();

	BOOST_REQUIRE_EQUAL(request.Params()->Get("test2"), "true");
	BOOST_REQUIRE(request.Params()->Get("test1").IsObjectType<Array>());
	BOOST_REQUIRE_EQUAL(request.GetLastParameter("test1"), "true");
	BOOST_REQUIRE(request.Params()->Get("test3").IsObjectType<Array>());
	BOOST_REQUIRE_EQUAL(request.GetLastParameter("test3"), "true");
}

BOOST_AUTO_TEST_CASE(response_stream_operator)
{

}

BOOST_AUTO_TEST_CASE(response_body_reader)
{

}

BOOST_AUTO_TEST_CASE(response_body_writer)
{

}

BOOST_AUTO_TEST_CASE(response_write_fixed)
{

}

BOOST_AUTO_TEST_CASE(response_write_chunked)
{

}

BOOST_AUTO_TEST_CASE(response_sendjsonbody)
{

}

BOOST_AUTO_TEST_SUITE_END()
