/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/variablequeryhandler.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "base/configtype.hpp"
#include "base/scriptglobal.hpp"
#include "base/logger.hpp"
#include "base/serializer.hpp"
#include "base/namespace.hpp"
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/variables", VariableQueryHandler);

class VariableTargetProvider final : public TargetProvider
{
public:
	DECLARE_PTR_TYPEDEFS(VariableTargetProvider);

	void FindTargets(const String& type,
		const std::function<void (const Value&)>& addTarget) const override
	{
		Namespace::Ptr globals = ScriptGlobal::GetGlobals();
		ObjectLock olock(globals);
		for (auto& [key, value] : globals) {
			/* We want wo avoid leaking the TicketSalt over the API, so we remove it here,
			 * as early as possible, so it isn't possible to abuse the fact that all of the
			 * global variables we return here later get checked against a user-provided
			 * filter expression that can cause its content to be printed in an error message
			 * or potentially access them otherwise.
			 */
			if (key == "TicketSalt") {
				continue;
			}

			addTarget(FilterUtility::GetTargetForVar(key, value.Val));
		}
	}

	Value GetTargetByName(const String& type, const String& name) const override
	{
		if (name == "TicketSalt") {
			BOOST_THROW_EXCEPTION(std::invalid_argument{"Access to TicketSalt via /v1/variables is not permitted."});
		}
		return FilterUtility::GetTargetForVar(name, ScriptGlobal::Get(name));
	}

	bool IsValidType(const String& type) const override
	{
		return type == "Variable";
	}

	String GetPluralName(const String& type) const override
	{
		return "variables";
	}
};

bool VariableQueryHandler::HandleRequest(
	const WaitGroup::Ptr&,
	const HttpApiRequest& request,
	HttpApiResponse& response,
	boost::asio::yield_context& yc
)
{
	namespace http = boost::beast::http;

	auto url = request.Url();
	auto user = request.User();
	auto params = request.Params();

	if (url->GetPath().size() > 3)
		return false;

	if (request.method() != http::verb::get)
		return false;

	QueryDescription qd;
	qd.Types.insert("Variable");
	qd.Permission = "variables";
	qd.Provider = new VariableTargetProvider();

	params->Set("type", "Variable");

	if (url->GetPath().size() >= 3)
		params->Set("variable", url->GetPath()[2]);

	std::vector<Value> objs;

	try {
		objs = FilterUtility::GetFilterTargets(qd, params, user, "variable");
	} catch (const std::exception& ex) {
		HttpUtility::SendJsonError(response, params, 404,
			"No variables found.",
			DiagnosticInformation(ex));
		return true;
	}

	ArrayData results;

	for (Dictionary::Ptr var : objs) {
		results.emplace_back(new Dictionary({
			{ "name", var->Get("name") },
			{ "type", var->Get("type") },
			{ "value", Serialize(var->Get("value"), 0) }
		}));
	}

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array(std::move(results)) }
	});

	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result);

	return true;
}
