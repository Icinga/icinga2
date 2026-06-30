// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "remote/actionshandler.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "remote/apiaction.hpp"
#include "base/defer.hpp"
#include "base/exception.hpp"
#include "base/logger.hpp"
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/actions", ActionsHandler);

bool ActionsHandler::HandleRequest(
	const WaitGroup::Ptr& waitGroup,
	const HttpApiRequest& request,
	HttpApiResponse& response,
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

	ArrayData results;

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

	for (ConfigObject::Ptr obj : objs) {
		if (!waitGroup->IsLockable()) {
			if (wgLock) {
				wgLock.unlock();
			}

			results.emplace_back(new Dictionary({
				{ "type", obj->GetReflectionType()->GetName() },
				{ "name", obj->GetName() },
				{ "code", 503 },
				{ "status", "Action skipped: Shutting down."}
			}));

			continue;
		}

		try {
			results.emplace_back(action->Invoke(obj, user, params));
		} catch (const std::exception& ex) {
			Dictionary::Ptr fail = new Dictionary({
				{ "code", 500 },
				{ "status", "Action execution failed: '" + DiagnosticInformation(ex, false) + "'." }
			});

			/* Exception for actions. Normally we would handle this inside SendJsonError(). */
			if (verbose)
				fail->Set("diagnostic_information", DiagnosticInformation(ex));

			results.emplace_back(std::move(fail));
		}
	}

	if (wgLock.owns_lock()) {
		// Unlock before starting to stream the response, so that we don't block the shutdown process.
		wgLock.unlock();
	}

	int statusCode = 500;
	std::set<int> okStatusCodes, nonOkStatusCodes;

	for (Dictionary::Ptr res : results) {
		if (!res->Contains("code")) {
			continue;
		}

		auto code = res->Get("code");

		if (code >= 200 && code <= 299) {
			okStatusCodes.insert(code);
		} else {
			nonOkStatusCodes.insert(code);
		}
	}

	size_t okSize = okStatusCodes.size();
	size_t nonOkSize = nonOkStatusCodes.size();

	if (okSize == 1u && nonOkSize == 0u) {
		statusCode = *okStatusCodes.begin();
	} else if (nonOkSize == 1u) {
		statusCode = *nonOkStatusCodes.begin();
	} else if (okSize >= 2u && nonOkSize == 0u) {
		statusCode = 200;
	}

	response.result(statusCode);

	Array::Ptr resultArray{new Array{std::move(results)}};
	resultArray->Freeze(); // Allows the JSON encoder to yield while encoding the array.

	Dictionary::Ptr result = new Dictionary{{"results", std::move(resultArray)}};
	result->Freeze();

	HttpUtility::SendJsonBody(response, params, result, yc);

	return true;
}
