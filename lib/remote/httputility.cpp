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

#include "remote/httputility.hpp"
#include "base/json.hpp"
#include "base/logger.hpp"

using namespace icinga;

Dictionary::Ptr HttpUtility::FetchRequestParameters(HttpRequest& request)
{
	Dictionary::Ptr result;

	String body;
	char buffer[1024];
	size_t count;

	while ((count = request.ReadBody(buffer, sizeof(buffer))) > 0)
		body += String(buffer, buffer + count);

	if (!body.IsEmpty()) {
		Log(LogDebug, "HttpUtility")
			<< "Request body: '" << body << "'";

		result = JsonDecode(body);
	}

	if (!result)
		result = new Dictionary();

	typedef std::pair<String, std::vector<String> > kv_pair;
	for (const kv_pair& kv : request.RequestUrl->GetQuery()) {
		result->Set(kv.first, Array::FromVector(kv.second));
	}

	return result;
}

void HttpUtility::SendJsonBody(HttpResponse& response, const Dictionary::Ptr& params, const Value& val)
{
	response.AddHeader("Content-Type", "application/json");

	bool prettyPrint = false;

	if (params)
		prettyPrint = GetLastParameter(params, "pretty");

	String body = JsonEncode(val, prettyPrint);

	response.WriteBody(body.CStr(), body.GetLength());
}

Value HttpUtility::GetLastParameter(const Dictionary::Ptr& params, const String& key)
{
	Value varr = params->Get(key);

	if (!varr.IsObjectType<Array>())
		return varr;

	Array::Ptr arr = varr;

	if (arr->GetLength() == 0)
		return Empty;
	else
		return arr->Get(arr->GetLength() - 1);
}

void HttpUtility::SendJsonError(HttpResponse& response, const Dictionary::Ptr& params,
	int code, const String& info, const String& diagnosticInformation)
{
	Dictionary::Ptr result = new Dictionary();
	response.SetStatus(code, HttpUtility::GetErrorNameByCode(code));
	result->Set("error", code);

	bool verbose = false;

	if (params)
		verbose = HttpUtility::GetLastParameter(params, "verbose");

	if (!info.IsEmpty())
		result->Set("status", info);

	if (verbose) {
		if (!diagnosticInformation.IsEmpty())
			result->Set("diagnostic_information", diagnosticInformation);
	}

	HttpUtility::SendJsonBody(response, params, result);
}

String HttpUtility::GetErrorNameByCode(const int code)
{
	switch(code) {
		case 200:
			return "OK";
		case 201:
			return "Created";
		case 204:
			return "No Content";
		case 304:
			return "Not Modified";
		case 400:
			return "Bad Request";
		case 401:
			return "Unauthorized";
		case 403:
			return "Forbidden";
		case 404:
			return "Not Found";
		case 409:
			return "Conflict";
		case 500:
			return "Internal Server Error";
		default:
			return "Unknown Error Code";
	}
}

