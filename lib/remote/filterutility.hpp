// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef FILTERUTILITY_H
#define FILTERUTILITY_H

#include "remote/i2-remote.hpp"
#include "remote/apiuser.hpp"
#include "config/expression.hpp"
#include "base/dictionary.hpp"
#include "base/configobject.hpp"
#include <set>

namespace icinga
{

class TargetProvider : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(TargetProvider);

	virtual void FindTargets(const String& type, const std::function<void (const Value&)>& addTarget) const = 0;
	virtual Value GetTargetByName(const String& type, const String& name) const = 0;
	virtual bool IsValidType(const String& type) const = 0;
	virtual String GetPluralName(const String& type) const = 0;
};

class ConfigObjectTargetProvider final : public TargetProvider
{
public:
	DECLARE_PTR_TYPEDEFS(ConfigObjectTargetProvider);

	void FindTargets(const String& type, const std::function<void (const Value&)>& addTarget) const override;
	Value GetTargetByName(const String& type, const String& name) const override;
	bool IsValidType(const String& type) const override;
	String GetPluralName(const String& type) const override;
};

struct QueryDescription
{
	std::set<String> Types;
	TargetProvider::Ptr Provider;
	String Permission;
};

/**
 * Filter utilities.
 *
 * @ingroup remote
 */
class FilterUtility
{
public:
	
	static Dictionary::Ptr GetTargetForVar(const String& name, const Value& value);
	static Type::Ptr TypeFromPluralName(const String& pluralName);
	static void CheckPermission(const ApiUser::Ptr& user, const String& permission, std::unique_ptr<Expression>* filter = nullptr);
	static bool HasPermission(const ApiUser::Ptr& user, const String& permission, std::unique_ptr<Expression>* permissionFilter = nullptr);
	static std::vector<Value> GetFilterTargets(const QueryDescription& qd, const Dictionary::Ptr& query,
		const ApiUser::Ptr& user, const String& variableName = String());
	static bool EvaluateFilter(ScriptFrame& frame, Expression *filter,
		const Object::Ptr& target, const String& variableName = String());
};

/**
 * Exception to report a missing permission to an API user.
 *
 * IMPORTANT: The what() message is reported back to the user and MUST NOT contain sensitive information like names of
 * objects they are not allowed to access. When using the exception, also pay attention that throwing the exception
 * does not introduce a sidechannel. For example, it should not be returned if a user-specified object exists but the
 * user is not allowed to access it, otherwise they would learn that the object exists.
 */
class MissingPermissionError : public ScriptError
{
	using ScriptError::ScriptError;
};

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

	explicit FilterExprPermissionChecker(ApiUser::Ptr user);

	Expression* CheckPermission(const String& permissionString);
	bool CanAccessGlobalVariable(const String& varName) override;
	bool CanAccessConfigObject(const ConfigObject::Ptr& obj) override;

private:
	bool CheckPermissionAndEvalFilter(const String& permissionString, const Object::Ptr& obj, const String& varName);

	std::unordered_map<String, std::pair<bool, std::unique_ptr<Expression>>> m_PermCache;
	ApiUser::Ptr m_User;
};

}

#endif /* FILTERUTILITY_H */
