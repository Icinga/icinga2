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

#include "remote/modifyobjecthandler.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "remote/apiaction.hpp"
#include "base/exception.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/objects", ModifyObjectHandler);

bool ModifyObjectHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response, const Dictionary::Ptr& params)
{
	if (request.RequestUrl->GetPath().size() < 3 || request.RequestUrl->GetPath().size() > 4)
		return false;

	if (request.RequestMethod != "POST")
		return false;

	Type::Ptr type = FilterUtility::TypeFromPluralName(request.RequestUrl->GetPath()[2]);

	if (!type) {
		HttpUtility::SendJsonError(response, params, 400, "Invalid type specified.");
		return true;
	}

	QueryDescription qd;
	qd.Types.insert(type->GetName());
	qd.Permission = "objects/modify/" + type->GetName();

	params->Set("type", type->GetName());

	if (request.RequestUrl->GetPath().size() >= 4) {
		String attr = type->GetName();
		boost::algorithm::to_lower(attr);
		params->Set(attr, request.RequestUrl->GetPath()[3]);
	}

	std::vector<Value> objs;

	try {
		objs = FilterUtility::GetFilterTargets(qd, params, user);
	} catch (const std::exception& ex) {
		HttpUtility::SendJsonError(response, params, 404,
			"No objects found.",
			DiagnosticInformation(ex));
		return true;
	}

	Value attrsVal = params->Get("attrs");

	if (attrsVal.GetReflectionType() != Dictionary::TypeInstance) {
		HttpUtility::SendJsonError(response, params, 400,
			"Invalid type for 'attrs' attribute specified. Dictionary type is required.");
		return true;
	}

	Dictionary::Ptr attrs = attrsVal;

	bool verbose = false;

	if (params)
		verbose = HttpUtility::GetLastParameter(params, "verbose");

	ArrayData results;

	for (const ConfigObject::Ptr& obj : objs) {
		Dictionary::Ptr result1 = new Dictionary();

		result1->Set("type", type->GetName());
		result1->Set("name", obj->GetName());

		String key;

		try {
			if (attrs) {
				ObjectLock olock(attrs);
				for (const Dictionary::Pair& kv : attrs) {
					key = kv.first;
					obj->ModifyAttribute(kv.first, kv.second);
				}
			}

			result1->Set("code", 200);
			result1->Set("status", "Attributes updated.");
		} catch (const std::exception& ex) {
			result1->Set("code", 500);
			result1->Set("status", "Attribute '" + key + "' could not be set: " + DiagnosticInformation(ex, false));

			if (verbose)
				result1->Set("diagnostic_information", DiagnosticInformation(ex));
		}

		results.push_back(std::move(result1));
	}

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array(std::move(results)) }
	});

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, params, result);

	return true;
}
