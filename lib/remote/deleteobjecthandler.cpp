/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/deleteobjecthandler.hpp"
#include "remote/configobjectslock.hpp"
#include "remote/configobjectutility.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "remote/apiaction.hpp"
#include "config/configitem.hpp"
#include "base/exception.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/objects", DeleteObjectHandler);

bool DeleteObjectHandler::HandleRequest(
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

	if (url->GetPath().size() < 3 || url->GetPath().size() > 4)
		return false;

	if (request.method() != http::verb::delete_)
		return false;

	Type::Ptr type = FilterUtility::TypeFromPluralName(url->GetPath()[2]);

	if (!type) {
		response.SendJsonError(params, 400, "Invalid type specified.");
		return true;
	}

	QueryDescription qd;
	qd.Types.insert(type->GetName());
	qd.Permission = "objects/delete/" + type->GetName();

	params->Set("type", type->GetName());

	if (url->GetPath().size() >= 4) {
		String attr = type->GetName();
		boost::algorithm::to_lower(attr);
		params->Set(attr, url->GetPath()[3]);
	}

	std::vector<Value> objs;

	try {
		objs = FilterUtility::GetFilterTargets(qd, params, user);
	} catch (const std::exception& ex) {
		response.SendJsonError(params, 404,
			"No objects found.",
			DiagnosticInformation(ex));
		return true;
	}

	bool cascade = request.GetLastParameter("cascade");
	bool verbose = request.IsVerbose();

	ConfigObjectsSharedLock lock (std::try_to_lock);

	if (!lock) {
		response.SendJsonError(params, 503, "Icinga is reloading");
		return true;
	}

	ArrayData results;

	bool success = true;

	std::shared_lock wgLock{*waitGroup, std::try_to_lock};
	if (!wgLock) {
		response.SendJsonError(params, 503, "Shutting down.");
		return true;
	}

	for (ConfigObject::Ptr obj : objs) {
		if (!waitGroup->IsLockable()) {
			if (wgLock) {
				wgLock.unlock();
			}

			results.emplace_back(new Dictionary({
				{ "type", type->GetName() },
				{ "name", obj->GetName() },
				{ "code", 503 },
				{ "status", "Action skipped: Shutting down."}
			}));

			success = false;

			continue;
		}

		int code;
		String status;
		Array::Ptr errors = new Array();
		Array::Ptr diagnosticInformation = new Array();

		// Lock the object name of the given type to prevent from being modified/deleted concurrently.
		ObjectNameLock objectNameLock(type, obj->GetName());

		if (!ConfigObjectUtility::DeleteObject(obj, cascade, errors, diagnosticInformation)) {
			code = 500;
			status = "Object could not be deleted.";
			success = false;
		} else {
			code = 200;
			status = "Object was deleted.";
		}

		Dictionary::Ptr result = new Dictionary({
			{ "type", type->GetName() },
			{ "name", obj->GetName() },
			{ "code", code },
			{ "status", status },
			{ "errors", errors }
		});

		if (verbose)
			result->Set("diagnostic_information", diagnosticInformation);

		results.push_back(result);
	}

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array(std::move(results)) }
	});

	if (!success)
		response.result(http::status::internal_server_error);
	else
		response.result(http::status::ok);

	response.SendJsonBody(result, request.IsPretty());

	return true;
}
