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

#include "remote/createobjecthandler.hpp"
#include "remote/configobjectutility.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "remote/apiaction.hpp"
#include <boost/algorithm/string.hpp>
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1", CreateObjectHandler);

bool CreateObjectHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	if (request.RequestMethod != "PUT") {
		HttpUtility::SendJsonError(response, 400, "Invalid request type. Must be PUT.");
		return true;
	}

	if (request.RequestUrl->GetPath().size() < 3) {
		HttpUtility::SendJsonError(response, 400, "Object name is missing.");
		return true;
	}

	Type::Ptr type = FilterUtility::TypeFromPluralName(request.RequestUrl->GetPath()[1]);

	if (!type) {
		HttpUtility::SendJsonError(response, 403, "Erroneous type was supplied.");
		return true;
	}

	String name = request.RequestUrl->GetPath()[2];
	Dictionary::Ptr params = HttpUtility::FetchRequestParameters(request);
	Array::Ptr templates = params->Get("templates");
	Dictionary::Ptr attrs = params->Get("attrs");

	Dictionary::Ptr result1 = new Dictionary();
	int code;
	String status;
	Array::Ptr errors = new Array();

	String config = ConfigObjectUtility::CreateObjectConfig(type, name, templates, attrs);

	if (!ConfigObjectUtility::CreateObject(type, name, config, errors)) {
		result1->Set("errors", errors);
		HttpUtility::SendJsonError(response, 500, "Object could not be created.");
		return true;
	}

	result1->Set("code", 200);
	result1->Set("status", "Object was created");

	Array::Ptr results = new Array();
	results->Add(result1);

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);

	return true;
}

