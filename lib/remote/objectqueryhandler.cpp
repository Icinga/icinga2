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

bool ObjectQueryHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
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

	Dictionary::Ptr params = HttpUtility::FetchRequestParameters(request);

	std::set<String> userJoinAttrs;
	std::set<String> attrs;
	Array::Ptr uattrs = params->Get("attrs");

	if (uattrs) {
		ObjectLock olock(uattrs);
		BOOST_FOREACH(const String& uattr, uattrs) {
			attrs.insert(uattr);

			String::SizeType dpos = uattr.FindFirstOf(".");
			if (dpos == String::NPos) {
				HttpUtility::SendJsonError(response, 400, "Attribute name must contain '.'.");
				return true;
			}

			String userJoinAttr = uattr.SubStr(0, dpos);

			if (userJoinAttr == type->GetName().ToLower())
				userJoinAttr = "";

			userJoinAttrs.insert(userJoinAttr);
		}
	}

	std::vector<String> joinAttrs;
	joinAttrs.push_back("");

	for (int fid = 0; fid < type->GetFieldCount(); fid++) {
		Field field = type->GetFieldInfo(fid);

		if (!(field.Attributes & FANavigation))
			continue;

		if (!userJoinAttrs.empty() && userJoinAttrs.find(field.Name) == userJoinAttrs.end())
			continue;

		joinAttrs.push_back(field.Name);
	}

	params->Set("type", type->GetName());

	if (request.RequestUrl->GetPath().size() >= 4) {
		String attr = type->GetName();
		boost::algorithm::to_lower(attr);
		params->Set(attr, request.RequestUrl->GetPath()[3]);
	}

	std::vector<Value> objs = FilterUtility::GetFilterTargets(qd, params, user);

	Array::Ptr results = new Array();
	results->Reserve(objs.size());

	BOOST_FOREACH(const ConfigObject::Ptr& obj, objs) {
		Dictionary::Ptr result1 = new Dictionary();
		results->Add(result1);

		Dictionary::Ptr resultAttrs = new Dictionary();
		result1->Set("attrs", resultAttrs);

		BOOST_FOREACH(const String& joinAttr, joinAttrs) {
			Object::Ptr joinedObj;
			String prefix;

			if (joinAttr.IsEmpty()) {
				joinedObj = obj;
				prefix = type->GetName();
			} else {
				int fid = type->GetFieldId(joinAttr);
				joinedObj = obj->NavigateField(fid);

				if (!joinedObj)
					continue;

				Field field = type->GetFieldInfo(fid);
				prefix = field.NavigationName;
			}

			boost::algorithm::to_lower(prefix);

			Type::Ptr joinedType = joinedObj->GetReflectionType();

			std::vector<int> fids;

			if (attrs.empty()) {
				for (int fid = 0; fid < joinedType->GetFieldCount(); fid++) {
					fids.push_back(fid);
				}
			} else {
				BOOST_FOREACH(const String& aname, attrs) {
					String::SizeType dpos = aname.FindFirstOf(".");
					ASSERT(dpos != String::NPos);

					String userJoinAttr = aname.SubStr(0, dpos);
					if (userJoinAttr != prefix)
						continue;

					String userAttr = aname.SubStr(dpos + 1);

					int fid = joinedType->GetFieldId(userAttr);
					fids.push_back(fid);
				}
			}

			BOOST_FOREACH(int& fid, fids) {
				Field field = joinedType->GetFieldInfo(fid);
				String aname = prefix + "." + field.Name;

				Value val = joinedObj->GetField(fid);

				/* hide internal navigation fields */
				if (field.Attributes & FANavigation) {
					Value nval = joinedObj->NavigateField(fid);

					if (val == nval)
						continue;
				}

				Value sval = Serialize(val, FAConfig | FAState);
				resultAttrs->Set(aname, sval);
			}
		}

		Array::Ptr used_by = new Array();
		result1->Set("used_by", used_by);

		BOOST_FOREACH(const Object::Ptr& pobj, DependencyGraph::GetParents((obj))) {
			ConfigObject::Ptr configObj = dynamic_pointer_cast<ConfigObject>(pobj);

			if (!configObj)
				continue;

			Dictionary::Ptr refInfo = new Dictionary();
			refInfo->Set("type", configObj->GetType()->GetName());
			refInfo->Set("name", configObj->GetName());
			used_by->Add(refInfo);
		}
	}

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);

	return true;
}

