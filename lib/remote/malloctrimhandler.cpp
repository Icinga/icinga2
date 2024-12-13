/* Icinga 2 | (c) 2024 Icinga GmbH | GPLv2+ */

#include "remote/filterutility.hpp"
#include "remote/httputility.hpp"
#include "remote/malloctrimhandler.hpp"

#ifdef HAVE_MALLOC_TRIM
#	include <malloc.h>
#endif /* HAVE_MALLOC_TRIM */

using namespace icinga;

REGISTER_URLHANDLER("/v1/debug/malloc_trim", MallocTrimHandler);

bool MallocTrimHandler::HandleRequest(
	AsioTlsStream&,
	const ApiUser::Ptr& user,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	const Url::Ptr& url,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	const Dictionary::Ptr& params,
	boost::asio::yield_context&,
	HttpServerConnection&
)
{
	namespace http = boost::beast::http;

	if (url->GetPath().size() != 3) {
		return false;
	}

	if (request.method() != http::verb::post) {
		return false;
	}

	double pad;

	try {
		pad = params->Get("pad");
	} catch (const std::exception&) {
		HttpUtility::SendJsonError(response, params, 400,
			"Invalid type for 'pad' specified. A number is required.");

		return true;
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
