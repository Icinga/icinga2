/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/configstageshandler.hpp"
#include "remote/configobjectslock.hpp"
#include "remote/configpackageutility.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "base/application.hpp"
#include "base/defer.hpp"
#include "base/exception.hpp"

using namespace icinga;

REGISTER_URLHANDLER("/v1/config/stages", ConfigStagesHandler);

std::atomic<ConfigStagesHandler::StagesUpdateState> ConfigStagesHandler::m_RunningPackageUpdates (Idle);
// This variable is used to track the last time a reload request was made via this handler.
// It doesn't need to be thread-safe, as it will never be accessed concurrently.
static double l_LastReloadRequestTime = 0.0;

bool ConfigStagesHandler::HandleRequest(
	AsioTlsStream& stream,
	const ApiUser::Ptr& user,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	const Url::Ptr& url,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	const Dictionary::Ptr& params,
	boost::asio::yield_context& yc,
	HttpServerConnection& server
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

	if (!ConfigPackageUtility::ValidatePackageName(packageName))
		return HttpUtility::SendJsonError(response, params, 400, "Invalid package name '" + packageName + "'.");

	if (!ConfigPackageUtility::ValidateStageName(stageName))
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

		auto now = Utility::GetTime();
		if (auto state = m_RunningPackageUpdates.exchange(Running); state == Running) {
			return HttpUtility::SendJsonError(response, params, 423,
				"Conflicting request, there is already an ongoing package update in progress. Please try it again later.");
		} else if (state == ReloadRequested) {
			// Try to acquire a shared lock on the ConfigObjectsSharedLock to ensure that we are
			// not in the middle of a reload process.
			ConfigObjectsSharedLock objectsSharedLock(std::try_to_lock);
			// If we weren't able to acquire the shared lock, then it's obviously because there's a reload in progress,
			// so we've to reject the request just like all the other handlers do. However, if we successfully acquired
			// the lock, then we've to heuristically perform a check when the last reload request was made via this
			// handler. The magic number 2.7 is composed of the 2.5 seconds that the worker's event loop waits before
			// its next tick, i.e. that's the worst case scenario how long it takes to react on the reload request,
			// plus 0.2 seconds that the umbrella process would theoretically take to react on the SIGHUP signal from
			// the worker. So, if the last reload request was made within the last 2.7 seconds, then we can assume that
			// the reload is still in progress, and we have to reject the request.
			if (!objectsSharedLock || l_LastReloadRequestTime >= now - 2.7) {
				return HttpUtility::SendJsonError(response, params, 503, "Icinga is reloading.");
			}
		}

		auto refreshPackageUpdates (Shared<Defer>::Make([reload, packageName, stageName]() {
			l_LastReloadRequestTime = Utility::GetTime();

			auto activeStage = ConfigPackageUtility::GetActiveStage(packageName);
			// If the currently active stage is the one we just created, then we can be sure that the config
			// validation was successful and that we (if reload is true) have requested the worker to reload.
			m_RunningPackageUpdates.store(reload && activeStage == stageName ? ReloadRequested : Idle);
		}));

		std::unique_lock<std::mutex> lock(ConfigPackageUtility::GetStaticPackageMutex());

		stageName = ConfigPackageUtility::CreateStage(packageName, files);

		/* validate the config. on success, activate stage and reload */
		ConfigPackageUtility::AsyncTryActivateStage(packageName, stageName, activate, reload, refreshPackageUpdates);
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

	if (!ConfigPackageUtility::ValidatePackageName(packageName))
		return HttpUtility::SendJsonError(response, params, 400, "Invalid package name '" + packageName + "'.");

	if (!ConfigPackageUtility::ValidateStageName(stageName))
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
