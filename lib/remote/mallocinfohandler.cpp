/* Icinga 2 | (c) 2024 Icinga GmbH | GPLv2+ */

#include "base/defer.hpp"
#include "remote/filterutility.hpp"
#include "remote/httputility.hpp"
#include "remote/mallocinfohandler.hpp"
#include <cstddef>
#include <cstdio>
#include <string>

#ifdef HAVE_MALLOC_INFO
#	include <errno.h>
#	include <malloc.h>
#endif /* HAVE_MALLOC_INFO */

using namespace icinga;

REGISTER_URLHANDLER("/v1/debug/malloc_info", MallocInfoHandler);

bool MallocInfoHandler::HandleRequest(
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

	if (request.method() != http::verb::get) {
		return false;
	}

	FilterUtility::CheckPermission(user, "debug");

#ifndef HAVE_MALLOC_INFO
	HttpUtility::SendJsonError(response, params, 501, "malloc_info(3) not available.");
#else /* HAVE_MALLOC_INFO */
	char* buf = nullptr;
	size_t bufSize = 0;
	FILE* f = nullptr;

	Defer release ([&f, &buf]() {
		if (f) {
			(void)fclose(f);
		}

		free(buf);
	});

	f = open_memstream(&buf, &bufSize);

	if (!f) {
		auto error (errno);

		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("open_memstream")
			<< boost::errinfo_errno(error));
	}

	if (malloc_info(0, f)) {
		auto error (errno);

		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("malloc_info")
			<< boost::errinfo_errno(error));
	}

	auto closeErr (fclose(f));
	f = nullptr;

	if (closeErr) {
		auto error (errno);

		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("fclose")
			<< boost::errinfo_errno(error));
	}

	response.result(200);
	response.set(http::field::content_type, "application/xml");
	response.body() << std::string_view(buf, bufSize);
#endif /* HAVE_MALLOC_INFO */

	return true;
}
