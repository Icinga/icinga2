/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "remote/actionshandler.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "remote/apiaction.hpp"
#include "base/exception.hpp"
#include "base/logger.hpp"
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/actions", ActionsHandler);

bool ActionsHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response, const Dictionary::Ptr& params)
{
	if (request.RequestUrl->GetPath().size() != 3)
		return false;

	if (request.RequestMethod != "POST")
		return false;

	String actionName = request.RequestUrl->GetPath()[2];

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

	for (const ConfigObject::Ptr& obj : objs) {
		try {
			results.emplace_back(action->Invoke(obj, params));
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

	int statusCode = 500;
	String statusMessage = "No action executed successfully";

	for (const Dictionary::Ptr& res : results) {
		if (res->Contains("code") && res->Get("code") == 200) {
			statusCode = 200;
			statusMessage = "OK";
			break;
		}
	}

	response.SetStatus(statusCode, statusMessage);

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array(std::move(results)) }
	});

	HttpUtility::SendJsonBody(response, params, result);

	return true;
}
