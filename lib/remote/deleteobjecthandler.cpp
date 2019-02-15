/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/deleteobjecthandler.hpp"
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
	AsioTlsStream& stream,
	const ApiUser::Ptr& user,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	const Url::Ptr& url,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	const Dictionary::Ptr& params,
	boost::asio::yield_context& yc,
	bool& hasStartedStreaming
)
{
	namespace http = boost::beast::http;

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

	ArrayData results;

	bool success = true;

	for (const ConfigObject::Ptr& obj : objs) {
		int code;
		String status;
		Array::Ptr errors = new Array();
		Array::Ptr diagnosticInformation = new Array();

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

	HttpUtility::SendJsonBody(response, params, result);

	return true;
}

