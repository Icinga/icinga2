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

#include "config/applyrule.hpp"
#include "base/logger.hpp"
#include <set>

using namespace icinga;

ApplyRule::RuleMap ApplyRule::m_Rules;
ApplyRule::TypeMap ApplyRule::m_Types;

ApplyRule::ApplyRule(const String& targetType, const String& name, const std::shared_ptr<Expression>& expression,
	const std::shared_ptr<Expression>& filter, const String& package, const String& fkvar, const String& fvvar, const std::shared_ptr<Expression>& fterm,
	bool ignoreOnError, const DebugInfo& di, const Dictionary::Ptr& scope)
	: m_TargetType(targetType), m_Name(name), m_Expression(expression), m_Filter(filter), m_Package(package), m_FKVar(fkvar),
	m_FVVar(fvvar), m_FTerm(fterm), m_IgnoreOnError(ignoreOnError), m_DebugInfo(di), m_Scope(scope)
{ }

String ApplyRule::GetTargetType(void) const
{
	return m_TargetType;
}

String ApplyRule::GetName(void) const
{
	return m_Name;
}

std::shared_ptr<Expression> ApplyRule::GetExpression(void) const
{
	return m_Expression;
}

std::shared_ptr<Expression> ApplyRule::GetFilter(void) const
{
	return m_Filter;
}

String ApplyRule::GetPackage(void) const
{
	return m_Package;
}

String ApplyRule::GetFKVar(void) const
{
	return m_FKVar;
}

String ApplyRule::GetFVVar(void) const
{
	return m_FVVar;
}

std::shared_ptr<Expression> ApplyRule::GetFTerm(void) const
{
	return m_FTerm;
}

bool ApplyRule::GetIgnoreOnError(void) const
{
	return m_IgnoreOnError;
}

DebugInfo ApplyRule::GetDebugInfo(void) const
{
	return m_DebugInfo;
}

Dictionary::Ptr ApplyRule::GetScope(void) const
{
	return m_Scope;
}

void ApplyRule::AddRule(const String& sourceType, const String& targetType, const String& name,
	const std::shared_ptr<Expression>& expression, const std::shared_ptr<Expression>& filter, const String& package, const String& fkvar,
	const String& fvvar, const std::shared_ptr<Expression>& fterm, bool ignoreOnError, const DebugInfo& di, const Dictionary::Ptr& scope)
{
	m_Rules[sourceType].emplace_back(new ApplyRule(targetType, name, expression, filter, package, fkvar, fvvar, fterm, ignoreOnError, di, scope));
}

bool ApplyRule::EvaluateFilter(ScriptFrame& frame) const
{
	return Convert::ToBool(m_Filter->Evaluate(frame));
}

void ApplyRule::RegisterType(const String& sourceType, const std::vector<String>& targetTypes)
{
	m_Types[sourceType] = targetTypes;
}

bool ApplyRule::IsValidSourceType(const String& sourceType)
{
	return m_Types.find(sourceType) != m_Types.end();
}

bool ApplyRule::IsValidTargetType(const String& sourceType, const String& targetType)
{
	auto it = m_Types.find(sourceType);

	if (it == m_Types.end())
		return false;

	if (it->second.size() == 1 && targetType == "")
		return true;

	for (const String& type : it->second) {
		if (type == targetType)
			return true;
	}

	return false;
}

std::vector<String> ApplyRule::GetTargetTypes(const String& sourceType)
{
	auto it = m_Types.find(sourceType);

	if (it == m_Types.end())
		return std::vector<String>();

	return it->second;
}

void ApplyRule::AddMatch(void)
{
	m_HasMatches = true;
}

bool ApplyRule::HasMatches(void) const
{
	return m_HasMatches;
}

std::vector<std::unique_ptr<ApplyRule> >& ApplyRule::GetRules(const String& type)
{
	auto it = m_Rules.find(type);
	if (it == m_Rules.end()) {
		static std::vector<std::unique_ptr<ApplyRule> > emptyList;
		return emptyList;
	}
	return it->second;
}

void ApplyRule::CheckMatches(void)
{
	for (const RuleMap::value_type& kv : m_Rules) {
		for (const auto& rule : kv.second) {
			if (!rule->HasMatches())
				Log(LogWarning, "ApplyRule")
					<< "Apply rule '" << rule->GetName() << "' (" << rule->GetDebugInfo() << ") for type '" << kv.first << "' does not match anywhere!";
		}
	}
}
