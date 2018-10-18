/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

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

bool VariableQueryHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response, const Dictionary::Ptr& params)
{
	if (request.RequestUrl->GetPath().size() > 3)
		return false;

	if (request.RequestMethod != "GET")
		return false;

	QueryDescription qd;
	qd.Types.insert("Variable");
	qd.Permission = "variables";
	qd.Provider = new VariableTargetProvider();

	params->Set("type", "Variable");

	if (request.RequestUrl->GetPath().size() >= 3)
		params->Set("variable", request.RequestUrl->GetPath()[2]);

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

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, params, result);

	return true;
}

