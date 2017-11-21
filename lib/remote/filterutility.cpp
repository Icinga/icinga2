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

#include "remote/filterutility.hpp"
#include "remote/httputility.hpp"
#include "config/configcompiler.hpp"
#include "config/expression.hpp"
#include "base/json.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include <boost/algorithm/string.hpp>

using namespace icinga;

Type::Ptr FilterUtility::TypeFromPluralName(const String& pluralName)
{
	String uname = pluralName;
	boost::algorithm::to_lower(uname);

	for (const Type::Ptr&type : Type::GetAllTypes()) {
		String pname = type->GetPluralName();
		boost::algorithm::to_lower(pname);

		if (uname == pname)
			return type;
	}

	return Type::Ptr();
}

void ConfigObjectTargetProvider::FindTargets(const String& type, const std::function<void (const Value&)>& addTarget) const
{
	Type::Ptr ptype = Type::GetByName(type);
	ConfigType *ctype = dynamic_cast<ConfigType *>(ptype.get());

	if (ctype) {
		for (const ConfigObject::Ptr& object : ctype->GetObjects()) {
			addTarget(object);
		}
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
	Type::Ptr ptype = Type::GetByName(type);

	if (!ptype)
		return false;

	return ConfigObject::TypeInstance->IsAssignableFrom(ptype);
}

String ConfigObjectTargetProvider::GetPluralName(const String& type) const
{
	return Type::GetByName(type)->GetPluralName();
}

bool FilterUtility::EvaluateFilter(ScriptFrame& frame, Expression *filter,
    const Object::Ptr& target, const String& variableName)
{
	if (!filter)
		return true;

	Type::Ptr type = target->GetReflectionType();
	String varName;

	if (variableName.IsEmpty())
		varName = type->GetName().ToLower();
	else
		varName = variableName;

	Dictionary::Ptr vars;

	if (frame.Self.IsEmpty()) {
		vars = new Dictionary();
		frame.Self = vars;
	} else
		vars = frame.Self;

	vars->Set("obj", target);
	vars->Set(varName, target);

	for (int fid = 0; fid < type->GetFieldCount(); fid++) {
		Field field = type->GetFieldInfo(fid);

		if ((field.Attributes & FANavigation) == 0)
			continue;

		Object::Ptr joinedObj = target->NavigateField(fid);

		if (field.NavigationName)
			vars->Set(field.NavigationName, joinedObj);
		else
			vars->Set(field.Name, joinedObj);
	}

	return Convert::ToBool(filter->Evaluate(frame));
}

static void FilteredAddTarget(ScriptFrame& permissionFrame, Expression *permissionFilter,
    ScriptFrame& frame, Expression *ufilter, std::vector<Value>& result, const String& variableName, const Object::Ptr& target)
{
	if (FilterUtility::EvaluateFilter(permissionFrame, permissionFilter, target, variableName) && FilterUtility::EvaluateFilter(frame, ufilter, target, variableName))
		result.push_back(target);
}

void FilterUtility::CheckPermission(const ApiUser::Ptr& user, const String& permission, Expression **permissionFilter)
{
	if (permissionFilter)
		*permissionFilter = NULL;

	if (permission.IsEmpty())
		return;

	bool foundPermission = false;
	String requiredPermission = permission.ToLower();

	Array::Ptr permissions = user->GetPermissions();
	if (permissions) {
		ObjectLock olock(permissions);
		for (const Value& item : permissions) {
			String permission;
			Function::Ptr filter;
			if (item.IsObjectType<Dictionary>()) {
				Dictionary::Ptr dict = item;
				permission = dict->Get("permission");
				filter = dict->Get("filter");
			} else
				permission = item;

			permission = permission.ToLower();

			if (!Utility::Match(permission, requiredPermission))
				continue;

			foundPermission = true;

			if (filter && permissionFilter) {
				std::vector<Expression *> args;
				args.push_back(new GetScopeExpression(ScopeLocal));
				FunctionCallExpression *fexpr = new FunctionCallExpression(new IndexerExpression(MakeLiteral(filter), MakeLiteral("call")), args);

				if (!*permissionFilter)
					*permissionFilter = fexpr;
				else
					*permissionFilter = new LogicalOrExpression(*permissionFilter, fexpr);
			}
		}
	}

	if (!foundPermission) {
		Log(LogWarning, "FilterUtility")
		    << "Missing permission: " << requiredPermission;

		BOOST_THROW_EXCEPTION(ScriptError("Missing permission: " + requiredPermission));
	}
}

std::vector<Value> FilterUtility::GetFilterTargets(const QueryDescription& qd, const Dictionary::Ptr& query, const ApiUser::Ptr& user, const String& variableName)
{
	std::vector<Value> result;

	TargetProvider::Ptr provider;

	if (qd.Provider)
		provider = qd.Provider;
	else
		provider = new ConfigObjectTargetProvider();

	Expression *permissionFilter;
	CheckPermission(user, qd.Permission, &permissionFilter);

	ScriptFrame permissionFrame;

	for (const String& type : qd.Types) {
		String attr = type;
		boost::algorithm::to_lower(attr);

		if (attr == "type")
			attr = "name";

		if (query->Contains(attr)) {
			String name = HttpUtility::GetLastParameter(query, attr);
			Object::Ptr target = provider->GetTargetByName(type, name);

			if (!FilterUtility::EvaluateFilter(permissionFrame, permissionFilter, target, variableName))
				BOOST_THROW_EXCEPTION(ScriptError("Access denied to object '" + name + "' of type '" + type + "'"));

			result.push_back(target);
		}

		attr = provider->GetPluralName(type);
		boost::algorithm::to_lower(attr);

		if (query->Contains(attr)) {
			Array::Ptr names = query->Get(attr);
			if (names) {
				ObjectLock olock(names);
				for (const String& name : names) {
					Object::Ptr target = provider->GetTargetByName(type, name);

					if (!FilterUtility::EvaluateFilter(permissionFrame, permissionFilter, target, variableName))
						BOOST_THROW_EXCEPTION(ScriptError("Access denied to object '" + name + "' of type '" + type + "'"));

					result.push_back(target);
				}
			}
		}
	}

	if (query->Contains("filter") || result.empty()) {
		if (!query->Contains("type"))
			BOOST_THROW_EXCEPTION(std::invalid_argument("Type must be specified when using a filter."));

		String type = HttpUtility::GetLastParameter(query, "type");

		if (!provider->IsValidType(type))
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid type specified."));

		if (qd.Types.find(type) == qd.Types.end())
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid type specified for this query."));

		ScriptFrame frame;
		frame.Sandboxed = true;
		Dictionary::Ptr uvars = new Dictionary();

		Expression *ufilter = NULL;

		if (query->Contains("filter")) {
			String filter = HttpUtility::GetLastParameter(query, "filter");
			ufilter = ConfigCompiler::CompileText("<API query>", filter);
		}

		Dictionary::Ptr filter_vars = query->Get("filter_vars");
		if (filter_vars) {
			ObjectLock olock(filter_vars);
			for (const Dictionary::Pair& kv : filter_vars) {
				uvars->Set(kv.first, kv.second);
			}
		}

		frame.Self = uvars;

		try {
			provider->FindTargets(type, std::bind(&FilteredAddTarget,
			    boost::ref(permissionFrame), permissionFilter,
			    boost::ref(frame), ufilter, boost::ref(result), variableName, _1));
		} catch (const std::exception& ex) {
			delete ufilter;
			throw;
		}

		delete ufilter;
	}

	return result;
}

