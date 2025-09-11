/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/actionshandler.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "remote/apiaction.hpp"
#include "base/defer.hpp"
#include "base/exception.hpp"
#include "base/generator.hpp"
#include "base/logger.hpp"
#include <optional>
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/actions", ActionsHandler);

bool ActionsHandler::HandleRequest(
	const WaitGroup::Ptr& waitGroup,
	const HttpRequest& request,
	HttpResponse& response,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;
	auto url = request.Url();
	auto user = request.User();
	auto params = request.Params();

	if (url->GetPath().size() != 3)
		return false;

	if (request.method() != http::verb::post)
		return false;

	String actionName = url->GetPath()[2];

	ApiAction::Ptr action = ApiAction::GetByName(actionName);

	if (!action) {
		HttpUtility::SendJsonError(response, params, 404, "Action '" + actionName + "' does not exist.");
		return true;
	}

	QueryDescription qd;

	const std::vector<String>& types = action->GetTypes();
	std::vector<Value> objs;

	String permission = "actions/" + actionName;

	if (!types.empty()) {
		qd.Types = std::set<String>(types.begin(), types.end());
		qd.Permission = permission;

		try {
			objs = FilterUtility::GetFilterTargets(qd, params, user);
		} catch (const std::exception& ex) {
			HttpUtility::SendJsonError(response, params, 404,
				"No objects found.",
				DiagnosticInformation(ex));
			return true;
		}

		if (objs.empty()) {
			HttpUtility::SendJsonError(response, params, 404,
				"No objects found.");
			return true;
		}
	} else {
		FilterUtility::CheckPermission(user, permission);
		objs.emplace_back(nullptr);
	}

	Log(LogNotice, "ApiActionHandler")
		<< "Running action " << actionName;

	bool verbose = false;
	if (params)
		verbose = HttpUtility::GetLastParameter(params, "verbose");

	std::shared_lock wgLock{*waitGroup, std::try_to_lock};
	if (!wgLock) {
		HttpUtility::SendJsonError(response, params, 503, "Shutting down.");
		return true;
	}

	auto generatorFunc = [&action, &user, &params, &waitGroup, &wgLock, verbose](
		const ConfigObject::Ptr& obj
	) -> std::optional<Value> {
		if (!waitGroup->IsLockable()) {
			if (wgLock) {
				wgLock.unlock();
			}

			return new Dictionary{
				{ "type", obj->GetReflectionType()->GetName() },
				{ "name", obj->GetName() },
				{ "code", 503 },
				{ "status", "Action skipped: Shutting down."}
			};
		}

		try {
			return action->Invoke(obj, user, params);
		} catch (const std::exception& ex) {
			Dictionary::Ptr fail = new Dictionary{
				{ "code", 500 },
				{ "status", "Action execution failed: '" + DiagnosticInformation(ex, false) + "'." }
			};

			/* Exception for actions. Normally we would handle this inside SendJsonError(). */
			if (verbose)
				fail->Set("diagnostic_information", DiagnosticInformation(ex));

			return fail;
		}
	};

	Dictionary::Ptr result = new Dictionary{{"results", new ValueGenerator{objs, generatorFunc}}};
	result->Freeze();

	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result, yc);

	return true;
}
