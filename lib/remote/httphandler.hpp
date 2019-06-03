/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H

#include "remote/i2-remote.hpp"
#include "remote/url.hpp"
#include "remote/httpserverconnection.hpp"
#include "remote/apiuser.hpp"
#include "base/registry.hpp"
#include "base/tlsstream.hpp"
#include <vector>
#include <boost/asio/spawn.hpp>
#include <boost/beast/http.hpp>

namespace icinga
{

/**
 * HTTP handler.
 *
 * @ingroup remote
 */
class HttpHandler : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(HttpHandler);

	virtual bool HandleRequest(
		AsioTlsStream& stream,
		const ApiUser::Ptr& user,
		boost::beast::http::request<boost::beast::http::string_body>& request,
		const Url::Ptr& url,
		boost::beast::http::response<boost::beast::http::string_body>& response,
		const Dictionary::Ptr& params,
		boost::asio::yield_context& yc,
		HttpServerConnection& server
	) = 0;

	static void Register(const Url::Ptr& url, const HttpHandler::Ptr& handler);
	static void ProcessRequest(
		AsioTlsStream& stream,
		const ApiUser::Ptr& user,
		boost::beast::http::request<boost::beast::http::string_body>& request,
		boost::beast::http::response<boost::beast::http::string_body>& response,
		boost::asio::yield_context& yc,
		HttpServerConnection& server
	);

private:
	static Dictionary::Ptr m_UrlTree;
};

/**
 * Helper class for registering HTTP handlers.
 *
 * @ingroup remote
 */
class RegisterHttpHandler
{
public:
	RegisterHttpHandler(const String& url, const HttpHandler& function);
};

#define REGISTER_URLHANDLER(url, klass) \
	INITIALIZE_ONCE([]() { \
		Url::Ptr uurl = new Url(url); \
		HttpHandler::Ptr handler = new klass(); \
		HttpHandler::Register(uurl, handler); \
	})

}

#endif /* HTTPHANDLER_H */
