/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "remote/url.hpp"
#include "base/dictionary.hpp"
#include <boost/beast/http.hpp>
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

	static void SendJsonBody(boost::beast::http::response<boost::beast::http::string_body>& response, const Dictionary::Ptr& params, const Value& val);
	static void SendJsonError(boost::beast::http::response<boost::beast::http::string_body>& response, const Dictionary::Ptr& params, const int code,
		const String& verbose = String(), const String& diagnosticInformation = String());
};

}
