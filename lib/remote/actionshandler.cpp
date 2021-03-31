/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/actionshandler.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "remote/apiaction.hpp"
#include "base/exception.hpp"
#include "base/logger.hpp"
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/actions", ActionsHandler);

bool ActionsHandler::HandleRequest(
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
	} else {
		FilterUtility::CheckPermission(user, permission);
		objs.emplace_back(nullptr);
	}

	ArrayData results;

	{
		Log msg (LogInformation, "ApiActionHandler");
		msg << "Running action " << actionName << " on " << objs.size() << " objects:";
		int limit = 10; // arbitrary limit to not blow up logs for filters matching many objects
		for (const ConfigObject::Ptr& obj : objs) {
			if (limit-- > 0) {
				msg << " '" << obj->GetName() << "'";
			} else {
				msg << " ...";
				break;
			}
		}
	}

	bool verbose = false;

	if (params)
		verbose = HttpUtility::GetLastParameter(params, "verbose");

	int limit = 10;

	for (const ConfigObject::Ptr& obj : objs) {
		try {
			Value result = action->Invoke(obj, params);
			if (result.IsObjectType<Dictionary>()) {
				Dictionary::Ptr dict = result;
				if (dict->Contains("code") && dict->Get("code") != 200) {
					if (limit-- > 0) {
						Log msg(LogInformation, "ApiActionHandler");
						msg << "Action " << actionName << " for '" << obj->GetName() << "' returned " << dict->Get("code");
						if (dict->Contains("status")) {
							msg << ": " << dict->Get("status");
						}
					}
				}
			}
			results.emplace_back(result);
		} catch (const std::exception& ex) {
			if (limit-- > 0) {
				Log(LogInformation, "ApiActionHandler") << "Action " << actionName << " for '" << obj->GetName()
					<< "' failed: " << DiagnosticInformation(ex, false);
			}

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

	for (const Dictionary::Ptr& res : results) {
		if (res->Contains("code") && res->Get("code") == 200) {
			statusCode = 200;
			break;
		}
	}

	response.result(statusCode);

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array(std::move(results)) }
	});

	HttpUtility::SendJsonBody(response, params, result);

	return true;
}
