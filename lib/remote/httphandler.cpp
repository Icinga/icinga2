/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/logger.hpp"
#include "remote/httphandler.hpp"
#include "remote/httputility.hpp"
#include "base/singleton.hpp"
#include "base/exception.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/beast/http.hpp>

using namespace icinga;

Dictionary::Ptr HttpHandler::m_UrlTree;

void HttpHandler::Register(const Url::Ptr& url, const HttpHandler::Ptr& handler)
{
	if (!m_UrlTree)
		m_UrlTree = new Dictionary();

	Dictionary::Ptr node = m_UrlTree;

	for (const String& elem : url->GetPath()) {
		Dictionary::Ptr children = node->Get("children");

		if (!children) {
			children = new Dictionary();
			node->Set("children", children);
		}

		Dictionary::Ptr sub_node = children->Get(elem);
		if (!sub_node) {
			sub_node = new Dictionary();
			children->Set(elem, sub_node);
		}

		node = sub_node;
	}

	Array::Ptr handlers = node->Get("handlers");

	if (!handlers) {
		handlers = new Array();
		node->Set("handlers", handlers);
	}

	handlers->Add(handler);
}

void HttpHandler::ProcessRequest(
	AsioTlsStream& stream,
	const ApiUser::Ptr& user,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	Url::Ptr& url,
	Dictionary::Ptr& params,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	boost::asio::yield_context& yc,
	HttpServerConnection& server
)
{
	Dictionary::Ptr node = m_UrlTree;
	std::vector<HttpHandler::Ptr> handlers;

	url = new Url(std::string(request.target()));
	auto& path (url->GetPath());

	for (std::vector<String>::size_type i = 0; i <= path.size(); i++) {
		Array::Ptr current_handlers = node->Get("handlers");

		if (current_handlers) {
			ObjectLock olock(current_handlers);
			for (const HttpHandler::Ptr& current_handler : current_handlers) {
				handlers.push_back(current_handler);
			}
		}

		Dictionary::Ptr children = node->Get("children");

		if (!children) {
			node.reset();
			break;
		}

		if (i == path.size())
			break;

		node = children->Get(path[i]);

		if (!node)
			break;
	}

	std::reverse(handlers.begin(), handlers.end());

	try {
		params = HttpUtility::FetchRequestParameters(url, request.body());
	} catch (const std::exception& ex) {
		HttpUtility::SendJsonError(response, params, 400, "Invalid request body: " + DiagnosticInformation(ex, false));
		return;
	}

	bool processed = false;

	/*
	 * HandleRequest may throw a permission exception.
	 * DO NOT return a specific permission error. This
	 * allows attackers to guess from words which objects
	 * do exist.
	 */
	try {
		for (const HttpHandler::Ptr& handler : handlers) {
			if (handler->HandleRequest(stream, user, request, url, response, params, yc, server)) {
				processed = true;
				break;
			}
		}
	} catch (const std::exception& ex) {
		Log(LogWarning, "HttpServerConnection")
			<< "Error while processing HTTP request: " << ex.what();

		processed = false;
	}

	if (!processed) {
		HttpUtility::SendJsonError(response, params, 404, "The requested path '" + boost::algorithm::join(path, "/") +
			"' could not be found or the request method is not valid for this path.");
		return;
	}
}
