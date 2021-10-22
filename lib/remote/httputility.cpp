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

void HttpUtility::SendJsonBody(boost::beast::http::response<boost::beast::http::string_body>& response, const Dictionary::Ptr& params, const Value& val)
{
	namespace http = boost::beast::http;

	response.set(http::field::content_type, "application/json");
	response.body() = JsonEncode(val, params && GetLastParameter(params, "pretty"));
	response.content_length(response.body().size());
}

void HttpUtility::SendJsonError(boost::beast::http::response<boost::beast::http::string_body>& response,
	const Dictionary::Ptr& params, int code, const String& info, const String& diagnosticInformation)
{
	Dictionary::Ptr result = new Dictionary({ { "error", code } });

	if (!info.IsEmpty()) {
		result->Set("status", info);
	}

	if (params && HttpUtility::GetLastParameter(params, "verbose") && !diagnosticInformation.IsEmpty()) {
		result->Set("diagnostic_information", diagnosticInformation);
	}

	response.result(code);

	HttpUtility::SendJsonBody(response, params, result);
}
