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

	ArrayData results;

	for (const ConfigObject::Ptr& obj : objs) {
		Dictionary::Ptr result1 = new Dictionary();

		result1->Set("type", type->GetName());
		result1->Set("name", obj->GetName());

		String key;

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
	HttpUtility::SendJsonBody(response, params, result);

	return true;
}
