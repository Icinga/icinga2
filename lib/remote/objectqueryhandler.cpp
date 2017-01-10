/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "remote/objectqueryhandler.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "base/serializer.hpp"
#include "base/dependencygraph.hpp"
#include "base/configtype.hpp"
#include <boost/algorithm/string.hpp>
#include <set>

using namespace icinga;

REGISTER_URLHANDLER("/v1/objects", ObjectQueryHandler);

Dictionary::Ptr ObjectQueryHandler::SerializeObjectAttrs(const Object::Ptr& object,
    const String& attrPrefix, const Array::Ptr& attrs, bool isJoin, bool allAttrs)
{
	Type::Ptr type = object->GetReflectionType();

	std::vector<int> fids;

	if (isJoin && attrs) {
		ObjectLock olock(attrs);
		for (const String& attr : attrs) {
			if (attr == attrPrefix) {
				allAttrs = true;
				break;
			}
		}
	}

	if (!isJoin && (!attrs || attrs->GetLength() == 0))
		allAttrs = true;

	if (allAttrs) {
		for (int fid = 0; fid < type->GetFieldCount(); fid++) {
			fids.push_back(fid);
		}
	} else if (attrs) {
		ObjectLock olock(attrs);
		for (const String& attr : attrs) {
			String userAttr;
			
			if (isJoin) {
				String::SizeType dpos = attr.FindFirstOf(".");
				if (dpos == String::NPos)
					continue;

				String userJoinAttr = attr.SubStr(0, dpos);
				if (userJoinAttr != attrPrefix)
					continue;

				userAttr = attr.SubStr(dpos + 1);
			} else
				userAttr = attr;

			int fid = type->GetFieldId(userAttr);

			if (fid < 0)
				BOOST_THROW_EXCEPTION(ScriptError("Invalid field specified: " + userAttr));

			fids.push_back(fid);
		}
	}

	Dictionary::Ptr resultAttrs = new Dictionary();

	for (int& fid : fids)
	{
		Field field = type->GetFieldInfo(fid);

		Value val = object->GetField(fid);

		/* hide attributes which shouldn't be user-visible */
		if (field.Attributes & FANoUserView)
			continue;

		/* hide internal navigation fields */
		if (field.Attributes & FANavigation && !(field.Attributes & (FAConfig | FAState)))
			continue;

		Value sval = Serialize(val, FAConfig | FAState);
		resultAttrs->Set(field.Name, sval);
	}

	return resultAttrs;
}

bool ObjectQueryHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response, const Dictionary::Ptr& params)
{
	if (request.RequestUrl->GetPath().size() < 3 || request.RequestUrl->GetPath().size() > 4)
		return false;

	if (request.RequestMethod != "GET")
		return false;

	Type::Ptr type = FilterUtility::TypeFromPluralName(request.RequestUrl->GetPath()[2]);

	if (!type) {
		HttpUtility::SendJsonError(response, 400, "Invalid type specified.");
		return true;
	}

	QueryDescription qd;
	qd.Types.insert(type->GetName());
	qd.Permission = "objects/query/" + type->GetName();

	Array::Ptr uattrs = params->Get("attrs");
	Array::Ptr ujoins = params->Get("joins");
	Array::Ptr umetas = params->Get("meta");
	bool allJoins = HttpUtility::GetLastParameter(params, "all_joins");

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
		HttpUtility::SendJsonError(response, 404,
		    "No objects found.",
		    HttpUtility::GetLastParameter(params, "verboseErrors") ? DiagnosticInformation(ex) : "");
		return true;
	}

	Array::Ptr results = new Array();
	results->Reserve(objs.size());

	std::set<String> joinAttrs;
	std::set<String> userJoinAttrs;

	if (ujoins) {
		ObjectLock olock(ujoins);
		for (const String& ujoin : ujoins) {
			userJoinAttrs.insert(ujoin.SubStr(0, ujoin.FindFirstOf(".")));
		}
	}

	for (int fid = 0; fid < type->GetFieldCount(); fid++) {
		Field field = type->GetFieldInfo(fid);

		if (!(field.Attributes & FANavigation))
			continue;

		if (!allJoins && userJoinAttrs.find(field.NavigationName) == userJoinAttrs.end())
			continue;

		joinAttrs.insert(field.Name);
	}

	for (const ConfigObject::Ptr& obj : objs) {
		Dictionary::Ptr result1 = new Dictionary();
		results->Add(result1);

		result1->Set("name", obj->GetName());
		result1->Set("type", obj->GetReflectionType()->GetName());

		Dictionary::Ptr metaAttrs = new Dictionary();
		result1->Set("meta", metaAttrs);

		if (umetas) {
			ObjectLock olock(umetas);
			for (const String& meta : umetas) {
				if (meta == "used_by") {
					Array::Ptr used_by = new Array();
					metaAttrs->Set("used_by", used_by);

					for (const Object::Ptr& pobj : DependencyGraph::GetParents((obj)))
					{
						ConfigObject::Ptr configObj = dynamic_pointer_cast<ConfigObject>(pobj);

						if (!configObj)
							continue;

						Dictionary::Ptr refInfo = new Dictionary();
						refInfo->Set("type", configObj->GetReflectionType()->GetName());
						refInfo->Set("name", configObj->GetName());
						used_by->Add(refInfo);
					}
				} else if (meta == "location") {
					DebugInfo di = obj->GetDebugInfo();
					Dictionary::Ptr dinfo = new Dictionary();
					dinfo->Set("path", di.Path);
					dinfo->Set("first_line", di.FirstLine);
					dinfo->Set("first_column", di.FirstColumn);
					dinfo->Set("last_line", di.LastLine);
					dinfo->Set("last_column", di.LastColumn);
					metaAttrs->Set("location", dinfo);
				} else {
					HttpUtility::SendJsonError(response, 400, "Invalid field specified for meta: " + meta);
					return true;
				}
			}
		}

		try {
			result1->Set("attrs", SerializeObjectAttrs(obj, String(), uattrs, false, false));
		} catch (const ScriptError& ex) {
			HttpUtility::SendJsonError(response, 400, ex.what());
			return true;
		}

		Dictionary::Ptr joins = new Dictionary();
		result1->Set("joins", joins);

		for (const String& joinAttr : joinAttrs) {
			Object::Ptr joinedObj;
			int fid = type->GetFieldId(joinAttr);

			if (fid < 0) {
				HttpUtility::SendJsonError(response, 400, "Invalid field specified for join: " + joinAttr);
				return true;
			}

			Field field = type->GetFieldInfo(fid);

			if (!(field.Attributes & FANavigation)) {
				HttpUtility::SendJsonError(response, 400, "Not a joinable field: " + joinAttr);
				return true;
			}

			joinedObj = obj->NavigateField(fid);

			if (!joinedObj)
				continue;

			String prefix = field.NavigationName;

			try {
				joins->Set(prefix, SerializeObjectAttrs(joinedObj, prefix, ujoins, true, allJoins));
			} catch (const ScriptError& ex) {
				HttpUtility::SendJsonError(response, 400, ex.what());
				return true;
			}
		}
	}

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);

	return true;
}
