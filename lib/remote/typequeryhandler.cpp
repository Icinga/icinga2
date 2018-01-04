/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "remote/typequeryhandler.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "base/configtype.hpp"
#include "base/scriptglobal.hpp"
#include "base/logger.hpp"
#include <boost/algorithm/string.hpp>
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/types", TypeQueryHandler);

class TypeTargetProvider final : public TargetProvider
{
public:
	DECLARE_PTR_TYPEDEFS(TypeTargetProvider);

	virtual void FindTargets(const String& type,
		const std::function<void (const Value&)>& addTarget) const override
	{
		for (const Type::Ptr& target : Type::GetAllTypes()) {
			addTarget(target);
		}
	}

	virtual Value GetTargetByName(const String& type, const String& name) const override
	{
		Type::Ptr ptype = Type::GetByName(name);

		if (!ptype)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Type does not exist."));

		return ptype;
	}

	virtual bool IsValidType(const String& type) const override
	{
		return type == "Type";
	}

	virtual String GetPluralName(const String& type) const override
	{
		return "types";
	}
};

bool TypeQueryHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response, const Dictionary::Ptr& params)
{
	if (request.RequestUrl->GetPath().size() > 3)
		return false;

	if (request.RequestMethod != "GET")
		return false;

	QueryDescription qd;
	qd.Types.insert("Type");
	qd.Permission = "types";
	qd.Provider = new TypeTargetProvider();

	if (params->Contains("type"))
		params->Set("name", params->Get("type"));

	params->Set("type", "Type");

	if (request.RequestUrl->GetPath().size() >= 3)
		params->Set("name", request.RequestUrl->GetPath()[2]);

	std::vector<Value> objs;

	try {
		objs = FilterUtility::GetFilterTargets(qd, params, user);
	} catch (const std::exception& ex) {
		HttpUtility::SendJsonError(response, 404,
			"No objects found.",
			HttpUtility::GetLastParameter(params, "verboseErrors") ? DiagnosticInformation(ex) : "");
		return true;
	}

	Array::Ptr results = new Array();

	for (const Type::Ptr& obj : objs) {
		Dictionary::Ptr result1 = new Dictionary();
		results->Add(result1);

		Dictionary::Ptr resultAttrs = new Dictionary();
		result1->Set("name", obj->GetName());
		result1->Set("plural_name", obj->GetPluralName());
		if (obj->GetBaseType())
			result1->Set("base", obj->GetBaseType()->GetName());
		result1->Set("abstract", obj->IsAbstract());
		result1->Set("fields", resultAttrs);

		Dictionary::Ptr prototype = dynamic_pointer_cast<Dictionary>(obj->GetPrototype());
		Array::Ptr prototypeKeys = new Array();
		result1->Set("prototype_keys", prototypeKeys);

		if (prototype) {
			ObjectLock olock(prototype);
			for (const Dictionary::Pair& kv : prototype) {
				prototypeKeys->Add(kv.first);
			}
		}

		int baseFieldCount = 0;

		if (obj->GetBaseType())
			baseFieldCount = obj->GetBaseType()->GetFieldCount();

		for (int fid = baseFieldCount; fid < obj->GetFieldCount(); fid++) {
			Field field = obj->GetFieldInfo(fid);

			Dictionary::Ptr fieldInfo = new Dictionary();
			resultAttrs->Set(field.Name, fieldInfo);

			fieldInfo->Set("id", fid);
			fieldInfo->Set("type", field.TypeName);
			if (field.RefTypeName)
				fieldInfo->Set("ref_type", field.RefTypeName);
			if (field.Attributes & FANavigation)
				fieldInfo->Set("navigation_name", field.NavigationName);
			fieldInfo->Set("array_rank", field.ArrayRank);

			Dictionary::Ptr attributeInfo = new Dictionary();
			fieldInfo->Set("attributes", attributeInfo);

			attributeInfo->Set("config", static_cast<bool>(field.Attributes & FAConfig));
			attributeInfo->Set("state", static_cast<bool>(field.Attributes & FAState));
			attributeInfo->Set("required", static_cast<bool>(field.Attributes & FARequired));
			attributeInfo->Set("navigation", static_cast<bool>(field.Attributes & FANavigation));
			attributeInfo->Set("no_user_modify", static_cast<bool>(field.Attributes & FANoUserModify));
			attributeInfo->Set("no_user_view", static_cast<bool>(field.Attributes & FANoUserView));
			attributeInfo->Set("deprecated", static_cast<bool>(field.Attributes & FADeprecated));
		}
	}

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);

	return true;
}

