/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/filterutility.hpp"
#include "remote/httputility.hpp"
#include "config/applyrule.hpp"
#include "config/configcompiler.hpp"
#include "config/expression.hpp"
#include "base/namespace.hpp"
#include "base/json.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include <memory>

using namespace icinga;

Type::Ptr FilterUtility::TypeFromPluralName(const String& pluralName)
{
	String uname = pluralName;
	boost::algorithm::to_lower(uname);

	for (const Type::Ptr& type : Type::GetAllTypes()) {
		String pname = type->GetPluralName();
		boost::algorithm::to_lower(pname);

		if (uname == pname)
			return type;
	}

	return nullptr;
}

void ConfigObjectTargetProvider::FindTargets(const String& type, const std::function<void (const Value&)>& addTarget) const
{
	Type::Ptr ptype = Type::GetByName(type);
	auto *ctype = dynamic_cast<ConfigType *>(ptype.get());

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

	Namespace::Ptr frameNS;

	if (frame.Self.IsEmpty()) {
		frameNS = new Namespace();
		frame.Self = frameNS;
	} else {
		/* Enforce a namespace object for 'frame.self'. */
		ASSERT(frame.Self.IsObjectType<Namespace>());

		frameNS = frame.Self;

		ASSERT(frameNS != ScriptGlobal::GetGlobals());
	}

	frameNS->Set("obj", target);
	frameNS->Set(varName, target);

	for (int fid = 0; fid < type->GetFieldCount(); fid++) {
		Field field = type->GetFieldInfo(fid);

		if ((field.Attributes & FANavigation) == 0)
			continue;

		Object::Ptr joinedObj = target->NavigateField(fid);

		if (field.NavigationName)
			frameNS->Set(field.NavigationName, joinedObj);
		else
			frameNS->Set(field.Name, joinedObj);
	}

	return Convert::ToBool(filter->Evaluate(frame));
}

static void FilteredAddTarget(ScriptFrame& permissionFrame, Expression *permissionFilter,
	ScriptFrame& frame, Expression *ufilter, std::vector<Value>& result, const String& variableName, const Object::Ptr& target)
{
	if (FilterUtility::EvaluateFilter(permissionFrame, permissionFilter, target, variableName)) {
		if (FilterUtility::EvaluateFilter(frame, ufilter, target, variableName)) {
			result.emplace_back(std::move(target));
		}
	}
}

/**
 * Checks whether the given API user is granted the given permission
 *
 * When you desire an exception to be raised when the given user doesn't have the given permission,
 * you need to use FilterUtility::CheckPermission().
 *
 * @param user ApiUser pointer to the user object you want to check the permission of
 * @param permission The actual permission you want to check the user permission against
 * @param permissionFilter Expression pointer that is used as an output buffer for all the filter expressions of the
 *                         individual permissions of the given user to be evaluated. It's up to the caller to delete
 *                         this pointer when it's not needed any more.
 *
 * @return bool
 */
bool FilterUtility::HasPermission(const ApiUser::Ptr& user, const String& permission, std::unique_ptr<Expression>* permissionFilter)
{
	if (permissionFilter)
		*permissionFilter = nullptr;

	if (permission.IsEmpty())
		return true;

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
				std::vector<std::unique_ptr<Expression> > args;
				args.emplace_back(new GetScopeExpression(ScopeThis));
				std::unique_ptr<Expression> indexer{new IndexerExpression(std::unique_ptr<Expression>(MakeLiteral(filter)), std::unique_ptr<Expression>(MakeLiteral("call")))};
				FunctionCallExpression *fexpr = new FunctionCallExpression(std::move(indexer), std::move(args));

				if (!*permissionFilter)
					permissionFilter->reset(fexpr);
				else
					*permissionFilter = std::make_unique<LogicalOrExpression>(std::move(*permissionFilter), std::unique_ptr<Expression>(fexpr));
			}
		}
	}

	if (!foundPermission) {
		Log(LogWarning, "FilterUtility")
			<< "Missing permission: " << requiredPermission;
	}

	return foundPermission;
}

