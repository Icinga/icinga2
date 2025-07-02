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
	HttpRequest& request,
	HttpResponse& response,
	boost::asio::yield_context& yc,
	HttpServerConnection& server
)
{
	namespace http = boost::beast::http;

	if (request.method() != http::verb::get)
		return false;

	const std::vector<String>& urlPath = request.Url()->GetPath();

	if (urlPath.size() >= 4)
		request.Params()->Set("package", urlPath[3]);

	if (urlPath.size() >= 5)
		request.Params()->Set("stage", urlPath[4]);

	if (urlPath.size() >= 6) {
		std::vector<String> tmpPath(urlPath.begin() + 5, urlPath.end());
		request.Params()->Set("path", boost::algorithm::join(tmpPath, "/"));
	}

	if (request[http::field::accept] == "application/json") {
		response.SendJsonError(request.Params(), 400, "Invalid Accept header. Either remove the Accept header or set it to 'application/octet-stream'.");
		return true;
	}

	FilterUtility::CheckPermission(request.User(), "config/query");

	String packageName = request.GetLastParameter("package");
	String stageName = request.GetLastParameter("stage");

	if (!ConfigPackageUtility::ValidatePackageName(packageName)) {
		response.SendJsonError(request.Params(), 400, "Invalid package name.");
		return true;
	}

	if (!ConfigPackageUtility::ValidateStageName(stageName)) {
		response.SendJsonError(request.Params(), 400, "Invalid stage name.");
		return true;
	}

	String relativePath = request.GetLastParameter("path");

	if (ConfigPackageUtility::ContainsDotDot(relativePath)) {
		response.SendJsonError(request.Params(), 400, "Path contains '..' (not allowed).");
		return true;
	}

	String path = ConfigPackageUtility::GetPackageDir() + "/" + packageName + "/" + stageName + "/" + relativePath;

	if (!Utility::PathExists(path)) {
		response.SendJsonError(request.Params(), 404, "Path not found.");
		return true;
	}

	try {
		std::ifstream fp(path.CStr(), std::ifstream::in | std::ifstream::binary);
		fp.exceptions(std::ifstream::badbit);

		String content((std::istreambuf_iterator<char>(fp)), std::istreambuf_iterator<char>());
		response.result(http::status::ok);
		response.set(http::field::content_type, "application/octet-stream");
		response.body() << content;
		response.prepare_payload();
	} catch (const std::exception& ex) {
		response.SendJsonError(request.Params(), 500, "Could not read file.",
			DiagnosticInformation(ex));
	}

	return true;
}
