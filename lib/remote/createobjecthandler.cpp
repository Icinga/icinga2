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
#include "base/configtype.hpp"
#include <boost/algorithm/string.hpp>
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/objects", CreateObjectHandler);

bool CreateObjectHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	if (request.RequestUrl->GetPath().size() != 4)
		return false;

	if (request.RequestMethod != "PUT")
		return false;

	Type::Ptr type = FilterUtility::TypeFromPluralName(request.RequestUrl->GetPath()[2]);

	if (!type) {
		HttpUtility::SendJsonError(response, 400, "Invalid type specified.");
		return true;
	}

	FilterUtility::CheckPermission(user, "objects/create/" + type->GetName());

	String name = request.RequestUrl->GetPath()[3];
	Dictionary::Ptr params = HttpUtility::FetchRequestParameters(request);
	Array::Ptr templates = params->Get("templates");
	Dictionary::Ptr attrs = params->Get("attrs");

	Dictionary::Ptr result1 = new Dictionary();
	int code;
	String status;
	Array::Ptr errors = new Array();

	bool ignoreOnError = false;

	if (params->Contains("ignore_on_error"))
		ignoreOnError = HttpUtility::GetLastParameter(params, "ignore_on_error");

	String config = ConfigObjectUtility::CreateObjectConfig(type, name, ignoreOnError, templates, attrs);

	Array::Ptr results = new Array();
	results->Add(result1);

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	if (!ConfigObjectUtility::CreateObject(type, name, config, errors)) {
		result1->Set("errors", errors);
		result1->Set("code", 500);
		result1->Set("status", "Object could not be created.");

		response.SetStatus(500, "Object could not be created");
		HttpUtility::SendJsonBody(response, result);

		return true;
	}

	ConfigType::Ptr dtype = ConfigType::GetByName(type->GetName());

	ConfigObject::Ptr obj = dtype->GetObject(name);

	result1->Set("code", 200);

	if (obj)
		result1->Set("status", "Object was created");
	else if (!obj && ignoreOnError)
		result1->Set("status", "Object was not created but 'ignore_on_error' was set to true");

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);

	return true;
}

