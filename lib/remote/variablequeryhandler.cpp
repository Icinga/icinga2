/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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
#include <boost/algorithm/string.hpp>
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/variables", VariableQueryHandler);

class VariableTargetProvider : public TargetProvider
{
public:
	DECLARE_PTR_TYPEDEFS(VariableTargetProvider);

	static Dictionary::Ptr GetTargetForVar(const String& name, const Value& value)
	{
		Dictionary::Ptr target = new Dictionary();
		target->Set("name", name);
		target->Set("type", value.GetReflectionType()->GetName());
		target->Set("value", value);
		return target;
	}

	virtual void FindTargets(const String& type,
	    const std::function<void (const Value&)>& addTarget) const override
	{
		{
			Dictionary::Ptr globals = ScriptGlobal::GetGlobals();
			ObjectLock olock(globals);
			for (const Dictionary::Pair& kv : globals) {
				addTarget(GetTargetForVar(kv.first, kv.second));
			}
		}
	}

	virtual Value GetTargetByName(const String& type, const String& name) const override
	{
		return GetTargetForVar(name, ScriptGlobal::Get(name));
	}

	virtual bool IsValidType(const String& type) const override
	{
		return type == "Variable";
	}

	virtual String GetPluralName(const String& type) const override
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
		HttpUtility::SendJsonError(response, 404,
		    "No variables found.",
		    HttpUtility::GetLastParameter(params, "verboseErrors") ? DiagnosticInformation(ex) : "");
		return true;
	}

	Array::Ptr results = new Array();

	for (const Dictionary::Ptr& var : objs) {
		Dictionary::Ptr result1 = new Dictionary();
		results->Add(result1);

		Dictionary::Ptr resultAttrs = new Dictionary();
		result1->Set("name", var->Get("name"));
		result1->Set("type", var->Get("type"));
		result1->Set("value", Serialize(var->Get("value"), 0));
	}

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);

	return true;
}

