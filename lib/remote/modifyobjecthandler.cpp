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

#include "remote/modifyobjecthandler.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "remote/apiaction.hpp"
#include "base/exception.hpp"
#include "base/serializer.hpp"
#include <boost/algorithm/string.hpp>
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/objects", ModifyObjectHandler);

bool ModifyObjectHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	if (request.RequestMethod != "POST") {
		/* there might be other request methods pending */
		return false;
	}

	if (request.RequestUrl->GetPath().size() < 3)
		return false;

	Type::Ptr type = FilterUtility::TypeFromPluralName(request.RequestUrl->GetPath()[1]);

	if (!type)
		return false;

	QueryDescription qd;
	qd.Types.insert(type->GetName());
	qd.Permission = "objects/modify/" + type->GetName();

	Dictionary::Ptr params = HttpUtility::FetchRequestParameters(request);

	params->Set("type", type->GetName());

	if (request.RequestUrl->GetPath().size() >= 4) {
		String attr = type->GetName();
		boost::algorithm::to_lower(attr);
		params->Set(attr, request.RequestUrl->GetPath()[3]);
	}

	std::vector<Value> objs = FilterUtility::GetFilterTargets(qd, params);

	Dictionary::Ptr attrs = params->Get("attrs");

	Array::Ptr results = new Array();

	BOOST_FOREACH(const ConfigObject::Ptr& obj, objs) {
		Dictionary::Ptr result1 = new Dictionary();

		result1->Set("type", type->GetName());
		result1->Set("name", obj->GetName());

		String key;

		try {
			if (attrs) {
				ObjectLock olock(attrs);
				BOOST_FOREACH(const Dictionary::Pair& kv, attrs) {
					key = kv.first;
					obj->ModifyAttribute(kv.first, kv.second);
				}
			}

			result1->Set("code", 200);
			result1->Set("status", "Attributes updated.");
		} catch (const std::exception& ex) {
			result1->Set("code", 500);
			result1->Set("status", "Attribute '" + key + "' could not be set: " + DiagnosticInformation(ex));
		}

		results->Add(result1);
	}

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);

	return true;
}

