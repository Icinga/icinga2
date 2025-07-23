/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/configfileshandler.hpp"
#include "remote/configpackageutility.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "base/exception.hpp"
#include "base/utility.hpp"
#include <boost/algorithm/string/join.hpp>
#include <fstream>

using namespace icinga;

REGISTER_URLHANDLER("/v1/config/files", ConfigFilesHandler);

bool ConfigFilesHandler::HandleRequest(
	const WaitGroup::Ptr&,
	AsioTlsStream& stream,
	const HttpRequest& request,
	HttpResponse& response,
	boost::asio::yield_context& yc,
	HttpServerConnection& server
)
{
	namespace http = boost::beast::http;

	auto url = request.Url();
	auto user = request.User();
	auto params = request.Params();

	if (request.method() != http::verb::get)
		return false;

	const std::vector<String>& urlPath = url->GetPath();

	if (urlPath.size() >= 4)
		params->Set("package", urlPath[3]);

	if (urlPath.size() >= 5)
		params->Set("stage", urlPath[4]);

	if (urlPath.size() >= 6) {
		std::vector<String> tmpPath(urlPath.begin() + 5, urlPath.end());
		params->Set("path", boost::algorithm::join(tmpPath, "/"));
	}

	if (request[http::field::accept] == "application/json") {
		HttpUtility::SendJsonError(response, params, 400, "Invalid Accept header. Either remove the Accept header or set it to 'application/octet-stream'.");
		return true;
	}

	FilterUtility::CheckPermission(user, "config/query");

	String packageName = HttpUtility::GetLastParameter(params, "package");
	String stageName = HttpUtility::GetLastParameter(params, "stage");

	if (!ConfigPackageUtility::ValidatePackageName(packageName)) {
		HttpUtility::SendJsonError(response, params, 400, "Invalid package name.");
		return true;
	}

	if (!ConfigPackageUtility::ValidateStageName(stageName)) {
		HttpUtility::SendJsonError(response, params, 400, "Invalid stage name.");
		return true;
	}

	String relativePath = HttpUtility::GetLastParameter(params, "path");

	if (ConfigPackageUtility::ContainsDotDot(relativePath)) {
		HttpUtility::SendJsonError(response, params, 400, "Path contains '..' (not allowed).");
		return true;
	}

	String path = ConfigPackageUtility::GetPackageDir() + "/" + packageName + "/" + stageName + "/" + relativePath;

	if (!Utility::PathExists(path)) {
		HttpUtility::SendJsonError(response, params, 404, "Path not found.");
		return true;
	}

	try {
		std::ifstream fp(path.CStr(), std::ifstream::in | std::ifstream::binary);
		fp.exceptions(std::ifstream::badbit);

		response.result(http::status::ok);
		response.set(http::field::content_type, "application/octet-stream");
		response.body() << fp.rdbuf();
	} catch (const std::exception& ex) {
		HttpUtility::SendJsonError(response, params, 500, "Could not read file.",
			DiagnosticInformation(ex));
	}

	return true;
}
