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
#include "base/serializer.hpp"
#include "base/statsfunction.hpp"

using namespace icinga;

REGISTER_URLHANDLER("/v1/status", StatusHandler);

bool StatusHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	Dictionary::Ptr result = new Dictionary();

	if (request.RequestMethod != "GET") {
		response.SetStatus(400, "Bad request");
		result->Set("info", "Request must be type GET");
		HttpUtility::SendJsonBody(response, result);
		return true;
	}

	if (request.RequestUrl->GetPath().size() < 2) {
		response.SetStatus(400, "Bad request");
		HttpUtility::SendJsonBody(response, result);
		return true;
	}

	Array::Ptr results = new Array();
	Dictionary::Ptr resultInner = new Dictionary();

	if (request.RequestUrl->GetPath().size() > 2) {

		StatsFunction::Ptr funcptr = StatsFunctionRegistry::GetInstance()->GetItem(request.RequestUrl->GetPath()[2]);
		resultInner = new Dictionary();

		if (!funcptr)
			return false;

		results->Add(resultInner);

		Dictionary::Ptr status = new Dictionary();
		Array::Ptr perfdata = new Array();
		funcptr->Invoke(status, perfdata);

		resultInner->Set("status", status);
		resultInner->Set("perfdata", perfdata);
	} else {
		typedef std::pair<String, StatsFunction::Ptr> kv_pair;
		BOOST_FOREACH(const kv_pair& kv, StatsFunctionRegistry::GetInstance()->GetItems()) {
			resultInner = new Dictionary();
			Dictionary::Ptr funcStatus = new Dictionary();
			Array::Ptr funcPData = new Array();
			kv.second->Invoke(funcStatus, funcPData);

			resultInner->Set("name", kv.first);
			resultInner->Set("status", funcPData);
			resultInner->Set("perfdata", funcPData);

			results->Add(resultInner);
		}
	}

	result->Set("results", results);

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);

	return true;
}

