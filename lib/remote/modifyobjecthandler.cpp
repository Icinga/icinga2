/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/modifyobjecthandler.hpp"
#include "remote/configobjectslock.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "remote/apiaction.hpp"
#include "base/exception.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/objects", ModifyObjectHandler);

bool ModifyObjectHandler::HandleRequest(
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

	if (request.method() != http::verb::post)
		return false;

	Type::Ptr type = FilterUtility::TypeFromPluralName(url->GetPath()[2]);

	if (!type) {
		response.SendJsonError(params, 400, "Invalid type specified.");
		return true;
	}

	QueryDescription qd;
	qd.Types.insert(type->GetName());
	qd.Permission = "objects/modify/" + type->GetName();

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

	Value attrsVal = params->Get("attrs");

	if (attrsVal.GetReflectionType() != Dictionary::TypeInstance && attrsVal.GetType() != ValueEmpty) {
		response.SendJsonError(params, 400,
			"Invalid type for 'attrs' attribute specified. Dictionary type is required."
			"Or is this a POST query and you missed adding a 'X-HTTP-Method-Override: GET' header?");
		return true;
	}

	Dictionary::Ptr attrs = attrsVal;

	Value restoreAttrsVal = params->Get("restore_attrs");

	if (restoreAttrsVal.GetReflectionType() != Array::TypeInstance && restoreAttrsVal.GetType() != ValueEmpty) {
		response.SendJsonError(params, 400,
			"Invalid type for 'restore_attrs' attribute specified. Array type is required.");
		return true;
	}

	Array::Ptr restoreAttrs = restoreAttrsVal;

	if (!(attrs || restoreAttrs)) {
		response.SendJsonError(params, 400,
			"Missing both 'attrs' and 'restore_attrs'. "
			"Or is this a POST query and you missed adding a 'X-HTTP-Method-Override: GET' header?");
		return true;
	}

	bool verbose = request.IsVerbose();

	ConfigObjectsSharedLock lock (std::try_to_lock);

	if (!lock) {
		response.SendJsonError(params, 503, "Icinga is reloading");
		return true;
	}

	ArrayData results;

	std::shared_lock wgLock{*waitGroup, std::try_to_lock};
	if (!wgLock) {
		response.SendJsonError(params, 503, "Shutting down.");
		return true;
	}

	for (ConfigObject::Ptr obj : objs) {
		Dictionary::Ptr result1 = new Dictionary();

		result1->Set("type", type->GetName());
		result1->Set("name", obj->GetName());

		if (!waitGroup->IsLockable()) {
			if (wgLock) {
				wgLock.unlock();
			}

			result1->Set("code", 503);
			result1->Set("status", "Action skipped: Shutting down.");

			results.emplace_back(std::move(result1));

			continue;
		}

		String key;

		// Lock the object name of the given type to prevent from being modified/deleted concurrently.
		ObjectNameLock objectNameLock(type, obj->GetName());

		try {
			if (restoreAttrs) {
				ObjectLock oLock (restoreAttrs);

				for (auto& attr : restoreAttrs) {
					key = attr;
					obj->RestoreAttribute(key);
				}
			}
		} catch (const std::exception& ex) {
			result1->Set("code", 500);
			result1->Set("status", "Attribute '" + key + "' could not be restored: " + DiagnosticInformation(ex, false));

			if (verbose)
				result1->Set("diagnostic_information", DiagnosticInformation(ex));

			results.push_back(std::move(result1));
			continue;
		}

		try {
			if (attrs) {
				ObjectLock olock(attrs);
				for (const Dictionary::Pair& kv : attrs) {
					key = kv.first;
					obj->ModifyAttribute(kv.first, kv.second);
				}
			}
		} catch (const std::exception& ex) {
			result1->Set("code", 500);
			result1->Set("status", "Attribute '" + key + "' could not be set: " + DiagnosticInformation(ex, false));

			if (verbose)
				result1->Set("diagnostic_information", DiagnosticInformation(ex));

			results.push_back(std::move(result1));
			continue;
		}

		result1->Set("code", 200);
		result1->Set("status", "Attributes updated.");

		results.push_back(std::move(result1));
	}

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array(std::move(results)) }
	});

	response.result(http::status::ok);
	response.SendJsonBody(result, request.IsPretty());

	return true;
}
