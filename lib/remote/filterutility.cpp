/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/filterutility.hpp"
#include "remote/httputility.hpp"
#include "config/configcompiler.hpp"
#include "config/expression.hpp"
#include "base/namespace.hpp"
#include "base/json.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include <unordered_map>
#include <boost/algorithm/string/case_conv.hpp>

using namespace icinga;

Dictionary::Ptr FilterUtility::GetTargetForVar(const String& name, const Value& value)
{
	return new Dictionary({
		{ "name", name },
		{ "type", value.GetReflectionType()->GetName() },
		{ "value", value }
	});
}

/**
 * Controls access to an object or variable based on an ApiUser's permissions.
 *
 * This is accomplished by caching the generated filter expressions so they don't have to be
 * regenerated again and again when access is repeatedly checked in script functions and when
 * evaluating expressions.
 */
class FilterExprPermissionChecker : public ScriptPermissionChecker
{
public:
	DECLARE_PTR_TYPEDEFS(FilterExprPermissionChecker);

	explicit FilterExprPermissionChecker(ApiUser::Ptr user) : m_User(std::move(user)) {}

	/**
	 * Check if the user has the given permission and cache the result if they do.
	 *
	 * This is a wrapper around FilterUtility::CheckPermission() that caches the generated
	 * filter expression for later use when checking permissions inside sandboxed ScriptFrames.
	 *
	 * Like FilterUtility::CheckPermission() an exception is thrown if the user does not have
	 * the requested permission.
	 *
	 * If the user has permission and there is a filter for the given permission, the filter
	 * expression  is generated, cached and then a pointer to it is returned, otherwise a
	 * nullptr will be returned.
	 *
	 * Since the optionally returned pointer is a raw-pointer and this class retains ownership
	 * over the expression it is only valid for the lifetime of the @c FilterExprPermissionChecker
	 * object that returned it.
	 *
	 * @param permissionString The permission string to check against the ApiUser member of this class.
	 *
	 * @return a pointer to the generated permission expression if the permission has a filter, or nullptr if not.
	 */
	Expression* CheckPermission(const String& permissionString)
	{
		bool inserted = m_PermCache.find(permissionString) == m_PermCache.end();
		auto& entry = m_PermCache[permissionString];
		auto& hasPermission = entry.first;
		auto& permissionExpr = entry.second;

		if (inserted) {
			Expression* expr;
			FilterUtility::CheckPermission(m_User, permissionString, &expr);
			permissionExpr = std::unique_ptr<Expression>(expr);
		} else if (!hasPermission) {
			BOOST_THROW_EXCEPTION(ScriptError("Missing permission: " + permissionString.ToLower()));
		}

		hasPermission = true;
		return permissionExpr.get();
	}

	/**
	 * Checks if this object's ApiUser has permissions to access variable `varName`.
	 *
	 * @param varName The name of the variable to check for access
	 *
	 * @return 'true' if the variable can be accessed, 'false' if it can't.
	 */
	bool CanAccessGlobalVariable(const String& varName) override
	{
		auto obj = FilterUtility::GetTargetForVar(varName, ScriptGlobal::Get(varName));
		return CheckPermissionAndEvalFilter("variables", obj, "variable");
	}

	/**
	 * Checks if this object's ApiUser has permissions to access ConfigObject `obj`.
	 *
	 * @param obj A pointer to the ConfigObject to check for access
	 *
	 * @return 'true' if the object can be accessed, 'false' if it can't.
	 */
	bool CanAccessConfigObject(const ConfigObject::Ptr& obj) override
	{
		ASSERT(obj);

		String perm = "objects/query/" + obj->GetReflectionType()->GetName();
		String varName = obj->GetReflectionType()->GetName().ToLower();

		return CheckPermissionAndEvalFilter(perm, obj, varName);
	}

private:
	bool CheckPermissionAndEvalFilter(const String& permissionString, const Object::Ptr& obj, const String& varName)
	{
		bool inserted = m_PermCache.find(permissionString) == m_PermCache.end();
		auto& entry = m_PermCache[permissionString];
		auto& hasPermission = entry.first;
		auto& permissionExpr = entry.second;

		if (inserted) {
			Expression* expr;
			hasPermission = FilterUtility::HasPermission(m_User, permissionString, &expr);
			permissionExpr = std::unique_ptr<Expression>(expr);
		}

		if (hasPermission && permissionExpr) {
			ScriptFrame permissionFrame(false, new Namespace());
			// Sandboxing is lifted because this only evaluates the function from the
			// ApiUser->permissions->filter
			permissionFrame.Sandboxed = false;
			return FilterUtility::EvaluateFilter(permissionFrame, permissionExpr.get(), obj, varName);
		}

		return hasPermission;
	}

	std::unordered_map<String, std::pair<bool, std::unique_ptr<Expression>>> m_PermCache;
	ApiUser::Ptr m_User;
};

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
bool FilterUtility::HasPermission(const ApiUser::Ptr& user, const String& permission, Expression **permissionFilter)
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
					*permissionFilter = fexpr;
				else
					*permissionFilter = new LogicalOrExpression(std::unique_ptr<Expression>(*permissionFilter), std::unique_ptr<Expression>(fexpr));
			}
		}
	}

	if (!foundPermission) {
		Log(LogWarning, "FilterUtility")
			<< "Missing permission: " << requiredPermission;
	}

	return foundPermission;
}

void FilterUtility::CheckPermission(const ApiUser::Ptr& user, const String& permission, Expression **permissionFilter)
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

	FilterExprPermissionChecker::Ptr permissionChecker = new FilterExprPermissionChecker{user};
	auto* permissionFilter = permissionChecker->CheckPermission(qd.Permission);

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

			if (!FilterUtility::EvaluateFilter(permissionFrame, permissionFilter, target, variableName))
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

					if (!FilterUtility::EvaluateFilter(permissionFrame, permissionFilter, target, variableName))
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
		frame.PermChecker = permissionChecker;

		if (query->Contains("filter")) {
			String filter = HttpUtility::GetLastParameter(query, "filter");
			std::unique_ptr<Expression> ufilter = ConfigCompiler::CompileText("<API query>", filter);

			Dictionary::Ptr filter_vars = query->Get("filter_vars");
			if (filter_vars) {
				ObjectLock olock(filter_vars);
				for (const Dictionary::Pair& kv : filter_vars) {
					frameNS->Set(kv.first, kv.second);
				}
			}

			provider->FindTargets(type, [&permissionFrame, permissionFilter, &frame, &ufilter, &result, variableName](const Object::Ptr& target) {
				FilteredAddTarget(permissionFrame, permissionFilter, frame, &*ufilter, result, variableName, target);
			});
		} else {
			/* Ensure to pass a nullptr as filter expression.
			 * GCC 8.1.1 on F28 causes problems, see GH #6533.
			 */
			provider->FindTargets(type, [&permissionFrame, permissionFilter, &frame, &result, variableName](const Object::Ptr& target) {
				FilteredAddTarget(permissionFrame, permissionFilter, frame, nullptr, result, variableName, target);
			});
		}
	}

	return result;
}

