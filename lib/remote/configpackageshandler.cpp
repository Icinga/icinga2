/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/configpackageshandler.hpp"
#include "remote/configpackageutility.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "base/exception.hpp"

using namespace icinga;

REGISTER_URLHANDLER("/v1/config/packages", ConfigPackagesHandler);

bool ConfigPackagesHandler::HandleRequest(
	const WaitGroup::Ptr&,
	HttpRequest& request,
	HttpResponse& response,
	boost::asio::yield_context& yc,
	HttpServerConnection& server
)
{
	namespace http = boost::beast::http;

	if (request.Url()->GetPath().size() > 4)
		return false;

	if (request.method() == http::verb::get)
		HandleGet(request, response);
	else if (request.method() == http::verb::post)
		HandlePost(request, response);
	else if (request.method() == http::verb::delete_)
		HandleDelete(request, response);
	else
		return false;

	return true;
}

void ConfigPackagesHandler::HandleGet(HttpRequest& request, HttpResponse& response)
{
	namespace http = boost::beast::http;

	FilterUtility::CheckPermission(request.User(), "config/query");

	std::vector<String> packages;

	try {
		packages = ConfigPackageUtility::GetPackages();
	} catch (const std::exception& ex) {
		response.SendJsonError(request.Params(), 500, "Could not retrieve packages.",
			DiagnosticInformation(ex));
		return;
	}

	ArrayData results;

	{
		std::unique_lock<std::mutex> lock(ConfigPackageUtility::GetStaticPackageMutex());

		for (const String& package : packages) {
			String activeStage;

			try {
				activeStage = ConfigPackageUtility::GetActiveStage(package);
			} catch (const std::exception&) { } /* Should never happen. */

			results.emplace_back(new Dictionary({
				{ "name", package },
				{ "stages", Array::FromVector(ConfigPackageUtility::GetStages(package)) },
				{ "active-stage", activeStage }
			}));
		}
	}

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array(std::move(results)) }
	});

	response.result(http::status::ok);
	response.SendJsonBody(request.Params(), result);
}

void ConfigPackagesHandler::HandlePost(HttpRequest& request, HttpResponse& response)
{
	namespace http = boost::beast::http;

	FilterUtility::CheckPermission(request.User(), "config/modify");

	if (request.Url()->GetPath().size() >= 4)
		request.Params()->Set("package", request.Url()->GetPath()[3]);

	String packageName = request.GetLastParameter("package");

	if (!ConfigPackageUtility::ValidatePackageName(packageName)) {
		response.SendJsonError(request.Params(), 400, "Invalid package name '" + packageName + "'.");
		return;
	}

	try {
		std::unique_lock<std::mutex> lock(ConfigPackageUtility::GetStaticPackageMutex());

		ConfigPackageUtility::CreatePackage(packageName);
	} catch (const std::exception& ex) {
		response.SendJsonError(request.Params(), 500, "Could not create package '" + packageName + "'.",
			DiagnosticInformation(ex));
		return;
	}

	Dictionary::Ptr result1 = new Dictionary({
		{ "code", 200 },
		{ "package", packageName },
		{ "status", "Created package." }
	});

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array({ result1 }) }
	});

	response.result(http::status::ok);
	response.SendJsonBody(request.Params(), result);
}

void ConfigPackagesHandler::HandleDelete(HttpRequest& request, HttpResponse& response)
{
	namespace http = boost::beast::http;

	FilterUtility::CheckPermission(request.User(), "config/modify");

	if (request.Url()->GetPath().size() >= 4)
		request.Params()->Set("package", request.Url()->GetPath()[3]);

	String packageName = request.GetLastParameter("package");

	if (!ConfigPackageUtility::ValidatePackageName(packageName)) {
		response.SendJsonError(request.Params(), 400, "Invalid package name '" + packageName + "'.");
		return;
	}

	try {
		ConfigPackageUtility::DeletePackage(packageName);
	} catch (const std::exception& ex) {
		response.SendJsonError(request.Params(), 500, "Failed to delete package '" + packageName + "'.",
			DiagnosticInformation(ex));
		return;
	}

	Dictionary::Ptr result1 = new Dictionary({
		{ "code", 200 },
		{ "package", packageName },
		{ "status", "Deleted package." }
	});

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array({ result1 }) }
	});

	response.result(http::status::ok);
	response.SendJsonBody(request.Params(), result);
}
