/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/configpackageshandler.hpp"
#include "remote/configpackageutility.hpp"
#include "remote/configobjectslock.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "base/exception.hpp"
#include "base/io-future.hpp"

using namespace icinga;

REGISTER_URLHANDLER("/v1/config/packages", ConfigPackagesHandler);

bool ConfigPackagesHandler::HandleRequest(
	const WaitGroup::Ptr&,
	const HttpRequest& request,
	HttpResponse& response,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;

	return QueueAsioFutureCallback([&]() {
		auto url = request.Url();
		auto user = request.User();
		auto params = request.Params();

		if (url->GetPath().size() > 4)
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
	})->Get(yc);
}

void ConfigPackagesHandler::HandleGet(const HttpRequest& request, HttpResponse& response)
{
	namespace http = boost::beast::http;

	auto url = request.Url();
	auto user = request.User();
	auto params = request.Params();

	FilterUtility::CheckPermission(user, "config/query");

	std::vector<String> packages;

	try {
		packages = ConfigPackageUtility::GetPackages();
	} catch (const std::exception& ex) {
		HttpUtility::SendJsonError(response, params, 500, "Could not retrieve packages.",
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
	HttpUtility::SendJsonBody(response, params, result);
}

void ConfigPackagesHandler::HandlePost(const HttpRequest& request, HttpResponse& response)
{
	namespace http = boost::beast::http;

	auto url = request.Url();
	auto user = request.User();
	auto params = request.Params();

	FilterUtility::CheckPermission(user, "config/modify");

	if (url->GetPath().size() >= 4)
		params->Set("package", url->GetPath()[3]);

	String packageName = HttpUtility::GetLastParameter(params, "package");

	if (!ConfigPackageUtility::ValidatePackageName(packageName)) {
		HttpUtility::SendJsonError(response, params, 400, "Invalid package name '" + packageName + "'.");
		return;
	}

	ConfigObjectsSharedLock configObjectsSharedLock(std::try_to_lock);
	if (!configObjectsSharedLock) {
		HttpUtility::SendJsonError(response, params, 503, "Icinga is reloading");
		return;
	}

	try {
		std::unique_lock<std::mutex> lock(ConfigPackageUtility::GetStaticPackageMutex());

		ConfigPackageUtility::CreatePackage(packageName);
	} catch (const std::exception& ex) {
		HttpUtility::SendJsonError(response, params, 500, "Could not create package '" + packageName + "'.",
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
	HttpUtility::SendJsonBody(response, params, result);
}

void ConfigPackagesHandler::HandleDelete(const HttpRequest& request, HttpResponse& response)
{
	namespace http = boost::beast::http;

	auto url = request.Url();
	auto user = request.User();
	auto params = request.Params();

	FilterUtility::CheckPermission(user, "config/modify");

	if (url->GetPath().size() >= 4)
		params->Set("package", url->GetPath()[3]);

	String packageName = HttpUtility::GetLastParameter(params, "package");

	if (!ConfigPackageUtility::ValidatePackageName(packageName)) {
		HttpUtility::SendJsonError(response, params, 400, "Invalid package name '" + packageName + "'.");
		return;
	}

	ConfigObjectsSharedLock lock(std::try_to_lock);
	if (!lock) {
		HttpUtility::SendJsonError(response, params, 503, "Icinga is reloading");
		return;
	}

	try {
		ConfigPackageUtility::DeletePackage(packageName);
	} catch (const std::exception& ex) {
		HttpUtility::SendJsonError(response, params, 500, "Failed to delete package '" + packageName + "'.",
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
	HttpUtility::SendJsonBody(response, params, result);
}