void FilterUtility::CheckPermission(const ApiUser::Ptr& user, const String& permission, std::unique_ptr<Expression>* permissionFilter)
{
	if (!HasPermission(user, permission, permissionFilter)) {
		BOOST_THROW_EXCEPTION(ScriptError("Missing permission: " + permission.ToLower()));
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

	std::unique_ptr<Expression> permissionFilter;
	CheckPermission(user, qd.Permission, &permissionFilter);

	Namespace::Ptr permissionFrameNS = new Namespace();
	ScriptFrame permissionFrame(false, permissionFrameNS);

	for (const String& type : qd.Types) {
		String attr = type;
		boost::algorithm::to_lower(attr);

		if (attr == "type")
			attr = "name";

		if (query && query->Contains(attr)) {
			String name = HttpUtility::GetLastParameter(query, attr);
			Object::Ptr target = provider->GetTargetByName(type, name);

			if (!FilterUtility::EvaluateFilter(permissionFrame, permissionFilter.get(), target, variableName))
				BOOST_THROW_EXCEPTION(ScriptError("Access denied to object '" + name + "' of type '" + type + "'"));

			result.emplace_back(std::move(target));
		}

		attr = provider->GetPluralName(type);
		boost::algorithm::to_lower(attr);

		if (query && query->Contains(attr)) {
			Array::Ptr names = query->Get(attr);
			if (names) {
				ObjectLock olock(names);
				for (const String& name : names) {
					Object::Ptr target = provider->GetTargetByName(type, name);

					if (!FilterUtility::EvaluateFilter(permissionFrame, permissionFilter.get(), target, variableName))
						BOOST_THROW_EXCEPTION(ScriptError("Access denied to object '" + name + "' of type '" + type + "'"));

					result.emplace_back(std::move(target));
				}
			}
		}
	}

	if ((query && query->Contains("filter")) || result.empty()) {
		if (!query->Contains("type"))
			BOOST_THROW_EXCEPTION(std::invalid_argument("Type must be specified when using a filter."));

		String type = HttpUtility::GetLastParameter(query, "type");

		if (!provider->IsValidType(type))
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid type specified."));

		if (qd.Types.find(type) == qd.Types.end())
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid type specified for this query."));

		Namespace::Ptr frameNS = new Namespace();
		ScriptFrame frame(false, frameNS);
		frame.Sandboxed = true;

		if (query->Contains("filter")) {
			String filter = HttpUtility::GetLastParameter(query, "filter");
			std::unique_ptr<Expression> ufilter = ConfigCompiler::CompileText("<API query>", filter);
			Dictionary::Ptr filter_vars = query->Get("filter_vars");
			bool targeted = false;
			std::vector<ConfigObject::Ptr> targets;

			if (dynamic_cast<ConfigObjectTargetProvider*>(provider.get())) {
				auto dict (dynamic_cast<DictExpression*>(ufilter.get()));

				if (dict) {
					auto& subex (dict->GetExpressions());

					if (subex.size() == 1u) {
						if (type == "Host") {
							std::vector<const String *> targetNames;

							if (ApplyRule::GetTargetHosts(subex.at(0).get(), targetNames, filter_vars)) {
								static const auto typeHost (Type::GetByName("Host"));
								static const auto ctypeHost (dynamic_cast<ConfigType*>(typeHost.get()));
								targeted = true;

								for (auto name : targetNames) {
									auto target (ctypeHost->GetObject(*name));

									if (target) {
										targets.emplace_back(target);
									}
								}
							}
						} else if (type == "Service") {
							std::vector<std::pair<const String *, const String *>> targetNames;

							if (ApplyRule::GetTargetServices(subex.at(0).get(), targetNames, filter_vars)) {
								static const auto typeService (Type::GetByName("Service"));
								static const auto ctypeService (dynamic_cast<ConfigType*>(typeService.get()));
								targeted = true;

								for (auto name : targetNames) {
									auto target (ctypeService->GetObject(*name.first + "!" + *name.second));

									if (target) {
										targets.emplace_back(target);
									}
								}
							}
						}
					}
				}
			}

			if (targeted) {
				for (auto& target : targets) {
					if (FilterUtility::EvaluateFilter(permissionFrame, permissionFilter.get(), target, variableName)) {
						result.emplace_back(std::move(target));
					}
				}
			} else {
				if (filter_vars) {
					ObjectLock olock (filter_vars);

					for (auto& kv : filter_vars) {
						frameNS->Set(kv.first, kv.second);
					}
				}

				provider->FindTargets(type, [&permissionFrame, &permissionFilter, &frame, &ufilter, &result, variableName](const Object::Ptr& target) {
					FilteredAddTarget(permissionFrame, permissionFilter.get(), frame, &*ufilter, result, variableName, target);
				});
			}
		} else {
			/* Ensure to pass a nullptr as filter expression.
			 * GCC 8.1.1 on F28 causes problems, see GH #6533.
			 */
			provider->FindTargets(type, [&permissionFrame, &permissionFilter, &frame, &result, variableName](const Object::Ptr& target) {
				FilteredAddTarget(permissionFrame, permissionFilter.get(), frame, nullptr, result, variableName, target);
			});
		}
	}

	return result;
}

