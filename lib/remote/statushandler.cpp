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

bool StatusHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response, const Dictionary::Ptr& params)
{
	if (request.RequestUrl->GetPath().size() > 3)
		return false;

	if (request.RequestMethod != "GET")
		return false;

	QueryDescription qd;
	qd.Types.insert("Status");
	qd.Provider = new StatusTargetProvider();
	qd.Permission = "status/query";

	params->Set("type", "Status");

	if (request.RequestUrl->GetPath().size() >= 3)
		params->Set("status", request.RequestUrl->GetPath()[2]);

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

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, params, result);

	return true;
}

