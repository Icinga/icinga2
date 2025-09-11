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
#include <optional>
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
		HttpUtility::SendJsonError(response, params, 400, "Invalid type specified.");
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
		HttpUtility::SendJsonError(response, params, 404,
			"No objects found.",
			DiagnosticInformation(ex));
		return true;
	}

	bool cascade = HttpUtility::GetLastParameter(params, "cascade");
	bool verbose = HttpUtility::GetLastParameter(params, "verbose");

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

	auto generatorFunc = [&type, &waitGroup, &wgLock, cascade, verbose](
		const ConfigObject::Ptr& obj
	) -> std::optional<Value> {
		if (!waitGroup->IsLockable()) {
			if (wgLock) {
				wgLock.unlock();
			}

			return new Dictionary{
				{ "type", type->GetName() },
				{ "name", obj->GetName() },
				{ "code", 503 },
				{ "status", "Action skipped: Shutting down."}
			};
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
		} else {
			code = 200;
			status = "Object was deleted.";
		}

		Dictionary::Ptr result = new Dictionary{
			{ "type", type->GetName() },
			{ "name", obj->GetName() },
			{ "code", code },
			{ "status", status },
			{ "errors", errors }
		};

		if (verbose)
			result->Set("diagnostic_information", diagnosticInformation);

		return result;
	};

	Dictionary::Ptr result = new Dictionary{{"results", new ValueGenerator{objs, generatorFunc}}};
	result->Freeze();

	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result, yc);

	return true;
}
