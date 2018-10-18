/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#include "remote/apiclient.hpp"
#include "base/base64.hpp"
#include "base/json.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"

using namespace icinga;

ApiClient::ApiClient(const String& host, const String& port,
	String user, String password)
	: m_Connection(new HttpClientConnection(host, port, true)), m_User(std::move(user)), m_Password(std::move(password))
{
	m_Connection->Start();
}

void ApiClient::ExecuteScript(const String& session, const String& command, bool sandboxed,
	const ExecuteScriptCompletionCallback& callback) const
{
	Url::Ptr url = new Url();
	url->SetScheme("https");
	url->SetHost(m_Connection->GetHost());
	url->SetPort(m_Connection->GetPort());
	url->SetPath({ "v1", "console", "execute-script" });

	std::map<String, std::vector<String> > params;
	params["session"].push_back(session);
	params["command"].push_back(command);
	params["sandboxed"].emplace_back(sandboxed ? "1" : "0");
	url->SetQuery(params);

	try {
		std::shared_ptr<HttpRequest> req = m_Connection->NewRequest();
		req->RequestMethod = "POST";
		req->RequestUrl = url;
		req->AddHeader("Authorization", "Basic " + Base64::Encode(m_User + ":" + m_Password));
		req->AddHeader("Accept", "application/json");
		m_Connection->SubmitRequest(req, std::bind(ExecuteScriptHttpCompletionCallback, _1, _2, callback));
	} catch (const std::exception&) {
		callback(boost::current_exception(), Empty);
	}
}

void ApiClient::ExecuteScriptHttpCompletionCallback(HttpRequest& request,
	HttpResponse& response, const ExecuteScriptCompletionCallback& callback)
{
	Dictionary::Ptr result;

	String body;
	char buffer[1024];
	size_t count;

	while ((count = response.ReadBody(buffer, sizeof(buffer))) > 0)
		body += String(buffer, buffer + count);

	try {
		if (response.StatusCode < 200 || response.StatusCode > 299) {
			std::string message = "HTTP request failed; Code: " + Convert::ToString(response.StatusCode) + "; Body: " + body;

			BOOST_THROW_EXCEPTION(ScriptError(message));
		}

		result = JsonDecode(body);

		Array::Ptr results = result->Get("results");
		Value result;
		String errorMessage = "Unexpected result from API.";

		if (results && results->GetLength() > 0) {
			Dictionary::Ptr resultInfo = results->Get(0);
			errorMessage = resultInfo->Get("status");

			if (resultInfo->Get("code") >= 200 && resultInfo->Get("code") <= 299) {
				result = resultInfo->Get("result");
			} else {
				DebugInfo di;
				Dictionary::Ptr debugInfo = resultInfo->Get("debug_info");
				if (debugInfo) {
					di.Path = debugInfo->Get("path");
					di.FirstLine = debugInfo->Get("first_line");
					di.FirstColumn = debugInfo->Get("first_column");
					di.LastLine = debugInfo->Get("last_line");
					di.LastColumn = debugInfo->Get("last_column");
				}
				bool incompleteExpression = resultInfo->Get("incomplete_expression");
				BOOST_THROW_EXCEPTION(ScriptError(errorMessage, di, incompleteExpression));
			}
		}

		callback(boost::exception_ptr(), result);
	} catch (const std::exception&) {
		callback(boost::current_exception(), Empty);
	}
}

void ApiClient::AutocompleteScript(const String& session, const String& command, bool sandboxed,
	const AutocompleteScriptCompletionCallback& callback) const
{
	Url::Ptr url = new Url();
	url->SetScheme("https");
	url->SetHost(m_Connection->GetHost());
	url->SetPort(m_Connection->GetPort());
	url->SetPath({ "v1", "console", "auto-complete-script" });

	std::map<String, std::vector<String> > params;
	params["session"].push_back(session);
	params["command"].push_back(command);
	params["sandboxed"].emplace_back(sandboxed ? "1" : "0");
	url->SetQuery(params);

	try {
		std::shared_ptr<HttpRequest> req = m_Connection->NewRequest();
		req->RequestMethod = "POST";
		req->RequestUrl = url;
		req->AddHeader("Authorization", "Basic " + Base64::Encode(m_User + ":" + m_Password));
		req->AddHeader("Accept", "application/json");
		m_Connection->SubmitRequest(req, std::bind(AutocompleteScriptHttpCompletionCallback, _1, _2, callback));
	} catch (const std::exception&) {
		callback(boost::current_exception(), nullptr);
	}
}

void ApiClient::AutocompleteScriptHttpCompletionCallback(HttpRequest& request,
	HttpResponse& response, const AutocompleteScriptCompletionCallback& callback)
{
	Dictionary::Ptr result;

	String body;
	char buffer[1024];
	size_t count;

	while ((count = response.ReadBody(buffer, sizeof(buffer))) > 0)
		body += String(buffer, buffer + count);

	try {
		if (response.StatusCode < 200 || response.StatusCode > 299) {
			std::string message = "HTTP request failed; Code: " + Convert::ToString(response.StatusCode) + "; Body: " + body;

			BOOST_THROW_EXCEPTION(ScriptError(message));
		}

		result = JsonDecode(body);

		Array::Ptr results = result->Get("results");
		Array::Ptr suggestions;
		String errorMessage = "Unexpected result from API.";

		if (results && results->GetLength() > 0) {
			Dictionary::Ptr resultInfo = results->Get(0);
			errorMessage = resultInfo->Get("status");

			if (resultInfo->Get("code") >= 200 && resultInfo->Get("code") <= 299)
				suggestions = resultInfo->Get("suggestions");
			else
				BOOST_THROW_EXCEPTION(ScriptError(errorMessage));
		}

		callback(boost::exception_ptr(), suggestions);
	} catch (const std::exception&) {
		callback(boost::current_exception(), nullptr);
	}
}
