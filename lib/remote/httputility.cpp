/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/httputility.hpp"
#include "remote/url.hpp"
#include "base/json.hpp"
#include "base/logger.hpp"
#include <map>
#include <string>
#include <vector>
#include <boost/beast/http.hpp>

using namespace icinga;

Dictionary::Ptr HttpUtility::FetchRequestParameters(const Url::Ptr& url, const std::string& body)
{
	Dictionary::Ptr result;

	if (!body.empty()) {
		Log(LogDebug, "HttpUtility")
			<< "Request body: '" << body << '\'';

		result = JsonDecode(body);
	}

	if (!result)
		result = new Dictionary();

	std::map<String, std::vector<String>> query;
	for (const auto& kv : url->GetQuery()) {
		query[kv.first].emplace_back(kv.second);
	}

	for (auto& kv : query) {
		result->Set(kv.first, Array::FromVector(kv.second));
	}

	return result;
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

/**
 * Stream a JSON-encoded body to the client.
 *
 * This function sets the Content-Type header to "application/json", starts the streaming of the response,
 * and encodes the given value as JSON to the client. If pretty-print is requested, the JSON output will be
 * formatted accordingly. It is assumed that the response status code and other necessary headers have already
 * been set.
 *
 * @param response The HTTP response to send the body to.
 * @param params The request parameters.
 * @param val The value to encode as JSON and stream to the client.
 * @param yc The yield context to use for asynchronous operations.
 */
void HttpUtility::SendJsonBody(HttpResponse& response, const Dictionary::Ptr& params, const Value& val, boost::asio::yield_context& yc)
{
	namespace http = boost::beast::http;

	response.set(http::field::content_type, "application/json");
	response.StartStreaming();
	response.GetJsonEncoder(params && GetLastParameter(params, "pretty")).Encode(val, &yc);
}

void HttpUtility::SendJsonBody(HttpResponse& response, const Dictionary::Ptr& params, const Value& val)
{
	namespace http = boost::beast::http;

	response.set(http::field::content_type, "application/json");
	response.GetJsonEncoder(params && GetLastParameter(params, "pretty")).Encode(val);
}

void HttpUtility::SendJsonError(HttpResponse& response,
	const Dictionary::Ptr& params, int code, const String& info, const String& diagnosticInformation)
{
	Dictionary::Ptr result = new Dictionary({ { "error", code } });

	if (!info.IsEmpty()) {
		result->Set("status", info);
	}

	if (params && HttpUtility::GetLastParameter(params, "verbose") && !diagnosticInformation.IsEmpty()) {
		result->Set("diagnostic_information", diagnosticInformation);
	}

	response.Clear();
	response.result(code);

	HttpUtility::SendJsonBody(response, params, result);
}
