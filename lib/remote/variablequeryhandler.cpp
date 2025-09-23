/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/variablequeryhandler.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "base/configtype.hpp"
#include "base/generator.hpp"
#include "base/scriptglobal.hpp"
#include "base/logger.hpp"
#include "base/serializer.hpp"
#include "base/namespace.hpp"
#include <optional>
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/variables", VariableQueryHandler);

class VariableTargetProvider final : public TargetProvider
{
public:
	DECLARE_PTR_TYPEDEFS(VariableTargetProvider);

	static Dictionary::Ptr GetTargetForVar(const String& name, const Value& value)
	{
		return new Dictionary({
			{ "name", name },
			{ "type", value.GetReflectionType()->GetName() },
			{ "value", value }
		});
	}

	void FindTargets(const String& type,
		const std::function<void (const Value&)>& addTarget) const override
	{
		{
			Namespace::Ptr globals = ScriptGlobal::GetGlobals();
			ObjectLock olock(globals);
			for (const Namespace::Pair& kv : globals) {
				addTarget(GetTargetForVar(kv.first, kv.second.Val));
			}
		}
	}

	Value GetTargetByName(const String& type, const String& name) const override
	{
		return GetTargetForVar(name, ScriptGlobal::Get(name));
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
	const HttpRequest& request,
	HttpResponse& response,
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

	auto generatorFunc = [](const Dictionary::Ptr& var) -> std::optional<Value> {
		if (var->Get("name") == "TicketSalt") {
			// Returning a nullopt here will cause the generator to skip this entry.
			// Meaning, when this variable wasn't the last element in the container,
			// the generator will directly call us again with the next element.
			return std::nullopt;
		}

		return new Dictionary{
			{ "name", var->Get("name") },
			{ "type", var->Get("type") },
			{ "value", Serialize(var->Get("value"), 0) }
		};
	};

	Dictionary::Ptr result = new Dictionary{{"results", new ValueGenerator{objs, generatorFunc}}};
	result->Freeze();

	response.result(http::status::ok);
	HttpUtility::SendJsonBody(response, params, result, yc);

	return true;
}
