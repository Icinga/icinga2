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
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

using namespace icinga;

Type::Ptr FilterUtility::TypeFromPluralName(const String& pluralName)
{
	String uname = pluralName;
	boost::algorithm::to_lower(uname);

	BOOST_FOREACH(const ConfigType::Ptr& dtype, ConfigType::GetTypes()) {
		Type::Ptr type = Type::GetByName(dtype->GetName());
		ASSERT(type);

		String pname = type->GetPluralName();
		boost::algorithm::to_lower(pname);

		if (uname == pname)
			return type;
	}

	return Type::Ptr();
}

void ConfigObjectTargetProvider::FindTargets(const String& type, const boost::function<void (const Value&)>& addTarget) const
{
	ConfigType::Ptr dtype = ConfigType::GetByName(type);
	ASSERT(dtype);

	BOOST_FOREACH(const ConfigObject::Ptr& object, dtype->GetObjects()) {
		addTarget(object);
	}
}

Value ConfigObjectTargetProvider::GetTargetByName(const String& type, const String& name) const
{
	ConfigObject::Ptr obj = ConfigObject::GetObject(type, name);

	if (!obj)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Object does not exist."));

	return obj;
}

bool ConfigObjectTargetProvider::IsValidType(const String& type) const
{
	return ConfigType::GetByName(type) != ConfigType::Ptr();
}

String ConfigObjectTargetProvider::GetPluralName(const String& type) const
{
	return Type::GetByName(type)->GetPluralName();
}

static void FilteredAddTarget(ScriptFrame& frame, Expression *ufilter, std::vector<Value>& result, const Object::Ptr& target)
{
	Type::Ptr type = target->GetReflectionType();
	String varName = type->GetName();
	boost::algorithm::to_lower(varName);

	frame.Locals->Set(varName, target);

	for (int fid = 0; fid < type->GetFieldCount(); fid++) {
		Field field = type->GetFieldInfo(fid);

		if ((field.Attributes & FANavigation) == 0)
			continue;

		Object::Ptr joinedObj = target->NavigateField(fid);

		varName = field.TypeName;
		boost::algorithm::to_lower(varName);

		frame.Locals->Set(varName, joinedObj);
	}

	if (Convert::ToBool(ufilter->Evaluate(frame)))
		result.push_back(target);
}

std::vector<Value> FilterUtility::GetFilterTargets(const QueryDescription& qd, const Dictionary::Ptr& query)
{
	std::vector<Value> result;

	TargetProvider::Ptr provider;

	if (qd.Provider)
		provider = qd.Provider;
	else
		provider = new ConfigObjectTargetProvider();

	BOOST_FOREACH(const String& type, qd.Types) {
		String attr = type;
		boost::algorithm::to_lower(attr);

		if (attr == "type")
			attr = "name";

		if (query->Contains(attr))
			result.push_back(provider->GetTargetByName(type, HttpUtility::GetLastParameter(query, attr)));

		attr = provider->GetPluralName(type);
		boost::algorithm::to_lower(attr);

		if (query->Contains(attr)) {
			Array::Ptr names = query->Get(attr);
			if (names) {
				ObjectLock olock(names);
				BOOST_FOREACH(const String& name, names) {
					result.push_back(provider->GetTargetByName(type, name));
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

		if (!provider->IsValidType(type))
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid type specified."));

		if (qd.Types.find(type) == qd.Types.end())
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid type specified for this query."));

		Expression *ufilter = ConfigCompiler::CompileText("<API query>", filter);
		ScriptFrame frame;
		frame.Sandboxed = true;

		Dictionary::Ptr filter_vars = query->Get("filter_vars");
		if (filter_vars) {
			ObjectLock olock(filter_vars);
			BOOST_FOREACH(const Dictionary::Pair& kv, filter_vars) {
				frame.Locals->Set(kv.first, kv.second);
			}
		}

		try {
			provider->FindTargets(type, boost::bind(&FilteredAddTarget, boost::ref(frame), ufilter, boost::ref(result), _1));
		} catch (const std::exception& ex) {
			delete ufilter;
			throw;
		}

		delete ufilter;
	}

	return result;
}
