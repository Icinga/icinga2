/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/createobjecthandler.hpp"
#include "remote/configobjectutility.hpp"
#include "remote/httputility.hpp"
#include "remote/jsonrpcconnection.hpp"
#include "remote/filterutility.hpp"
#include "remote/apiaction.hpp"
#include "remote/zone.hpp"
#include "base/configtype.hpp"
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/objects", CreateObjectHandler);

bool CreateObjectHandler::HandleRequest(
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

	if (url->GetPath().size() != 4)
		return false;

	if (request.method() != http::verb::put)
		return false;

	Type::Ptr type = FilterUtility::TypeFromPluralName(url->GetPath()[2]);

	if (!type) {
		HttpUtility::SendJsonError(response, params, 400, "Invalid type specified.");
		return true;
	}

	FilterUtility::CheckPermission(user, "objects/create/" + type->GetName());

	String name = url->GetPath()[3];
	Array::Ptr templates = params->Get("templates");
	Dictionary::Ptr attrs = params->Get("attrs");

	/* Put created objects into the local zone if not explicitly defined.
	 * This allows additional zone members to sync the
	 * configuration at some later point.
	 */
	Zone::Ptr localZone = Zone::GetLocalZone();
	String localZoneName;

	if (localZone) {
		localZoneName = localZone->GetName();

		if (!attrs) {
			attrs = new Dictionary({
				{ "zone", localZoneName }
			});
		} else if (!attrs->Contains("zone")) {
			attrs->Set("zone", localZoneName);
		}
	}

	/* Sanity checks for unique groups array. */
	if (attrs->Contains("groups")) {
		Array::Ptr groups = attrs->Get("groups");

		if (groups)
			attrs->Set("groups", groups->Unique());
	}

	Dictionary::Ptr result1 = new Dictionary();
	String status;
	Array::Ptr errors = new Array();
	Array::Ptr diagnosticInformation = new Array();

	bool ignoreOnError = false;

	if (params->Contains("ignore_on_error"))
		ignoreOnError = HttpUtility::GetLastParameter(params, "ignore_on_error");

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array({ result1 }) }
	});

	String config;

	bool verbose = false;

	if (params)
		verbose = HttpUtility::GetLastParameter(params, "verbose");

	/* Object creation can cause multiple errors and optionally diagnostic information.
	 * We can't use SendJsonError() here.
	 */
	try {
		config = ConfigObjectUtility::CreateObjectConfig(type, name, ignoreOnError, templates, attrs);
	} catch (const std::exception& ex) {
		errors->Add(DiagnosticInformation(ex, false));
		diagnosticInformation->Add(DiagnosticInformation(ex));

		if (verbose)
			result1->Set("diagnostic_information", diagnosticInformation);

		result1->Set("errors", errors);
		result1->Set("code", 500);
		result1->Set("status", "Object could not be created.");

		response.result(http::status::internal_server_error);
		HttpUtility::SendJsonBody(response, params, result);

		return true;
	}

	if (!ConfigObjectUtility::CreateObject(type, name, config, errors, diagnosticInformation)) {
		result1->Set("errors", errors);
		result1->Set("code", 500);
		result1->Set("status", "Object could not be created.");

		if (verbose)
			result1->Set("diagnostic_information", diagnosticInformation);

		response.result(http::status::internal_server_error);
		HttpUtility::SendJsonBody(response, params, result);

		return true;
	}

	auto *ctype = dynamic_cast<ConfigType *>(type.get());
	ConfigObject::Ptr obj = ctype->GetObject(name);

	result1->Set("code", 200);

	if (obj)
		result1->Set("status", "Object was created");
	else if (!obj && ignoreOnError)
		result1->Set("status", "Object was not created but 'ignore_on_error' was set to true");

	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result);

	return true;
}
