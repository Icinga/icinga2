/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/httpserverconnection.hpp"
#include "remote/httphandler.hpp"
#include "remote/httputility.hpp"
#include "remote/apilistener.hpp"
#include "remote/apifunction.hpp"
#include "remote/jsonrpc.hpp"
#include "base/application.hpp"
#include "base/base64.hpp"
#include "base/convert.hpp"
#include "base/configtype.hpp"
#include "base/defer.hpp"
#include "base/exception.hpp"
#include "base/io-engine.hpp"
#include "base/logger.hpp"
#include "base/objectlock.hpp"
#include "base/timer.hpp"
#include "base/tlsstream.hpp"
#include "base/utility.hpp"
#include <memory>
#include <stdexcept>
#include <boost/asio/spawn.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/system/system_error.hpp>
#include <boost/thread/once.hpp>

using namespace icinga;

auto const l_ServerHeader ("Icinga/" + Application::GetAppVersion());

HttpServerConnection::HttpServerConnection(const String& identity, bool authenticated, const std::shared_ptr<AsioTlsStream>& stream)
	: m_Stream(stream)
{
	if (authenticated) {
		m_ApiUser = ApiUser::GetByClientCN(identity);
	}

	{
		std::ostringstream address;
		auto endpoint (stream->lowest_layer().remote_endpoint());

		address << '[' << endpoint.address() << "]:" << endpoint.port();

		m_PeerAddress = address.str();
	}
}

void HttpServerConnection::Start()
{
	namespace asio = boost::asio;

	asio::spawn(IoEngine::Get().GetIoService(), [this](asio::yield_context yc) { ProcessMessages(yc); });
}

static inline
bool EnsureValidHeaders(
	AsioTlsStream& stream,
	boost::beast::flat_buffer& buf,
	boost::beast::http::parser<true, boost::beast::http::string_body>& parser,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;

	try {
		try {
			http::async_read_header(stream, buf, parser, yc);
		} catch (const boost::system::system_error& ex) {
			/**
			 * Unfortunately there's no way to tell an HTTP protocol error
			 * from an error on a lower layer:
			 *
			 * <https://github.com/boostorg/beast/issues/643>
			 */
			throw std::invalid_argument(ex.what());
		}

		switch (parser.get().version()) {
		case 10:
		case 11:
			break;
		default:
			throw std::invalid_argument("Unsupported HTTP version");
		}
	} catch (const std::invalid_argument& ex) {
		response.result(http::status::bad_request);
		response.set(http::field::content_type, "text/html");
		response.body() = String("<h1>Bad Request</h1><p><pre>") + ex.what() + "</pre></p>";
		response.set(http::field::content_length, response.body().size());
		response.set(http::field::connection, "close");

		http::async_write(stream, response, yc);
		stream.async_flush(yc);

		return false;
	}

	return true;
}

static inline
void HandleExpect100(
	AsioTlsStream& stream,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;

	if (request[http::field::expect] == "100-continue") {
		http::response<http::string_body> response;

		response.result(http::status::continue_);

		http::async_write(stream, response, yc);
		stream.async_flush(yc);
	}
}

static inline
bool HandleAccessControl(
	AsioTlsStream& stream,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;

	auto listener (ApiListener::GetInstance());

	if (listener) {
		auto headerAllowOrigin (listener->GetAccessControlAllowOrigin());

		if (headerAllowOrigin) {
			CpuBoundWork allowOriginHeader (yc);

			auto allowedOrigins (headerAllowOrigin->ToSet<String>());

			if (!allowedOrigins.empty()) {
				auto& origin (request[http::field::origin]);

				if (allowedOrigins.find(origin.to_string()) != allowedOrigins.end()) {
					response.set(http::field::access_control_allow_origin, origin);
				}

				allowOriginHeader.Done();

				response.set(http::field::access_control_allow_credentials, "true");

				if (request.method() == http::verb::options && !request[http::field::access_control_request_method].empty()) {
					response.result(http::status::ok);
					response.set(http::field::access_control_allow_methods, "GET, POST, PUT, DELETE");
					response.set(http::field::access_control_allow_headers, "Authorization, X-HTTP-Method-Override");
					response.body() = "Preflight OK";
					response.set(http::field::content_length, response.body().size());
					response.set(http::field::connection, "close");

					http::async_write(stream, response, yc);
					stream.async_flush(yc);

					return false;
				}
			}
		}
	}

	return true;
}

static inline
bool EnsureAcceptHeader(
	AsioTlsStream& stream,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;

	if (request.method() == http::verb::get && request[http::field::accept] != "application/json") {
		response.result(http::status::bad_request);
		response.set(http::field::content_type, "text/html");
		response.body() = "<h1>Accept header is missing or not set to 'application/json'.</h1>";
		response.set(http::field::content_length, response.body().size());
		response.set(http::field::connection, "close");

		http::async_write(stream, response, yc);
		stream.async_flush(yc);

		return false;
	}

	return true;
}

