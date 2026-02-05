// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef HTTPUTILITY_H
#define HTTPUTILITY_H

#include "remote/url.hpp"
#include "base/dictionary.hpp"
#include "remote/httpmessage.hpp"
#include <boost/asio/spawn.hpp>
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

	static void SendJsonBody(HttpApiResponse& response, const Dictionary::Ptr& params, const Value& val, boost::asio::yield_context& yc);
	static void SendJsonBody(HttpApiResponse& response, const Dictionary::Ptr& params, const Value& val);
	static void SendJsonError(HttpApiResponse& response, const Dictionary::Ptr& params, const int code,
		const String& info = {}, const String& diagnosticInformation = {});

	static bool IsValidHeaderName(std::string_view name);
	static bool IsValidHeaderValue(std::string_view value);
};

}

#endif /* HTTPUTILITY_H */
