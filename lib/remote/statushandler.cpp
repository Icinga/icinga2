/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

using namespace icinga;

REGISTER_URLHANDLER("/v1/status", StatusHandler);

class StatusTargetProvider : public TargetProvider
{
public:
	DECLARE_PTR_TYPEDEFS(StatusTargetProvider);

	virtual void FindTargets(const String& type,
	    const boost::function<void (const Value&)>& addTarget) const override
	{
		typedef std::pair<String, StatsFunction::Ptr> kv_pair;
		BOOST_FOREACH(const kv_pair& kv, StatsFunctionRegistry::GetInstance()->GetItems()) {
			addTarget(GetTargetByName("Status", kv.first));
		}
	}

	virtual Value GetTargetByName(const String& type, const String& name) const override
	{
		StatsFunction::Ptr func = StatsFunctionRegistry::GetInstance()->GetItem(name);

		Dictionary::Ptr result = new Dictionary();

		Dictionary::Ptr status = new Dictionary();
		Array::Ptr perfdata = new Array();
		func->Invoke(status, perfdata);

		result->Set("name", name);
		result->Set("status", status);
		result->Set("perfdata", Serialize(perfdata, FAState));

		return result;
	}

	virtual bool IsValidType(const String& type) const override
	{
		return type == "Status";
	}

	virtual String GetPluralName(const String& type) const override
	{
		return "statuses";
	}
};

bool StatusHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	Dictionary::Ptr result = new Dictionary();

	if (request.RequestMethod != "GET") {
		response.SetStatus(400, "Bad request");
		result->Set("info", "Request must be type GET");
		HttpUtility::SendJsonBody(response, result);
		return true;
	}


	QueryDescription qd;
	qd.Types.insert("Status");
	qd.Provider = new StatusTargetProvider();
	qd.Permission = "status/query";

	Dictionary::Ptr params = HttpUtility::FetchRequestParameters(request);

	params->Set("type", "Status");

	if (request.RequestUrl->GetPath().size() >= 3)
		params->Set("status", request.RequestUrl->GetPath()[2]);

	std::vector<Value> objs = FilterUtility::GetFilterTargets(qd, params, user);

	Array::Ptr results = Array::FromVector(objs);

	result->Set("results", results);

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);

	return true;
}

