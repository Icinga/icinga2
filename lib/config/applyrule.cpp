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

using namespace icinga;

ApplyRule::RuleMap ApplyRule::m_Rules;
ApplyRule::CallbackMap ApplyRule::m_Callbacks;

ApplyRule::ApplyRule(const String& tmpl, const AExpression::Ptr& expression, const DebugInfo& di)
	: m_Template(tmpl), m_Expression(expression), m_DebugInfo(di)
{ }

String ApplyRule::GetTemplate(void) const
{
	return m_Template;
}

AExpression::Ptr ApplyRule::GetExpression(void) const
{
	return m_Expression;
}

DebugInfo ApplyRule::GetDebugInfo(void) const
{
	return m_DebugInfo;
}

void ApplyRule::AddRule(const String& sourceType, const String& tmpl, const String& targetType, const AExpression::Ptr& expression, const DebugInfo& di)
{
	m_Rules[std::make_pair(sourceType, targetType)].push_back(ApplyRule(tmpl, expression, di));
}

void ApplyRule::EvaluateRules(void)
{
	std::pair<TypeCombination, Callback> kv;
	BOOST_FOREACH(kv, m_Callbacks) {
		RuleMap::const_iterator it = m_Rules.find(kv.first);

		if (it == m_Rules.end())
			continue;

		kv.second(it->second);
	}
}

void ApplyRule::RegisterCombination(const String& sourceType, const String& targetType, const ApplyRule::Callback& callback)
{
	m_Callbacks[std::make_pair(sourceType, targetType)] = callback;
}

bool ApplyRule::IsValidCombination(const String& sourceType, const String& targetType)
{
	return m_Callbacks.find(std::make_pair(sourceType, targetType)) != m_Callbacks.end();
}
