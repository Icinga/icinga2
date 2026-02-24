// SPDX-FileCopyrightText: 2024 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "remote/filterutility.hpp"
#include "remote/httputility.hpp"
#include "remote/malloctrimhandler.hpp"
#include <boost/lexical_cast.hpp>
#include <cstddef>

#ifdef HAVE_MALLOC_TRIM
#	include <malloc.h>
#endif /* HAVE_MALLOC_TRIM */

using namespace icinga;

REGISTER_URLHANDLER("/v1/debug/malloc_trim", MallocTrimHandler);

bool MallocTrimHandler::HandleRequest(
	const WaitGroup::Ptr&,
	const HttpApiRequest& request,
	HttpApiResponse& response,
	boost::asio::yield_context&
)
{
	namespace http = boost::beast::http;

	auto url = request.Url();
	auto user = request.User();
	auto params = request.Params();

	if (url->GetPath().size() != 3) {
		return false;
	}

	if (request.method() != http::verb::post) {
		return false;
	}

	auto rawPad (HttpUtility::GetLastParameter(params, "pad"));
	size_t pad = 0;

	if (rawPad.GetType() != ValueEmpty) {
		try {
			pad = boost::lexical_cast<size_t>(rawPad);
		} catch (const std::exception&) {
			HttpUtility::SendJsonError(response, params, 400,
				"Invalid 'pad' specified. An integer [0," BOOST_PP_STRINGIZE(SIZE_MAX) "] is required.");

			return true;
		}
	}

	FilterUtility::CheckPermission(user, "debug");

#ifndef HAVE_MALLOC_TRIM
	HttpUtility::SendJsonError(response, params, 501, "malloc_trim(3) not available.");
#else /* HAVE_MALLOC_TRIM */
	Dictionary::Ptr result1;
	auto ret (malloc_trim(pad));

	if (ret) {
		result1 = new Dictionary({
			{ "code", 200 },
			{ "malloc_trim", ret },
			{ "status", "Some memory was released back to the system." }
		});
	} else {
		result1 = new Dictionary({
			{ "code", 503 },
			{ "malloc_trim", ret },
			{ "status", "It was not possible to release any memory." }
		});

		response.result(http::status::service_unavailable);
	}

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array({ result1 }) }
	});

	HttpUtility::SendJsonBody(response, params, result);
#endif /* HAVE_MALLOC_TRIM */

	return true;
}
