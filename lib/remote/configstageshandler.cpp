/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/configstageshandler.hpp"
#include "remote/configpackageutility.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "base/application.hpp"
#include "base/exception.hpp"

using namespace icinga;

REGISTER_URLHANDLER("/v1/config/stages", ConfigStagesHandler);

bool ConfigStagesHandler::HandleRequest(
	const ApiUser::Ptr& user,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	const Url::Ptr& url,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	const Dictionary::Ptr& params
)
{
	namespace http = boost::beast::http;

	if (url->GetPath().size() > 5)
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

void ConfigStagesHandler::HandleGet(
	const ApiUser::Ptr& user,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	const Url::Ptr& url,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	const Dictionary::Ptr& params
)
{
	namespace http = boost::beast::http;

	FilterUtility::CheckPermission(user, "config/query");

	if (url->GetPath().size() >= 4)
		params->Set("package", url->GetPath()[3]);

	if (url->GetPath().size() >= 5)
		params->Set("stage", url->GetPath()[4]);

	String packageName = HttpUtility::GetLastParameter(params, "package");
	String stageName = HttpUtility::GetLastParameter(params, "stage");

	if (!ConfigPackageUtility::ValidateName(packageName))
		return HttpUtility::SendJsonError(response, params, 400, "Invalid package name '" + packageName + "'.");

	if (!ConfigPackageUtility::ValidateName(stageName))
		return HttpUtility::SendJsonError(response, params, 400, "Invalid stage name '" + stageName + "'.");

	ArrayData results;

	std::vector<std::pair<String, bool> > paths = ConfigPackageUtility::GetFiles(packageName, stageName);

	String prefixPath = ConfigPackageUtility::GetPackageDir() + "/" + packageName + "/" + stageName + "/";

	for (const auto& kv : paths) {
		results.push_back(new Dictionary({
			{ "type", kv.second ? "directory" : "file" },
			{ "name", kv.first.SubStr(prefixPath.GetLength()) }
		}));
	}

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array(std::move(results)) }
	});

	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result);
}

void ConfigStagesHandler::HandlePost(
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

	if (!ConfigPackageUtility::ValidateName(packageName))
		return HttpUtility::SendJsonError(response, params, 400, "Invalid package name '" + packageName + "'.");

	bool reload = true;

	if (params->Contains("reload"))
		reload = HttpUtility::GetLastParameter(params, "reload");

	Dictionary::Ptr files = params->Get("files");

	String stageName;

	try {
		if (!files)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Parameter 'files' must be specified."));

		boost::mutex::scoped_lock lock(ConfigPackageUtility::GetStaticMutex());
		stageName = ConfigPackageUtility::CreateStage(packageName, files);

		/* validate the config. on success, activate stage and reload */
		ConfigPackageUtility::AsyncTryActivateStage(packageName, stageName, reload);
	} catch (const std::exception& ex) {
		return HttpUtility::SendJsonError(response, params, 500,
			"Stage creation failed.",
			DiagnosticInformation(ex));
	}


	String responseStatus = "Created stage. ";

	if (reload)
		responseStatus += "Reload triggered.";
	else
		responseStatus += "Reload skipped.";

	Dictionary::Ptr result1 = new Dictionary({
		{ "package", packageName },
		{ "stage", stageName },
		{ "code", 200 },
		{ "status", responseStatus }
	});

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array({ result1 }) }
	});

	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result);
}

void ConfigStagesHandler::HandleDelete(
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

	if (url->GetPath().size() >= 5)
		params->Set("stage", url->GetPath()[4]);

	String packageName = HttpUtility::GetLastParameter(params, "package");
	String stageName = HttpUtility::GetLastParameter(params, "stage");

	if (!ConfigPackageUtility::ValidateName(packageName))
		return HttpUtility::SendJsonError(response, params, 400, "Invalid package name '" + packageName + "'.");

	if (!ConfigPackageUtility::ValidateName(stageName))
		return HttpUtility::SendJsonError(response, params, 400, "Invalid stage name '" + stageName + "'.");

	try {
		ConfigPackageUtility::DeleteStage(packageName, stageName);
	} catch (const std::exception& ex) {
		return HttpUtility::SendJsonError(response, params, 500,
			"Failed to delete stage '" + stageName + "' in package '" + packageName + "'.",
			DiagnosticInformation(ex));
	}

	Dictionary::Ptr result1 = new Dictionary({
		{ "code", 200 },
		{ "package", packageName },
		{ "stage", stageName },
		{ "status", "Stage deleted." }
	});

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array({ result1 }) }
	});

	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result);
}

