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
#include <limits>
#include <memory>
#include <stdexcept>
#include <boost/asio/error.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/thread/once.hpp>

using namespace icinga;

auto const l_ServerHeader ("Icinga/" + Application::GetAppVersion());

HttpServerConnection::HttpServerConnection(const WaitGroup::Ptr& waitGroup, const String& identity, bool authenticated, const Shared<AsioTlsStream>::Ptr& stream)
	: HttpServerConnection(waitGroup, identity, authenticated, stream, IoEngine::Get().GetIoContext())
{
}

HttpServerConnection::HttpServerConnection(const WaitGroup::Ptr& waitGroup, const String& identity, bool authenticated, const Shared<AsioTlsStream>::Ptr& stream, boost::asio::io_context& io)
	: m_WaitGroup(waitGroup), m_Stream(stream), m_IoStrand(io), m_ShuttingDown(false), m_ConnectionReusable(true), m_CheckLivenessTimer(io)
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

	HttpServerConnection::Ptr keepAlive (this);

	IoEngine::SpawnCoroutine(m_IoStrand, [this, keepAlive](asio::yield_context yc) { ProcessMessages(yc); });
	IoEngine::SpawnCoroutine(m_IoStrand, [this, keepAlive](asio::yield_context yc) { CheckLiveness(yc); });
}

/**
 * Tries to asynchronously shut down the SSL stream and underlying socket.
 *
 * It is important to note that this method should only be called from within a coroutine that uses `m_IoStrand`.
 *
 * @param yc boost::asio::yield_context The coroutine yield context which you are calling this method from.
 */
void HttpServerConnection::Disconnect(boost::asio::yield_context yc)
{
	namespace asio = boost::asio;

	if (m_ShuttingDown) {
		return;
	}

	m_ShuttingDown = true;

	Log(LogInformation, "HttpServerConnection")
		<< "HTTP client disconnected (from " << m_PeerAddress << ")";

	m_CheckLivenessTimer.cancel();

	m_Stream->GracefulDisconnect(m_IoStrand, yc);

	auto listener (ApiListener::GetInstance());

	if (listener) {
		listener->RemoveHttpClient(this);
	}
}

/**
 * Starts a coroutine that continually reads from the stream to detect a disconnect from the client.
 *
 * This can be accessed inside an @c HttpHandler via the HttpResponse::StartStreaming() method by
 * passing true as the argument, expressing that disconnect detection is desired.
 */
void HttpServerConnection::StartDetectClientSideShutdown()
{
	namespace asio = boost::asio;

	m_ConnectionReusable = false;

	HttpServerConnection::Ptr keepAlive (this);

	/* Technically it would be possible to detect disconnects on the TCP-side by setting the
	 * socket to non-blocking and then performing a read directly on the socket with the message_peek
	 * flag. As the TCP FIN message will put the connection into a CLOSE_WAIT even if the kernel
	 * buffer is full, this would technically be reliable way of detecting a shutdown and free
	 * of side-effects.
	 *
	 * However, for detecting the close_notify on the SSL/TLS-side, an async_fill() would be necessary
	 * when the check on the TCP level above returns that there are readable bytes (and no FIN/eof).
	 * If this async_fill() then buffers more application data and not an immediate eof, we could
	 * attempt to read another message before disconnecting.
	 *
	 * This could either be done at the level of the handlers, via the @c HttpResponse class, or
	 * generally as a separate coroutine here in @c HttpServerConnection, both (mostly) side-effect
	 * free and without affecting the state of the connection.
	 *
	 * However, due to the complexity of this approach, involving several asio operations, message
	 * flags, synchronous and asynchronous operations in blocking and non-blocking mode, ioctl cmds,
	 * etc., it was decided to stick with a simple reading loop, started conditionally on request by
	 * the handler.
	 */
	IoEngine::SpawnCoroutine(m_IoStrand, [this, keepAlive](asio::yield_context yc) {
		if (!m_ShuttingDown) {
			char buf[128];
			asio::mutable_buffer readBuf (buf, 128);
			boost::system::error_code ec;

			do {
				m_Stream->async_read_some(readBuf, yc[ec]);
			} while (!ec);

			Disconnect(yc);
		}
	});
}

