/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/modifyobjecthandler.hpp"
#include "remote/configobjectslock.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "remote/apiaction.hpp"
#include "base/exception.hpp"
#include "base/generator.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include <optional>
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
		HttpUtility::SendJsonError(response, params, 400, "Invalid type specified.");
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
		HttpUtility::SendJsonError(response, params, 404,
			"No objects found.",
			DiagnosticInformation(ex));
		return true;
	}

	Value attrsVal = params->Get("attrs");

	if (attrsVal.GetReflectionType() != Dictionary::TypeInstance && attrsVal.GetType() != ValueEmpty) {
		HttpUtility::SendJsonError(response, params, 400,
			"Invalid type for 'attrs' attribute specified. Dictionary type is required."
			"Or is this a POST query and you missed adding a 'X-HTTP-Method-Override: GET' header?");
		return true;
	}

	Dictionary::Ptr attrs = attrsVal;

	Value restoreAttrsVal = params->Get("restore_attrs");

	if (restoreAttrsVal.GetReflectionType() != Array::TypeInstance && restoreAttrsVal.GetType() != ValueEmpty) {
		HttpUtility::SendJsonError(response, params, 400,
			"Invalid type for 'restore_attrs' attribute specified. Array type is required.");
		return true;
	}

	Array::Ptr restoreAttrs = restoreAttrsVal;

	if (!(attrs || restoreAttrs)) {
		HttpUtility::SendJsonError(response, params, 400,
			"Missing both 'attrs' and 'restore_attrs'. "
			"Or is this a POST query and you missed adding a 'X-HTTP-Method-Override: GET' header?");
		return true;
	}

	bool verbose = false;

	if (params)
		verbose = HttpUtility::GetLastParameter(params, "verbose");

	ConfigObjectsSharedLock lock (std::try_to_lock);

	if (!lock) {
		HttpUtility::SendJsonError(response, params, 503, "Icinga is reloading");
		return true;
	}

	std::shared_lock wgLock{*waitGroup, std::try_to_lock};
	if (!wgLock) {
		HttpUtility::SendJsonError(response, params, 503, "Shutting down.");
		return true;
	}

	auto generatorFunc = [&waitGroup, &wgLock, &type, &attrs, &restoreAttrs, verbose](
		const ConfigObject::Ptr& obj
	) -> std::optional<Value> {
		Dictionary::Ptr result = new Dictionary();

		result->Set("type", type->GetName());
		result->Set("name", obj->GetName());

		if (!waitGroup->IsLockable()) {
			if (wgLock) {
				wgLock.unlock();
			}

			result->Set("code", 503);
			result->Set("status", "Action skipped: Shutting down.");
			return result;
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
			result->Set("code", 500);
			result->Set("status", "Attribute '" + key + "' could not be restored: " + DiagnosticInformation(ex, false));

			if (verbose)
				result->Set("diagnostic_information", DiagnosticInformation(ex));

			return result;
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
			result->Set("code", 500);
			result->Set("status", "Attribute '" + key + "' could not be set: " + DiagnosticInformation(ex, false));

			if (verbose)
				result->Set("diagnostic_information", DiagnosticInformation(ex));

			return result;
		}

		result->Set("code", 200);
		result->Set("status", "Attributes updated.");
		return result;
	};

	Dictionary::Ptr result = new Dictionary{{"results", new ValueGenerator{objs, generatorFunc}}};
	result->Freeze();

	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result, yc);

	return true;
}
