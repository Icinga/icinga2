/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-present Icinga Development Team (http://www.icinga.org) *
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

#include "config/applyrule.h"
#include "base/logger_fwd.h"

using namespace icinga;

ApplyRule::RuleMap ApplyRule::m_Rules;
ApplyRule::CallbackMap ApplyRule::m_Callbacks;

ApplyRule::ApplyRule(const String& name, const AExpression::Ptr& expression,
    const AExpression::Ptr& filter, const DebugInfo& di, const Dictionary::Ptr& scope)
	: m_Name(name), m_Expression(expression), m_Filter(filter), m_DebugInfo(di), m_Scope(scope)
{ }

String ApplyRule::GetName(void) const
{
	return m_Name;
}

AExpression::Ptr ApplyRule::GetExpression(void) const
{
	return m_Expression;
}

AExpression::Ptr ApplyRule::GetFilter(void) const
{
	return m_Filter;
}

DebugInfo ApplyRule::GetDebugInfo(void) const
{
	return m_DebugInfo;
}

Dictionary::Ptr ApplyRule::GetScope(void) const
{
	return m_Scope;
}

void ApplyRule::AddRule(const String& sourceType, const String& name,
    const AExpression::Ptr& expression, const AExpression::Ptr& filter,
    const DebugInfo& di, const Dictionary::Ptr& scope)
{
	m_Rules[sourceType].push_back(ApplyRule(name, expression, filter, di, scope));
}

bool ApplyRule::EvaluateFilter(const Dictionary::Ptr& scope) const
{
	scope->Set("__parent", m_Scope);
	bool result = m_Filter->Evaluate(scope);
	scope->Remove("__parent");
	return result;
}

void ApplyRule::EvaluateRules(void)
{
	std::set<String> completedTypes;

	while (completedTypes.size() < m_Callbacks.size()) {
		std::pair<String, std::pair<Callback, String> > kv;
		BOOST_FOREACH(kv, m_Callbacks) {
			const String& sourceType = kv.first;

			if (completedTypes.find(sourceType) != completedTypes.end())
				continue;

			const Callback& callback = kv.second.first;
			const String& targetType = kv.second.second;

			if (IsValidType(targetType) && completedTypes.find(targetType) == completedTypes.end())
				continue;

			completedTypes.insert(sourceType);

			RuleMap::const_iterator it = m_Rules.find(kv.first);

			if (it == m_Rules.end())
				continue;

			callback(it->second);
		}
	}

	m_Rules.clear();
}

void ApplyRule::RegisterType(const String& sourceType, const String& targetType, const ApplyRule::Callback& callback)
{
	m_Callbacks[sourceType] = make_pair(callback, targetType);
}

bool ApplyRule::IsValidType(const String& sourceType)
{
	return m_Callbacks.find(sourceType) != m_Callbacks.end();
}
