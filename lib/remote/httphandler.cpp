/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "remote/httphandler.hpp"
#include "remote/httputility.hpp"
#include "base/singleton.hpp"
#include "base/exception.hpp"
#include <boost/algorithm/string/join.hpp>

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

void HttpHandler::ProcessRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	Dictionary::Ptr node = m_UrlTree;
	std::vector<HttpHandler::Ptr> handlers;
	const std::vector<String>& path = request.RequestUrl->GetPath();

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

	Dictionary::Ptr params;

	try {
		params = HttpUtility::FetchRequestParameters(request);
	} catch (const std::exception& ex) {
		HttpUtility::SendJsonError(response, 400, "Invalid request body: " + DiagnosticInformation(ex, false));
		return;
	}

	bool processed = false;
	for (const HttpHandler::Ptr& handler : handlers) {
		if (handler->HandleRequest(user, request, response, params)) {
			processed = true;
			break;
		}
	}

	if (!processed) {
		String path = boost::algorithm::join(request.RequestUrl->GetPath(), "/");
		HttpUtility::SendJsonError(response, 404, "The requested path '" + path +
				"' could not be found or the request method is not valid for this path.");
		return;
	}
}

