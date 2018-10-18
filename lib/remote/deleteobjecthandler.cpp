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

#include "remote/deleteobjecthandler.hpp"
#include "remote/configobjectutility.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "remote/apiaction.hpp"
#include "config/configitem.hpp"
#include "base/exception.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/objects", DeleteObjectHandler);

bool DeleteObjectHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response, const Dictionary::Ptr& params)
{
	if (request.RequestUrl->GetPath().size() < 3 || request.RequestUrl->GetPath().size() > 4)
		return false;

	if (request.RequestMethod != "DELETE")
		return false;

	Type::Ptr type = FilterUtility::TypeFromPluralName(request.RequestUrl->GetPath()[2]);

	if (!type) {
		HttpUtility::SendJsonError(response, params, 400, "Invalid type specified.");
		return true;
	}

	QueryDescription qd;
	qd.Types.insert(type->GetName());
	qd.Permission = "objects/delete/" + type->GetName();

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

	bool cascade = HttpUtility::GetLastParameter(params, "cascade");
	bool verbose = HttpUtility::GetLastParameter(params, "verbose");

	ArrayData results;

	bool success = true;

	for (const ConfigObject::Ptr& obj : objs) {
		int code;
		String status;
		Array::Ptr errors = new Array();
		Array::Ptr diagnosticInformation = new Array();

		if (!ConfigObjectUtility::DeleteObject(obj, cascade, errors, diagnosticInformation)) {
			code = 500;
			status = "Object could not be deleted.";
			success = false;
		} else {
			code = 200;
			status = "Object was deleted.";
		}

		Dictionary::Ptr result = new Dictionary({
			{ "type", type->GetName() },
			{ "name", obj->GetName() },
			{ "code", code },
			{ "status", status },
			{ "errors", errors }
		});

		if (verbose)
			result->Set("diagnostic_information", diagnosticInformation);

		results.push_back(result);
	}

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array(std::move(results)) }
	});

	if (!success)
		response.SetStatus(500, "One or more objects could not be deleted");
	else
		response.SetStatus(200, "OK");

	HttpUtility::SendJsonBody(response, params, result);

	return true;
}

