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
	return m_Filter->Evaluate(scope);
}

void ApplyRule::EvaluateRules(void)
{
	// TODO: priority
	std::pair<String, std::pair<Callback, int> > kv;
	BOOST_FOREACH(kv, m_Callbacks) {
		RuleMap::const_iterator it = m_Rules.find(kv.first);

		if (it == m_Rules.end())
			continue;

		kv.second.first(it->second);
	}
}

void ApplyRule::RegisterType(const String& sourceType, const ApplyRule::Callback& callback, int priority)
{
	m_Callbacks[sourceType] = make_pair(callback, priority);
}

bool ApplyRule::IsValidType(const String& sourceType)
{
	return m_Callbacks.find(sourceType) != m_Callbacks.end();
}