bool HttpServerConnection::Disconnected()
{
	return m_ShuttingDown;
}

void HttpServerConnection::SetLivenessTimeout(std::chrono::milliseconds timeout)
{
	m_LivenessTimeout = timeout;
}

static inline
bool EnsureValidHeaders(
	boost::beast::flat_buffer& buf,
	HttpRequest& request,
	HttpResponse& response,
	bool& shuttingDown,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;

	if (shuttingDown)
		return false;

	bool httpError = false;
	String errorMsg;

	boost::system::error_code ec;

	request.ParseHeader(buf, yc[ec]);

	if (ec) {
		if (ec == boost::asio::error::operation_aborted)
			return false;

		errorMsg = ec.message();
		httpError = true;
	} else {
		switch (request.version()) {
		case 10:
		case 11:
			break;
		default:
			errorMsg = "Unsupported HTTP version";
		}
	}

	if (!errorMsg.IsEmpty() || httpError) {
		response.result(http::status::bad_request);

		if (!httpError && request[http::field::accept] == "application/json") {
			HttpUtility::SendJsonError(response, nullptr, 400, "Bad Request: " + errorMsg);
		} else {
			response.set(http::field::content_type, "text/html");
			response.body() << "<h1>Bad Request</h1><p><pre>" << errorMsg << "</pre></p>";
		}

		response.set(http::field::connection, "close");

		response.Flush(yc);

		return false;
	}

	return true;
}

static inline
void HandleExpect100(
	const Shared<AsioTlsStream>::Ptr& stream,
	const HttpRequest& request,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;

	if (request[http::field::expect] == "100-continue") {
		HttpResponse response{stream};
		response.result(http::status::continue_);
		response.Flush(yc);
	}
}

static inline
bool HandleAccessControl(
	const HttpRequest& request,
	HttpResponse& response,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;

	auto listener (ApiListener::GetInstance());

	if (listener) {
		auto headerAllowOrigin (listener->GetAccessControlAllowOrigin());

		if (headerAllowOrigin) {
			auto allowedOrigins (headerAllowOrigin->ToSet<String>());

			if (!allowedOrigins.empty()) {
				auto& origin (request[http::field::origin]);

				if (allowedOrigins.find(std::string(origin)) != allowedOrigins.end()) {
					response.set(http::field::access_control_allow_origin, origin);
				}

				response.set(http::field::access_control_allow_credentials, "true");

				if (request.method() == http::verb::options && !request[http::field::access_control_request_method].empty()) {
					response.result(http::status::ok);
					response.set(http::field::access_control_allow_methods, "GET, POST, PUT, DELETE");
					response.set(http::field::access_control_allow_headers, "Authorization, Content-Type, X-HTTP-Method-Override");
					response.body() << "Preflight OK";
					response.set(http::field::connection, "close");

					response.Flush(yc);

					return false;
				}
			}
		}
	}

	return true;
}

static inline
bool EnsureAcceptHeader(
	const HttpRequest& request,
	HttpResponse& response,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;

	if (request.method() != http::verb::get && request[http::field::accept] != "application/json") {
		response.result(http::status::bad_request);
		response.set(http::field::content_type, "text/html");
		response.body() << "<h1>Accept header is missing or not set to 'application/json'.</h1>";
		response.set(http::field::connection, "close");

		response.Flush(yc);

		return false;
	}

	return true;
}

static inline
bool EnsureAuthenticatedUser(
	const HttpRequest& request,
	HttpResponse& response,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;

	if (!request.User()) {
		Log(LogWarning, "HttpServerConnection")
			<< "Unauthorized request: " << request.method_string() << ' ' << request.target();

		response.result(http::status::unauthorized);
		response.set(http::field::www_authenticate, "Basic realm=\"Icinga 2\"");
		response.set(http::field::connection, "close");

		if (request[http::field::accept] == "application/json") {
			HttpUtility::SendJsonError(response, nullptr, 401, "Unauthorized. Please check your user credentials.");
		} else {
			response.set(http::field::content_type, "text/html");
			response.body() << "<h1>Unauthorized. Please check your user credentials.</h1>";
		}

		response.Flush(yc);

		return false;
	}

	return true;
}