static inline
bool EnsureAuthenticatedUser(
	AsioTlsStream& stream,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	ApiUser::Ptr& authenticatedUser,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;

	if (!authenticatedUser) {
		Log(LogWarning, "HttpServerConnection")
			<< "Unauthorized request: " << request.method_string() << ' ' << request.target();

		response.result(http::status::unauthorized);
		response.set(http::field::www_authenticate, "Basic realm=\"Icinga 2\"");
		response.set(http::field::connection, "close");

		if (request[http::field::accept] == "application/json") {
			HttpUtility::SendJsonBody(response, nullptr, new Dictionary({
				{ "error", 401 },
				{ "status", "Unauthorized. Please check your user credentials." }
			}));
		} else {
			response.set(http::field::content_type, "text/html");
			response.body() = "<h1>Unauthorized. Please check your user credentials.</h1>";
			response.set(http::field::content_length, response.body().size());
		}

		http::async_write(stream, response, yc);
		stream.async_flush(yc);

		return false;
	}

	return true;
}

static inline
bool EnsureValidBody(
	AsioTlsStream& stream,
	boost::beast::flat_buffer& buf,
	boost::beast::http::parser<true, boost::beast::http::string_body>& parser,
	ApiUser::Ptr& authenticatedUser,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;

	{
		size_t maxSize = 1024 * 1024;
		Array::Ptr permissions = authenticatedUser->GetPermissions();

		if (permissions) {
			CpuBoundWork evalPermissions (yc);

			ObjectLock olock(permissions);

			for (const Value& permissionInfo : permissions) {
				String permission;

				if (permissionInfo.IsObjectType<Dictionary>()) {
					permission = static_cast<Dictionary::Ptr>(permissionInfo)->Get("permission");
				} else {
					permission = permissionInfo;
				}

				static std::vector<std::pair<String, size_t>> specialContentLengthLimits {
					 { "config/modify", 512 * 1024 * 1024 }
				};

				for (const auto& limitInfo : specialContentLengthLimits) {
					if (limitInfo.second <= maxSize) {
						continue;
					}

					if (Utility::Match(permission, limitInfo.first)) {
						maxSize = limitInfo.second;
					}
				}
			}
		}

		parser.body_limit(maxSize);
	}

	try {
		http::async_read(stream, buf, parser, yc);
	} catch (const boost::system::system_error& ex) {
		/**
		 * Unfortunately there's no way to tell an HTTP protocol error
		 * from an error on a lower layer:
		 *
		 * <https://github.com/boostorg/beast/issues/643>
		 */

		response.result(http::status::bad_request);
		response.set(http::field::content_type, "text/html");
		response.body() = String("<h1>Bad Request</h1><p><pre>") + ex.what() + "</pre></p>";
		response.set(http::field::content_length, response.body().size());
		response.set(http::field::connection, "close");

		http::async_write(stream, response, yc);
		stream.async_flush(yc);

		return false;
	}

	return true;
}

static inline
void ProcessRequest(
	AsioTlsStream& stream,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	ApiUser::Ptr& authenticatedUser,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;

	HttpUtility::SendJsonError(response, nullptr, 503, "Unhandled exception" , "");

	http::async_write(stream, response, yc);
	stream.async_flush(yc);
}

void HttpServerConnection::ProcessMessages(boost::asio::yield_context yc)
{
	namespace beast = boost::beast;
	namespace http = beast::http;

	Defer removeHttpClient ([this, &yc]() {
		auto listener (ApiListener::GetInstance());

		if (listener) {
			CpuBoundWork removeHttpClient (yc);

			listener->RemoveHttpClient(this);
		}
	});

	Defer shutdown ([this, &yc]() {
		try {
			m_Stream->next_layer().async_shutdown(yc);
		} catch (...) {
			// https://stackoverflow.com/questions/130117/throwing-exceptions-out-of-a-destructor
		}
	});

	try {
		beast::flat_buffer buf;

		for (;;) {
			http::parser<true, http::string_body> parser;
			http::response<http::string_body> response;

			parser.header_limit(1024 * 1024);

			response.set(http::field::server, l_ServerHeader);

			if (!EnsureValidHeaders(*m_Stream, buf, parser, response, yc)) {
				break;
			}

			auto& request (parser.get());

			{
				auto method (http::string_to_verb(request["X-Http-Method-Override"]));

				if (method != http::verb::unknown) {
					request.method(method);
				}
			}

			HandleExpect100(*m_Stream, request, yc);

			auto authenticatedUser (m_ApiUser);

			if (!authenticatedUser) {
				CpuBoundWork fetchingAuthenticatedUser (yc);

				authenticatedUser = ApiUser::GetByAuthHeader(request[http::field::authorization].to_string());
			}

			Log(LogInformation, "HttpServerConnection")
				<< "Request: " << request.method_string() << ' ' << request.target()
				<< " (from " << m_PeerAddress
				<< "), user: " << (authenticatedUser ? authenticatedUser->GetName() : "<unauthenticated>") << ')';

			if (!HandleAccessControl(*m_Stream, request, response, yc)) {
				break;
			}

			if (!EnsureAcceptHeader(*m_Stream, request, response, yc)) {
				break;
			}

			if (!EnsureAuthenticatedUser(*m_Stream, request, authenticatedUser, response, yc)) {
				break;
			}

			if (!EnsureValidBody(*m_Stream, buf, parser, authenticatedUser, response, yc)) {
				break;
			}

			ProcessRequest(*m_Stream, request, authenticatedUser, response, yc);

			if (request.version() != 11 || request[http::field::connection] == "close") {
				break;
			}
		}
	} catch (const std::exception& ex) {
		Log(LogCritical, "HttpServerConnection")
			<< "Unhandled exception while processing HTTP request: " << DiagnosticInformation(ex);
	}
}
