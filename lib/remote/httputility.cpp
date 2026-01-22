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

void HttpUtility::SendJsonBody(HttpApiResponse& response, const Dictionary::Ptr& params, const Value& val)
{
	namespace http = boost::beast::http;

	response.set(http::field::content_type, "application/json");
	response.GetJsonEncoder(params && GetLastParameter(params, "pretty")).Encode(val);
}

void HttpUtility::SendJsonError(HttpApiResponse& response,
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

/**
 * Check if the given string is suitable to be used as an HTTP header name.
 *
 * @param name The value to check for validity
 * @return true if the argument is a valid header name, false otherwise
 */
bool HttpUtility::IsValidHeaderName(std::string_view name)
{
	/*
	 * Derived from the following syntax definition in RFC9110:
	 *
	 *     field-name = token
	 *     token = 1*tchar
	 *     tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*" / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~" / DIGIT / ALPHA
	 *     ALPHA = %x41-5A / %x61-7A   ; A-Z / a-z
	 *     DIGIT = %x30-39   ; 0-9
	 *
	 * References:
	 * - https://datatracker.ietf.org/doc/html/rfc9110#section-5.1
	 * - https://datatracker.ietf.org/doc/html/rfc9110#appendix-A
	 * - https://www.rfc-editor.org/rfc/rfc5234#appendix-B.1
	 */

	return !name.empty() && std::all_of(name.begin(), name.end(), [](char c) {
		switch (c) {
			case '!': case '#': case '$': case '%': case '&': case '\'': case '*': case '+':
			case '-': case '.': case '^': case '_': case '`': case '|': case '~':
				return true;
			default:
				return ('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
		}
	});
}

/**
 * Check if the given string is suitable to be used as an HTTP header value.
 *
 * @param value The value to check for validity
 * @return true if the argument is a valid header value, false otherwise
 */
bool HttpUtility::IsValidHeaderValue(std::string_view value)
{
	/*
	 * Derived from the following syntax definition in RFC9110:
	 *
	 *     field-value = *field-content
	 *     field-content = field-vchar [ 1*( SP / HTAB / field-vchar ) field-vchar ]
	 *     field-vchar = VCHAR / obs-text
	 *     obs-text = %x80-FF
	 *     VCHAR = %x21-7E ; visible (printing) characters
	 *
	 * References:
	 * - https://datatracker.ietf.org/doc/html/rfc9110#section-5.5
	 * - https://datatracker.ietf.org/doc/html/rfc9110#appendix-A
	 * - https://www.rfc-editor.org/rfc/rfc5234#appendix-B.1
	 */

	if (!value.empty()) {
		// Must not start or end with space or tab.
		for (char c : {value.front(), value.back()}) {
			if (c == ' ' || c == '\t') {
				return false;
			}
		}
	}

	return std::all_of(value.begin(), value.end(), [](char c) {
		return c == ' ' || c == '\t' || ('\x21' <= c && c <= '\x7e') || ('\x80' <= c && c <= '\xff');
	});
}
