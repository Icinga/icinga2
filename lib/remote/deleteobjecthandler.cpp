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

#include "remote/deleteobjecthandler.hpp"
#include "remote/configobjectutility.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "remote/apiaction.hpp"
#include "config/configitem.hpp"
#include "base/exception.hpp"
#include "base/serializer.hpp"
#include <boost/algorithm/string.hpp>
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1", DeleteObjectHandler);

bool DeleteObjectHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	if (request.RequestMethod != "DELETE") {
		HttpUtility::SendJsonError(response, 400, "Invalid request type. Must be DELETE.");
		return true;
	}

	if (request.RequestUrl->GetPath().size() < 2) {
		String path = boost::algorithm::join(request.RequestUrl->GetPath(), "/");
		HttpUtility::SendJsonError(response, 404, "The requested path is too long to match any config tag requests.");
		return true;
	}

	Type::Ptr type = FilterUtility::TypeFromPluralName(request.RequestUrl->GetPath()[1]);

	if (!type) {
		HttpUtility::SendJsonError(response, 400, "Erroneous type was supplied.");
		return true;
	}

	QueryDescription qd;
	qd.Types.insert(type->GetName());

	Dictionary::Ptr params = HttpUtility::FetchRequestParameters(request);

	params->Set("type", type->GetName());

	if (request.RequestUrl->GetPath().size() >= 3) {
		String attr = type->GetName();
		boost::algorithm::to_lower(attr);
		params->Set(attr, request.RequestUrl->GetPath()[2]);
	}

	std::vector<Value> objs = FilterUtility::GetFilterTargets(qd, params);

	bool cascade = HttpUtility::GetLastParameter(params, "cascade");

	Array::Ptr results = new Array();

	BOOST_FOREACH(const ConfigObject::Ptr& obj, objs) {
		Dictionary::Ptr result1 = new Dictionary();
		result1->Set("type", type->GetName());
		result1->Set("name", obj->GetName());
		results->Add(result1);

		Array::Ptr errors = new Array();

		if (!ConfigObjectUtility::DeleteObject(obj, cascade, errors)) {
			result1->Set("code", 500);
			result1->Set("status", "Object could not be deleted.");
			result1->Set("errors", errors);
		} else {
			result1->Set("code", 200);
			result1->Set("status", "Object was deleted.");
		}
	}

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);

	return true;
}

