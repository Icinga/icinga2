/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/createobjecthandler.hpp"
#include "remote/configobjectslock.hpp"
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
	const WaitGroup::Ptr& waitGroup,
	AsioTlsStream& stream,
	HttpRequest& request,
	HttpResponse& response,
	boost::asio::yield_context& yc,
	HttpServerConnection& server
)
{
	namespace http = boost::beast::http;

	if (request.Url()->GetPath().size() != 4)
		return false;

	if (request.method() != http::verb::put)
		return false;

	Type::Ptr type = FilterUtility::TypeFromPluralName(request.Url()->GetPath()[2]);

	if (!type) {
		response.SendJsonError(request.Params(), 400, "Invalid type specified.");
		return true;
	}

	FilterUtility::CheckPermission(request.User(), "objects/create/" + type->GetName());

	String name = request.Url()->GetPath()[3];
	Array::Ptr templates = request.Params()->Get("templates");
	Dictionary::Ptr attrs = request.Params()->Get("attrs");

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

	if (request.Params()->Contains("ignore_on_error"))
		ignoreOnError = request.GetLastParameter("ignore_on_error");

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array({ result1 }) }
	});

	String config;

	bool verbose = false;

	if (request.Params())
		verbose = request.GetLastParameter("verbose");

	ConfigObjectsSharedLock lock (std::try_to_lock);

	if (!lock) {
		response.SendJsonError(request.Params(), 503, "Icinga is reloading");
		return true;
	}

	std::shared_lock wgLock{*waitGroup, std::try_to_lock};
	if (!wgLock) {
		response.SendJsonError(request.Params(), 503, "Shutting down.");
		return true;
	}

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
		response.SendJsonBody(result, request.IsPretty());

		return true;
	}

	// Lock the object name of the given type to prevent from being created concurrently.
	ObjectNameLock objectNameLock(type, name);

	if (!ConfigObjectUtility::CreateObject(type, name, config, errors, diagnosticInformation)) {
		result1->Set("errors", errors);
		result1->Set("code", 500);
		result1->Set("status", "Object could not be created.");

		if (verbose)
			result1->Set("diagnostic_information", diagnosticInformation);

		response.result(http::status::internal_server_error);
		response.SendJsonBody(result, request.IsPretty());

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
	response.SendJsonBody(result, request.IsPretty());

	return true;
}
