/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/actionshandler.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "remote/apiaction.hpp"
#include "base/defer.hpp"
#include "base/exception.hpp"
#include "base/logger.hpp"
#include <set>

using namespace icinga;

thread_local ApiUser::Ptr ActionsHandler::AuthenticatedApiUser;

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

	Log(LogNotice, "ApiActionHandler")
		<< "Running action " << actionName;

	bool verbose = false;

	ActionsHandler::AuthenticatedApiUser = user;
	Defer a ([]() {
		ActionsHandler::AuthenticatedApiUser = nullptr;
	});

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
	std::set<int> okStatusCodes, nonOkStatusCodes;

	for (const Dictionary::Ptr& res : results) {
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

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array(std::move(results)) }
	});

	HttpUtility::SendJsonBody(response, params, result);

	return true;
}
