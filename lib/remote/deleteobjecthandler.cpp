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
	if (request.RequestMethod != "DELETE")
		return false;

	if (request.RequestUrl->GetPath().size() < 2)
		return false;

	Type::Ptr type = FilterUtility::TypeFromPluralName(request.RequestUrl->GetPath()[1]);

	if (!type)
		return false;

	QueryDescription qd;
	qd.Types.insert(type);

	Dictionary::Ptr params = HttpUtility::FetchRequestParameters(request);

	params->Set("type", type->GetName());

	if (request.RequestUrl->GetPath().size() >= 3) {
		String attr = type->GetName();
		boost::algorithm::to_lower(attr);
		params->Set(attr, request.RequestUrl->GetPath()[2]);
	}

	std::vector<ConfigObject::Ptr> objs = FilterUtility::GetFilterTargets(qd, params);

	Array::Ptr results = new Array();

	BOOST_FOREACH(const ConfigObject::Ptr& obj, objs) {
		Dictionary::Ptr result1 = new Dictionary();
		result1->Set("type", type->GetName());
		result1->Set("name", obj->GetName());
		results->Add(result1);

		if (obj->GetModule() != "_api") {
			result1->Set("code", 500);
			result1->Set("status", "Object cannot be deleted because it was not created using the API.");
			continue;
		}

		ConfigItem::Ptr item = ConfigItem::GetObject(type->GetName(), obj->GetName());

		try {
			obj->Deactivate();

			if (item)
				item->Unregister();
			else
				obj->Unregister();

			result1->Set("code", 200);
			result1->Set("status", "Object was deleted.");
		} catch (const std::exception& ex) {
			result1->Set("code", 500);
			result1->Set("status", "Object could not be deleted: " + DiagnosticInformation(ex));
		}
	}

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);

	return true;
}