static inline
bool EnsureValidBody(
	boost::beast::flat_buffer& buf,
	HttpRequest& request,
	HttpResponse& response,
	bool& shuttingDown,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;

	{
		size_t maxSize = 1024 * 1024;
		Array::Ptr permissions = request.User()->GetPermissions();

		if (permissions) {
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

		request.Parser().body_limit(maxSize);
	}

	if (shuttingDown)
		return false;

	boost::system::error_code ec;

	request.ParseBody(buf, yc[ec]);

	if (ec) {
		if (ec == boost::asio::error::operation_aborted)
			return false;

		/**
		 * Unfortunately there's no way to tell an HTTP protocol error
		 * from an error on a lower layer:
		 *
		 * <https://github.com/boostorg/beast/issues/643>
		 */

		response.result(http::status::bad_request);

		if (request[http::field::accept] == "application/json") {
			HttpUtility::SendJsonError(response, nullptr, 400, "Bad Request: " + ec.message());
		} else {
			response.set(http::field::content_type, "text/html");
			response.body() << "<h1>Bad Request</h1><p><pre>" << ec.message() << "</pre></p>";
		}

		response.set(http::field::connection, "close");

		response.Flush(yc);

		return false;
	}

	return true;
}

static inline
void ProcessRequest(
	HttpRequest& request,
	HttpResponse& response,
	const WaitGroup::Ptr& waitGroup,
	boost::asio::yield_context& yc
)
{
	try {
		/* Place some restrictions on the total number of HTTP requests handled concurrently to prevent HTTP requests
		 * from hogging the entire coroutine thread pool by running too many requests handlers at once that don't
		 * regularly yield, starving other coroutines.
		 *
		 * We need to consider two types of handlers here:
		 *
		 * 1. Those performing a more or less expensive operation and then returning the whole response at once.
		 *    Not too many of such handlers should run concurrently.
		 * 2. Those already streaming the response while they are running, for example using chunked transfer encoding.
		 *    For these, we assume that they will frequently yield to other coroutines, in particular when writing parts
		 *    of the response to the client, or in case of EventsHandler also when waiting for new events.
		 *
		 * The following approach handles both of this automatically: we acquire one of a limited number of slots for
		 * each request and release it automatically the first time anything (either the full response after the handler
		 * finished or the first chunk from within the handler) is written using the response object. This means that
		 * we don't have to handle acquiring or releasing that slot inside individual handlers.
		 *
		 * Overall, this is more or less a safeguard preventing long-running HTTP handlers from taking down the entire
		 * Icinga 2 process by blocking the execution of JSON-RPC message handlers. In general, (new) HTTP handlers
		 * shouldn't rely on this behavior but rather ensure that they are quick or at least yield regularly.
		 */
		if (!response.TryAcquireSlowSlot()) {
			HttpUtility::SendJsonError(response, request.Params(), 503,
				"Too many requests already in progress, please try again later.");
			response.Flush(yc);
			return;
		}

		HttpHandler::ProcessRequest(waitGroup, request, response, yc);
		response.body().Finish();
	} catch (const std::exception& ex) {
		/* Since we don't know the state the stream is in, we can't send an error response and
		 * have to just cause a disconnect here.
		 */
		if (response.HasSerializationStarted()) {
			throw;
		}

		HttpUtility::SendJsonError(response, request.Params(), 500, "Unhandled exception", DiagnosticInformation(ex));
	}

	response.Flush(yc);
}

void HttpServerConnection::ProcessMessages(boost::asio::yield_context yc)
{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace ch = std::chrono;

	try {
		/* Do not reset the buffer in the state machine.
		 * EnsureValidHeaders already reads from the stream into the buffer,
		 * EnsureValidBody continues. ProcessRequest() actually handles the request
		 * and needs the full buffer.
		 */
		beast::flat_buffer buf;

		while (m_WaitGroup->IsLockable()) {
			m_Seen = ch::steady_clock::now();

			HttpRequest request(m_Stream);
			HttpResponse response(m_Stream, this);

			request.Parser().header_limit(1024 * 1024);
			request.Parser().body_limit(-1);

			response.set(http::field::server, l_ServerHeader);
			if (auto listener (ApiListener::GetInstance()); listener) {
				if (Dictionary::Ptr headers = listener->GetHttpResponseHeaders(); headers) {
					ObjectLock lock(headers);
					for (auto& [header, value] : headers) {
						if (value.IsString()) {
							response.set(header, value.Get<String>());
						}
					}
				}
			}

			if (!EnsureValidHeaders(buf, request, response, m_ShuttingDown, yc)) {
				break;
			}

			m_Seen = ch::steady_clock::now();
			auto start (ch::steady_clock::now());

			{
				auto method (http::string_to_verb(request["X-Http-Method-Override"]));

				if (method != http::verb::unknown) {
					request.method(method);
				}
			}

			HandleExpect100(m_Stream, request, yc);

			if (m_ApiUser) {
				request.User(m_ApiUser);
			} else {
				request.User(ApiUser::GetByAuthHeader(std::string(request[http::field::authorization])));
			}

			Log logMsg (LogInformation, "HttpServerConnection");

			logMsg << "Request " << request.method_string() << ' ' << request.target()
				<< " (from " << m_PeerAddress
				<< ", user: " << (request.User() ? request.User()->GetName() : "<unauthenticated>")
				<< ", agent: " << request[http::field::user_agent]; //operator[] - Returns the value for a field, or "" if it does not exist.

			Defer addRespCode ([&response, start, &logMsg]() {
				logMsg << ", status: " << response.result() << ")" << " took total "
					<< ch::duration_cast<ch::milliseconds>(ch::steady_clock::now() - start).count() << "ms.";
			});

			if (!HandleAccessControl(request, response, yc)) {
				break;
			}

			if (!EnsureAcceptHeader(request, response, yc)) {
				break;
			}

			if (!EnsureAuthenticatedUser(request, response, yc)) {
				break;
			}

			if (!EnsureValidBody(buf, request, response, m_ShuttingDown, yc)) {
				break;
			}

			m_Seen = ch::steady_clock::time_point::max();

			ProcessRequest(request, response, m_WaitGroup, yc);

			if (!request.keep_alive() || !m_ConnectionReusable) {
				break;
			}
		}
	} catch (const std::exception& ex) {
		if (!m_ShuttingDown) {
			Log(LogWarning, "HttpServerConnection")
				<< "Exception while processing HTTP request from " << m_PeerAddress << ": " << ex.what();
		}
	}

	Disconnect(yc);
}

void HttpServerConnection::CheckLiveness(boost::asio::yield_context yc)
{
	boost::system::error_code ec;

	for (;;) {
		// Wait for the half of the liveness timeout to give the connection some leeway to do other work.
		// But never wait longer than 5 seconds to ensure timely shutdowns.
		auto sleepTime = std::min(5000ms, m_LivenessTimeout / 2);
		m_CheckLivenessTimer.expires_from_now(boost::posix_time::milliseconds(sleepTime.count()));
		m_CheckLivenessTimer.async_wait(yc[ec]);

		if (m_ShuttingDown) {
			break;
		}

		if (m_LivenessTimeout < std::chrono::steady_clock::now() - m_Seen) {
			Log(LogInformation, "HttpServerConnection")
				<<  "No messages for HTTP connection have been received in the last "
				<< std::chrono::duration_cast<std::chrono::seconds>(m_LivenessTimeout).count()
				<< " seconds, disconnecting (from " << m_PeerAddress << ").";

			Disconnect(yc);
			break;
		}
	}
}
