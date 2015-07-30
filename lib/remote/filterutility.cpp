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

#include "remote/filterutility.hpp"
#include "remote/httputility.hpp"
#include "config/configcompiler.hpp"
#include "config/expression.hpp"
#include "base/json.hpp"
#include "base/dynamictype.hpp"
#include "base/logger.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

using namespace icinga;

Type::Ptr FilterUtility::TypeFromPluralName(const String& pluralName)
{
	String uname = pluralName;
	boost::algorithm::to_lower(uname);

	BOOST_FOREACH(const DynamicType::Ptr& dtype, DynamicType::GetTypes()) {
		Type::Ptr type = Type::GetByName(dtype->GetName());
		ASSERT(type);

		String pname = GetPluralName(type);
		boost::algorithm::to_lower(pname);

		if (uname == pname)
			return type;
	}

	return Type::Ptr();
}

String FilterUtility::GetPluralName(const Type::Ptr& type)
{
	String name = type->GetName();

	if (name[name.GetLength() - 1] == 'y')
		return name.SubStr(0, name.GetLength() - 1) + "ies";
	else
		return name + "s";
}

DynamicObject::Ptr FilterUtility::GetObjectByTypeAndName(const String& type, const String& name)
{
	DynamicType::Ptr dtype = DynamicType::GetByName(type);
	ASSERT(dtype);

	return dtype->GetObject(name);
}

std::vector<DynamicObject::Ptr> FilterUtility::GetFilterTargets(const QueryDescription& qd, const Dictionary::Ptr& query)
{
	std::vector<DynamicObject::Ptr> result;

	BOOST_FOREACH(const Type::Ptr& type, qd.Types) {
		String attr = type->GetName();
		boost::algorithm::to_lower(attr);

		if (query->Contains(attr)) {
			String name = HttpUtility::GetLastParameter(query, attr);
			DynamicObject::Ptr obj = GetObjectByTypeAndName(type->GetName(), name);
			if (!obj)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Object does not exist."));
			result.push_back(obj);
		}

		attr = GetPluralName(type);
		boost::algorithm::to_lower(attr);

		if (query->Contains(attr)) {
			Array::Ptr names = query->Get(attr);
			if (names) {
				ObjectLock olock(names);
				BOOST_FOREACH(const String& name, names) {
					DynamicObject::Ptr obj = GetObjectByTypeAndName(type->GetName(), name);
					if (!obj)
						BOOST_THROW_EXCEPTION(std::invalid_argument("Object does not exist."));
					result.push_back(obj);
				}
			}
		}
	}

	if (query->Contains("filter") || result.empty()) {
		if (!query->Contains("type"))
			BOOST_THROW_EXCEPTION(std::invalid_argument("Type must be specified when using a filter."));

		String filter;
		if (!query->Contains("filter"))
			filter = "true";
		else
			filter = HttpUtility::GetLastParameter(query, "filter");

		String type = HttpUtility::GetLastParameter(query, "type");

		Log(LogInformation, "FilterUtility", filter);

		Type::Ptr utype = Type::GetByName(type);

		if (!utype)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid type specified."));

		if (qd.Types.find(utype) == qd.Types.end())
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid type specified for this query."));

		DynamicType::Ptr dtype = DynamicType::GetByName(type);
		ASSERT(dtype);

		Expression *ufilter = ConfigCompiler::CompileText("<API query>", filter, false);
		ScriptFrame frame;
		frame.Sandboxed = true;

		String varName = utype->GetName();
		boost::algorithm::to_lower(varName);

		try {
			BOOST_FOREACH(const DynamicObject::Ptr& object, dtype->GetObjects()) {
				frame.Locals->Set(varName, object);

				if (Convert::ToBool(ufilter->Evaluate(frame)))
					result.push_back(object);
			}
		} catch (const std::exception& ex) {
			delete ufilter;
			throw;
		}

		delete ufilter;
	}

	return result;
}
