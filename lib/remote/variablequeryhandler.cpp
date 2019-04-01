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
				addTarget(GetTargetForVar(kv.first, kv.second->Get()));
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

	for (const Dictionary::Ptr& var : objs) {
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

