// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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

		node = std::move(sub_node);
	}

	Array::Ptr handlers = node->Get("handlers");

	if (!handlers) {
		handlers = new Array();
		node->Set("handlers", handlers);
	}

	handlers->Add(handler);
}

void HttpHandler::ProcessRequest(
	const WaitGroup::Ptr& waitGroup,
	HttpApiRequest& request,
	HttpApiResponse& response,
	boost::asio::yield_context& yc
)
{
	Dictionary::Ptr node = m_UrlTree;
	std::vector<HttpHandler::Ptr> handlers;

	request.DecodeUrl();
	auto& path (request.Url()->GetPath());

	for (std::vector<String>::size_type i = 0; i <= path.size(); i++) {
		Array::Ptr current_handlers = node->Get("handlers");

		if (current_handlers) {
			ObjectLock olock(current_handlers);
			for (HttpHandler::Ptr current_handler : current_handlers) {
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
		request.DecodeParams();
	} catch (const std::exception& ex) {
		HttpUtility::SendJsonError(response, request.Params(), 400, "Invalid request body: " + DiagnosticInformation(ex, false));
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
			if (handler->HandleRequest(waitGroup, request, response, yc)) {
				processed = true;
				break;
			}
		}
	} catch (const std::exception& ex) {
		// Errors related to writing the response should be handled in HttpServerConnection.
		if (dynamic_cast<const boost::system::system_error*>(&ex)) {
			throw;
		}

		/* This means we can't send an error response because the exception was thrown
		 * in the middle of a streaming response. We can't send any error response, so the
		 * only thing we can do is propagate it up.
		 */
		if (response.HasSerializationStarted()) {
			throw;
		}

		Log(LogWarning, "HttpServerConnection")
			<< "Error while processing HTTP request: " << ex.what();

		processed = false;
	}

	if (!processed) {
		HttpUtility::SendJsonError(response, request.Params(), 404, "The requested path '" + boost::algorithm::join(path, "/") +
			"' could not be found or the request method is not valid for this path.");
		return;
	}
}
