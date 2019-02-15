/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/statushandler.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "base/serializer.hpp"
#include "base/statsfunction.hpp"
#include "base/namespace.hpp"

using namespace icinga;

REGISTER_URLHANDLER("/v1/status", StatusHandler);

class StatusTargetProvider final : public TargetProvider
{
public:
	DECLARE_PTR_TYPEDEFS(StatusTargetProvider);

	void FindTargets(const String& type,
		const std::function<void (const Value&)>& addTarget) const override
	{
		Namespace::Ptr statsFunctions = ScriptGlobal::Get("StatsFunctions", &Empty);

		if (statsFunctions) {
			ObjectLock olock(statsFunctions);

			for (const Namespace::Pair& kv : statsFunctions)
				addTarget(GetTargetByName("Status", kv.first));
		}
	}

	Value GetTargetByName(const String& type, const String& name) const override
	{
		Namespace::Ptr statsFunctions = ScriptGlobal::Get("StatsFunctions", &Empty);

		if (!statsFunctions)
			BOOST_THROW_EXCEPTION(std::invalid_argument("No status functions are available."));

		Value vfunc;

		if (!statsFunctions->Get(name, &vfunc))
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid status function name."));

		Function::Ptr func = vfunc;

		if (!func)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid status function name."));

		Dictionary::Ptr status = new Dictionary();
		Array::Ptr perfdata = new Array();
		func->Invoke({ status, perfdata });

		return new Dictionary({
			{ "name", name },
			{ "status", status },
			{ "perfdata", Serialize(perfdata, FAState) }
		});
	}

	bool IsValidType(const String& type) const override
	{
		return type == "Status";
	}

	String GetPluralName(const String& type) const override
	{
		return "statuses";
	}
};

bool StatusHandler::HandleRequest(
	const ApiUser::Ptr& user,
	boost::beast::http::request<boost::beast::http::string_body>& request,
	const Url::Ptr& url,
	boost::beast::http::response<boost::beast::http::string_body>& response,
	const Dictionary::Ptr& params
)
{
	namespace http = boost::beast::http;

	if (url->GetPath().size() > 3)
		return false;

	if (request.method() != http::verb::get)
		return false;

	QueryDescription qd;
	qd.Types.insert("Status");
	qd.Provider = new StatusTargetProvider();
	qd.Permission = "status/query";

	params->Set("type", "Status");

	if (url->GetPath().size() >= 3)
		params->Set("status", url->GetPath()[2]);

	std::vector<Value> objs;

	try {
		objs = FilterUtility::GetFilterTargets(qd, params, user);
	} catch (const std::exception& ex) {
		HttpUtility::SendJsonError(response, params, 404,
			"No objects found.",
			DiagnosticInformation(ex));
		return true;
	}

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array(std::move(objs)) }
	});

	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result);

	return true;
}

