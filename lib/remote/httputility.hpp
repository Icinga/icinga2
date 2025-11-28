/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef HTTPUTILITY_H
#define HTTPUTILITY_H

#include "remote/url.hpp"
#include "base/dictionary.hpp"
#include "remote/httpmessage.hpp"
#include <string>

namespace icinga
{

/**
 * Helper functions.
 *
 * @ingroup remote
 */
class HttpUtility
{

public:
	static Dictionary::Ptr FetchRequestParameters(const Url::Ptr& url, const std::string& body);
	static Value GetLastParameter(const Dictionary::Ptr& params, const String& key);

	static void SendJsonBody(HttpResponse& response, const Dictionary::Ptr& params, const Value& val);
	static void SendJsonError(HttpResponse& response, const Dictionary::Ptr& params, const int code,
		const String& info = {}, const String& diagnosticInformation = {});

	static bool IsValidHeaderName(std::string_view name);
	static bool IsValidHeaderValue(std::string_view value);
};

}

#endif /* HTTPUTILITY_H */
