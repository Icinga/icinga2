/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/configstageshandler.hpp"
#include "remote/configpackageutility.hpp"
#include "remote/filterutility.hpp"
#include "base/application.hpp"
#include "base/defer.hpp"
#include "base/exception.hpp"

using namespace icinga;

REGISTER_URLHANDLER("/v1/config/stages", ConfigStagesHandler);

std::atomic<bool> ConfigStagesHandler::m_RunningPackageUpdates (false);

bool ConfigStagesHandler::HandleRequest(
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

	if (url->GetPath().size() > 5)
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

void ConfigStagesHandler::HandleGet(const HttpRequest& request, HttpResponse& response)
{
	namespace http = boost::beast::http;

	auto url = request.Url();
	auto user = request.User();
	auto params = request.Params();

	FilterUtility::CheckPermission(user, "config/query");

	if (url->GetPath().size() >= 4)
		params->Set("package", url->GetPath()[3]);

	if (url->GetPath().size() >= 5)
		params->Set("stage", url->GetPath()[4]);

	String packageName = request.GetLastParameter("package");
	String stageName = request.GetLastParameter("stage");

	if (!ConfigPackageUtility::ValidatePackageName(packageName))
		return response.SendJsonError(params, 400, "Invalid package name '" + packageName + "'.");

	if (!ConfigPackageUtility::ValidateStageName(stageName))
		return response.SendJsonError(params, 400, "Invalid stage name '" + stageName + "'.");

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
	response.SendJsonBody(result, request.IsPretty());
}

void ConfigStagesHandler::HandlePost(const HttpRequest& request, HttpResponse& response)
{
	namespace http = boost::beast::http;

	auto url = request.Url();
	auto user = request.User();
	auto params = request.Params();

	FilterUtility::CheckPermission(user, "config/modify");

	if (url->GetPath().size() >= 4)
		params->Set("package", url->GetPath()[3]);

	String packageName = request.GetLastParameter("package");

	if (!ConfigPackageUtility::ValidatePackageName(packageName))
		return response.SendJsonError(params, 400, "Invalid package name '" + packageName + "'.");

	bool reload = true;

	if (params->Contains("reload"))
		reload = request.GetLastParameter("reload");

	bool activate = true;

	if (params->Contains("activate"))
		activate = request.GetLastParameter("activate");

	Dictionary::Ptr files = params->Get("files");

	String stageName;

	try {
		if (!files)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Parameter 'files' must be specified."));

		if (reload && !activate)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Parameter 'reload' must be false when 'activate' is false."));

		if (m_RunningPackageUpdates.exchange(true)) {
			return response.SendJsonError(params, 423,
				"Conflicting request, there is already an ongoing package update in progress. Please try it again later.");
		}

		auto resetPackageUpdates (Shared<Defer>::Make([]() { ConfigStagesHandler::m_RunningPackageUpdates.store(false); }));

		std::unique_lock<std::mutex> lock(ConfigPackageUtility::GetStaticPackageMutex());

		stageName = ConfigPackageUtility::CreateStage(packageName, files);

		/* validate the config. on success, activate stage and reload */
		ConfigPackageUtility::AsyncTryActivateStage(packageName, stageName, activate, reload, resetPackageUpdates);
	} catch (const std::exception& ex) {
		return response.SendJsonError(params, 500,
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
	response.SendJsonBody(result, request.IsPretty());
}

void ConfigStagesHandler::HandleDelete(const HttpRequest& request, HttpResponse& response)
{
	namespace http = boost::beast::http;

	auto url = request.Url();
	auto user = request.User();
	auto params = request.Params();

	FilterUtility::CheckPermission(user, "config/modify");

	if (url->GetPath().size() >= 4)
		params->Set("package", url->GetPath()[3]);

	if (url->GetPath().size() >= 5)
		params->Set("stage", url->GetPath()[4]);

	String packageName = request.GetLastParameter("package");
	String stageName = request.GetLastParameter("stage");

	if (!ConfigPackageUtility::ValidatePackageName(packageName))
		return response.SendJsonError(params, 400, "Invalid package name '" + packageName + "'.");

	if (!ConfigPackageUtility::ValidateStageName(stageName))
		return response.SendJsonError(params, 400, "Invalid stage name '" + stageName + "'.");

	try {
		ConfigPackageUtility::DeleteStage(packageName, stageName);
	} catch (const std::exception& ex) {
		return response.SendJsonError(params, 500,
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
	response.SendJsonBody(result, request.IsPretty());
}
