/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef HTTPUTILITY_H
#define HTTPUTILITY_H

#include "remote/httprequest.hpp"
#include "remote/httpresponse.hpp"
#include "remote/url.hpp"
#include "base/dictionary.hpp"
#include <string>
#include <boost/beast/http.hpp>

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
	static void SendJsonBody(HttpResponse& response, const Dictionary::Ptr& params, const Value& val);
	static void SendJsonBody(boost::beast::http::response<boost::beast::http::string_body>& response, const Dictionary::Ptr& params, const Value& val);
	static Value GetLastParameter(const Dictionary::Ptr& params, const String& key);
	static void SendJsonError(HttpResponse& response, const Dictionary::Ptr& params, const int code,
		const String& verbose = String(), const String& diagnosticInformation = String());
	static void SendJsonError(boost::beast::http::response<boost::beast::http::string_body>& response, const Dictionary::Ptr& params, const int code,
		const String& verbose = String(), const String& diagnosticInformation = String());

private:
	static String GetErrorNameByCode(int code);

};

}

#endif /* HTTPUTILITY_H */
