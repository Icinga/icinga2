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
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "remote/apiaction.hpp"
#include "config/configitembuilder.hpp"
#include "config/configitem.hpp"
#include "base/exception.hpp"
#include "base/serializer.hpp"
#include <boost/algorithm/string.hpp>
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1", CreateObjectHandler);

bool CreateObjectHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	if (request.RequestMethod != "PUT")
		return false;

	if (request.RequestUrl->GetPath().size() < 3)
		return false;

	Type::Ptr type = FilterUtility::TypeFromPluralName(request.RequestUrl->GetPath()[1]);

	if (!type)
		return false;

	String name = request.RequestUrl->GetPath()[2];

	Dictionary::Ptr params = HttpUtility::FetchRequestParameters(request);

	ConfigItemBuilder::Ptr builder = new ConfigItemBuilder();
	builder->SetType(type->GetName());
	builder->SetName(name);
	builder->SetScope(ScriptGlobal::GetGlobals());

	Array::Ptr templates = params->Get("templates");

	if (templates) {
		ObjectLock olock(templates);
		BOOST_FOREACH(const String& tmpl, templates) {
			ImportExpression *expr = new ImportExpression(MakeLiteral(tmpl));
			builder->AddExpression(expr);
		}
	}

	Dictionary::Ptr attrs = params->Get("attrs");

	if (attrs) {
		ObjectLock olock(attrs);
		BOOST_FOREACH(const Dictionary::Pair& kv, attrs) {
			SetExpression *expr = new SetExpression(MakeIndexer(ScopeThis, kv.first), OpSetLiteral, MakeLiteral(kv.second));
			builder->AddExpression(expr);
		}
	}

	ConfigItem::Ptr item = builder->Compile();
	item->Register();

	ConfigItem::CommitAndActivate();

	Dictionary::Ptr result1 = new Dictionary();
	result1->Set("code", 200);
	result1->Set("status", "Object created.");

	Array::Ptr results = new Array();
	results->Add(result1);

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);

	return true;
}

