/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/configpackageshandler.hpp"
#include "remote/configpackageutility.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "base/exception.hpp"

using namespace icinga;

REGISTER_URLHANDLER("/v1/config/packages", ConfigPackagesHandler);

bool ConfigPackagesHandler::HandleRequest(
	const ApiUser::Ptr& user,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	const Url::Ptr& url,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	const Dictionary::Ptr& params
)
{
	namespace http = boost::beast::http;

	if (url->GetPath().size() > 4)
		return false;

	if (request.method() == http::verb::get)
		HandleGet(user, request, url, response, params);
	else if (request.method() == http::verb::post)
		HandlePost(user, request, url, response, params);
	else if (request.method() == http::verb::delete_)
		HandleDelete(user, request, url, response, params);
	else
		return false;

	return true;
}

void ConfigPackagesHandler::HandleGet(
	const ApiUser::Ptr& user,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	const Url::Ptr& url,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	const Dictionary::Ptr& params
)
{
	namespace http = boost::beast::http;

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
		boost::mutex::scoped_lock lock(ConfigPackageUtility::GetStaticMutex());
		for (const String& package : packages) {
			results.emplace_back(new Dictionary({
				{ "name", package },
				{ "stages", Array::FromVector(ConfigPackageUtility::GetStages(package)) },
				{ "active-stage", ConfigPackageUtility::GetActiveStage(package) }
			}));
		}
	}

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array(std::move(results)) }
	});

	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result);
}

void ConfigPackagesHandler::HandlePost(
	const ApiUser::Ptr& user,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	const Url::Ptr& url,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	const Dictionary::Ptr& params
)
{
	namespace http = boost::beast::http;

	FilterUtility::CheckPermission(user, "config/modify");

	if (url->GetPath().size() >= 4)
		params->Set("package", url->GetPath()[3]);

	String packageName = HttpUtility::GetLastParameter(params, "package");

	if (!ConfigPackageUtility::ValidateName(packageName)) {
		HttpUtility::SendJsonError(response, params, 400, "Invalid package name '" + packageName + "'.");
		return;
	}

	try {
		boost::mutex::scoped_lock lock(ConfigPackageUtility::GetStaticMutex());
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

void ConfigPackagesHandler::HandleDelete(
	const ApiUser::Ptr& user,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	const Url::Ptr& url,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	const Dictionary::Ptr& params
)
{
	namespace http = boost::beast::http;

	FilterUtility::CheckPermission(user, "config/modify");

	if (url->GetPath().size() >= 4)
		params->Set("package", url->GetPath()[3]);

	String packageName = HttpUtility::GetLastParameter(params, "package");

	if (!ConfigPackageUtility::ValidateName(packageName)) {
		HttpUtility::SendJsonError(response, params, 400, "Invalid package name '" + packageName + "'.");
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
