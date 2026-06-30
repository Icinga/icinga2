// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "remote/configstageshandler.hpp"
#include "remote/configpackageutility.hpp"
#include "remote/configobjectslock.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "base/application.hpp"
#include "base/defer.hpp"
#include "base/exception.hpp"

using namespace icinga;

REGISTER_URLHANDLER("/v1/config/stages", ConfigStagesHandler);

static bool l_RunningPackageUpdates(false);
// A timestamp that indicates the last time an Icinga 2 reload failed.
static double l_LastReloadFailedTime(0);
static std::mutex l_RunningPackageUpdatesMutex; // Protects the above two variables.

bool ConfigStagesHandler::HandleRequest(
	const WaitGroup::Ptr&,
	const HttpApiRequest& request,
	HttpApiResponse& response,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;

	auto url = request.Url();
	auto user = request.User();
	auto params = request.Params();

	if (url->GetPath().size() > 5)
		return false;

	if (request.method() == http::verb::get)
		HandleGet(request, response, yc);
	else if (request.method() == http::verb::post)
		HandlePost(request, response);
	else if (request.method() == http::verb::delete_)
		HandleDelete(request, response);
	else
		return false;

	return true;
}

void ConfigStagesHandler::HandleGet(const HttpApiRequest& request, HttpApiResponse& response, boost::asio::yield_context& yc)
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

	String packageName = HttpUtility::GetLastParameter(params, "package");
	String stageName = HttpUtility::GetLastParameter(params, "stage");

	if (!ConfigPackageUtility::ValidatePackageName(packageName))
		return HttpUtility::SendJsonError(response, params, 400, "Invalid package name '" + packageName + "'.");

	if (!ConfigPackageUtility::ValidateStageName(stageName))
		return HttpUtility::SendJsonError(response, params, 400, "Invalid stage name '" + stageName + "'.");

	std::vector<std::pair<String, bool> > paths = ConfigPackageUtility::GetFiles(packageName, stageName);

	String prefixPath = ConfigPackageUtility::GetPackageDir() + "/" + packageName + "/" + stageName + "/";

	auto generatorFunc = [&prefixPath](const std::pair<String, bool>& kv) -> Value {
		return new Dictionary{
			{ "type", kv.second ? "directory" : "file" },
			{ "name", kv.first.SubStr(prefixPath.GetLength()) },
		};
	};

	Dictionary::Ptr result = new Dictionary{{"results", new ValueGenerator{paths, generatorFunc}}};
	result->Freeze();

	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result, yc);
}

void ConfigStagesHandler::HandlePost(const HttpApiRequest& request, HttpApiResponse& response)
{
	namespace http = boost::beast::http;

	auto url = request.Url();
	auto user = request.User();
	auto params = request.Params();

	FilterUtility::CheckPermission(user, "config/modify");

	if (url->GetPath().size() >= 4)
		params->Set("package", url->GetPath()[3]);

	String packageName = HttpUtility::GetLastParameter(params, "package");

	if (!ConfigPackageUtility::ValidatePackageName(packageName))
		return HttpUtility::SendJsonError(response, params, 400, "Invalid package name '" + packageName + "'.");

	bool reload = true;

	if (params->Contains("reload"))
		reload = HttpUtility::GetLastParameter(params, "reload");

	bool activate = true;

	if (params->Contains("activate"))
		activate = HttpUtility::GetLastParameter(params, "activate");

	Dictionary::Ptr files = params->Get("files");

	String stageName;

	try {
		if (!files)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Parameter 'files' must be specified."));

		if (reload && !activate)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Parameter 'reload' must be false when 'activate' is false."));

		ConfigObjectsSharedLock configObjectsSharedLock(std::try_to_lock);
		if (!configObjectsSharedLock) {
			HttpUtility::SendJsonError(response, params, 503, "Icinga is reloading");
			return;
		}

		{
			std::lock_guard runningPackageUpdatesLock(l_RunningPackageUpdatesMutex);
			double currentReloadFailedTime = Application::GetLastReloadFailed();

			/**
			 * Once the m_RunningPackageUpdates flag is set, it typically remains set until the current worker process is
			 * terminated, in which case the new worker will have its own m_RunningPackageUpdates flag set to false.
			 * However, if the reload fails for any reason, the m_RunningPackageUpdates flag will remain set to true
			 * in the current worker process, which will prevent any further package updates from being processed until
			 * the next Icinga 2 restart.
			 *
			 * So, in order to prevent such a situation, we are additionally tracking the last time a reload failed
			 * and allow to bypass the m_RunningPackageUpdates flag only if the last reload failed time was changed
			 * since the previous request.
			 */
			if (l_RunningPackageUpdates && l_LastReloadFailedTime == currentReloadFailedTime) {
				return HttpUtility::SendJsonError(
					response, params, 423,
					"Conflicting request, there is already an ongoing package update in progress. Please try it again later."
				);
			}

			l_RunningPackageUpdates = true;
			l_LastReloadFailedTime = currentReloadFailedTime;
		}

		auto resetPackageUpdates (Shared<Defer>::Make([]() {
			std::lock_guard lock(l_RunningPackageUpdatesMutex);
			l_RunningPackageUpdates = false;
		}));

		std::unique_lock<std::mutex> lock(ConfigPackageUtility::GetStaticPackageMutex());

		stageName = ConfigPackageUtility::CreateStage(packageName, files);

		/* validate the config. on success, activate stage and reload */
		ConfigPackageUtility::AsyncTryActivateStage(packageName, stageName, activate, reload, resetPackageUpdates);
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

void ConfigStagesHandler::HandleDelete(const HttpApiRequest& request, HttpApiResponse& response)
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

	String packageName = HttpUtility::GetLastParameter(params, "package");
	String stageName = HttpUtility::GetLastParameter(params, "stage");

	if (!ConfigPackageUtility::ValidatePackageName(packageName))
		return HttpUtility::SendJsonError(response, params, 400, "Invalid package name '" + packageName + "'.");

	if (!ConfigPackageUtility::ValidateStageName(stageName))
		return HttpUtility::SendJsonError(response, params, 400, "Invalid stage name '" + stageName + "'.");

	ConfigObjectsSharedLock lock(std::try_to_lock);
	if (!lock) {
		HttpUtility::SendJsonError(response, params, 503, "Icinga is reloading");
		return;
	}

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
