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

	virtual void FindTargets(const String& type, const std::function<void (const Value&)>& addTarget) const override;
	virtual Value GetTargetByName(const String& type, const String& name) const override;
	virtual bool IsValidType(const String& type) const override;
	virtual String GetPluralName(const String& type) const override;
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
	static Type::Ptr TypeFromPluralName(const String& pluralName);
	static void CheckPermission(const ApiUser::Ptr& user, const String& permission, Expression **filter = nullptr);
	static std::vector<Value> GetFilterTargets(const QueryDescription& qd, const Dictionary::Ptr& query,
		const ApiUser::Ptr& user, const String& variableName = String());
	static bool EvaluateFilter(ScriptFrame& frame, Expression *filter,
		const Object::Ptr& target, const String& variableName = String());
};

}

#endif /* FILTERUTILITY_H */
