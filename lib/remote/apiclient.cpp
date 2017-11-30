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

#include "remote/apiclient.hpp"
#include "base/base64.hpp"
#include "base/json.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"

using namespace icinga;

ApiClient::ApiClient(const String& host, const String& port,
    const String& user, const String& password)
    : m_Connection(new HttpClientConnection(host, port, true)), m_User(user), m_Password(password)
{
	m_Connection->Start();
}

void ApiClient::GetTypes(const TypesCompletionCallback& callback) const
{
	Url::Ptr url = new Url();
	url->SetScheme("https");
	url->SetHost(m_Connection->GetHost());
	url->SetPort(m_Connection->GetPort());
	url->SetPath({ "v1", "types" });

	try {
		std::shared_ptr<HttpRequest> req = m_Connection->NewRequest();
		req->RequestMethod = "GET";
		req->RequestUrl = url;
		req->AddHeader("Authorization", "Basic " + Base64::Encode(m_User + ":" + m_Password));
		req->AddHeader("Accept", "application/json");
		m_Connection->SubmitRequest(req, std::bind(TypesHttpCompletionCallback, _1, _2, callback));
	} catch (const std::exception& ex) {
		callback(boost::current_exception(), std::vector<ApiType::Ptr>());
	}
}

void ApiClient::TypesHttpCompletionCallback(HttpRequest& request, HttpResponse& response,
    const TypesCompletionCallback& callback)
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

		std::vector<ApiType::Ptr> types;

		result = JsonDecode(body);

		Array::Ptr results = result->Get("results");

		ObjectLock olock(results);
		for (const Dictionary::Ptr typeInfo : results)
		{
			ApiType::Ptr type = new ApiType();;
			type->Abstract = typeInfo->Get("abstract");
			type->BaseName = typeInfo->Get("base");
			type->Name = typeInfo->Get("name");
			type->PluralName = typeInfo->Get("plural_name");
			// TODO: attributes
			types.emplace_back(std::move(type));
		}

		callback(boost::exception_ptr(), types);
	} catch (const std::exception& ex) {
		Log(LogCritical, "ApiClient")
		    << "Error while decoding response: " << DiagnosticInformation(ex);
		callback(boost::current_exception(), std::vector<ApiType::Ptr>());
	}

}

void ApiClient::GetObjects(const String& pluralType, const ObjectsCompletionCallback& callback,
    const std::vector<String>& names, const std::vector<String>& attrs, const std::vector<String>& joins, bool all_joins) const
{
	Url::Ptr url = new Url();
	url->SetScheme("https");
	url->SetHost(m_Connection->GetHost());
	url->SetPort(m_Connection->GetPort());
	url->SetPath({ "v1", "objects", pluralType });

	std::map<String, std::vector<String> > params;

	for (const String& name : names) {
		params[pluralType.ToLower()].push_back(name);
	}

	for (const String& attr : attrs) {
		params["attrs"].push_back(attr);
	}

	for (const String& join : joins) {
		params["joins"].push_back(join);
	}

	params["all_joins"].push_back(all_joins ? "1" : "0");

	url->SetQuery(params);

	try {
		std::shared_ptr<HttpRequest> req = m_Connection->NewRequest();
		req->RequestMethod = "GET";
		req->RequestUrl = url;
		req->AddHeader("Authorization", "Basic " + Base64::Encode(m_User + ":" + m_Password));
		req->AddHeader("Accept", "application/json");
		m_Connection->SubmitRequest(req, std::bind(ObjectsHttpCompletionCallback, _1, _2, callback));
	} catch (const std::exception& ex) {
		callback(boost::current_exception(), std::vector<ApiObject::Ptr>());
	}
}

void ApiClient::ObjectsHttpCompletionCallback(HttpRequest& request,
    HttpResponse& response, const ObjectsCompletionCallback& callback)
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

		std::vector<ApiObject::Ptr> objects;

		result = JsonDecode(body);

		Array::Ptr results = result->Get("results");

		if (results) {
			ObjectLock olock(results);
			for (const Dictionary::Ptr objectInfo : results) {
				ApiObject::Ptr object = new ApiObject();

				object->Name = objectInfo->Get("name");
				object->Type = objectInfo->Get("type");

				Dictionary::Ptr attrs = objectInfo->Get("attrs");

				if (attrs) {
					ObjectLock olock(attrs);
					for (const Dictionary::Pair& kv : attrs) {
						object->Attrs[object->Type.ToLower() + "." + kv.first] = kv.second;
					}
				}

				Dictionary::Ptr joins = objectInfo->Get("joins");

				if (joins) {
					ObjectLock olock(joins);
					for (const Dictionary::Pair& kv : joins) {
						Dictionary::Ptr attrs = kv.second;

						if (attrs) {
							ObjectLock olock(attrs);
							for (const Dictionary::Pair& kv2 : attrs) {
								object->Attrs[kv.first + "." + kv2.first] = kv2.second;
							}
						}
					}
				}

				Array::Ptr used_by = objectInfo->Get("used_by");

				if (used_by) {
					ObjectLock olock(used_by);
					for (const Dictionary::Ptr& refInfo : used_by) {
						ApiObjectReference ref;
						ref.Name = refInfo->Get("name");
						ref.Type = refInfo->Get("type");
						object->UsedBy.emplace_back(std::move(ref));
					}
				}

				objects.push_back(object);
			}
		}

		callback(boost::exception_ptr(), objects);
	} catch (const std::exception& ex) {
		Log(LogCritical, "ApiClient")
			<< "Error while decoding response: " << DiagnosticInformation(ex);
		callback(boost::current_exception(), std::vector<ApiObject::Ptr>());
	}
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
	params["sandboxed"].push_back(sandboxed ? "1" : "0");
	url->SetQuery(params);

	try {
		std::shared_ptr<HttpRequest> req = m_Connection->NewRequest();
		req->RequestMethod = "POST";
		req->RequestUrl = url;
		req->AddHeader("Authorization", "Basic " + Base64::Encode(m_User + ":" + m_Password));
		req->AddHeader("Accept", "application/json");
		m_Connection->SubmitRequest(req, std::bind(ExecuteScriptHttpCompletionCallback, _1, _2, callback));
	} catch (const std::exception& ex) {
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
	} catch (const std::exception& ex) {
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
	params["sandboxed"].push_back(sandboxed ? "1" : "0");
	url->SetQuery(params);

	try {
		std::shared_ptr<HttpRequest> req = m_Connection->NewRequest();
		req->RequestMethod = "POST";
		req->RequestUrl = url;
		req->AddHeader("Authorization", "Basic " + Base64::Encode(m_User + ":" + m_Password));
		req->AddHeader("Accept", "application/json");
		m_Connection->SubmitRequest(req, std::bind(AutocompleteScriptHttpCompletionCallback, _1, _2, callback));
	} catch (const std::exception& ex) {
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
	} catch (const std::exception& ex) {
		callback(boost::current_exception(), nullptr);
	}
}
