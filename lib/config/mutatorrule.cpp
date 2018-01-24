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

#include "config/mutatorrule.hpp"
#include "base/logger.hpp"
#include <set>

using namespace icinga;

MutatorRule::RuleVector MutatorRule::m_Rules;

MutatorRule::MutatorRule(String namePattern, std::set<Type::Ptr> targets, std::shared_ptr<Expression> expression,
	DebugInfo di, Dictionary::Ptr scope)
	: m_NamePattern(std::move(namePattern)), m_TargetTypes(std::move(targets)),
	m_Expression(std::move(expression)), m_DebugInfo(std::move(di)), m_Scope(std::move(scope))
{ }

String MutatorRule::GetNamePattern() const
{
	return m_NamePattern;
}

std::set<Type::Ptr> MutatorRule::GetTargetTypes() const
{
	return m_TargetTypes;
}

std::shared_ptr<Expression> MutatorRule::GetExpression() const
{
	return m_Expression;
}

DebugInfo MutatorRule::GetDebugInfo() const
{
	return m_DebugInfo;
}

Dictionary::Ptr MutatorRule::GetScope() const
{
	return m_Scope;
}

void MutatorRule::AddRule(const String& namePattern, const std::set<Type::Ptr>& targetTypes, const std::shared_ptr<Expression>& expression,
	const DebugInfo& di, const Dictionary::Ptr& scope)
{
	m_Rules.push_back(MutatorRule(namePattern, targetTypes, expression, di, scope));
}

void MutatorRule::EvaluateRule(ScriptFrame& frame, const ConfigObject::Ptr& object, DebugHint *dhint) const
{
	if (m_Scope)
		m_Scope->CopyTo(frame.Locals);

	ASSERT(frame.Self == object);

	m_Expression->Evaluate(frame, dhint);
}

void MutatorRule::EvaluateRules(ScriptFrame& frame, const ConfigObject::Ptr& object, DebugHint *dhint)
{
	for (const MutatorRule& rule : m_Rules) {
		if (!rule.m_TargetTypes.empty()) {
			bool found = false;
			Type::Ptr type = object->GetReflectionType();

			for (const Type::Ptr& targetType : rule.m_TargetTypes) {
				if (targetType->IsAssignableFrom(type)) {
					found = true;
					break;
				}
			}

			if (!found)
				continue;
		}

		if (!rule.m_NamePattern.IsEmpty() && !Utility::Match(rule.m_NamePattern, object->GetName()))
			continue;

		rule.EvaluateRule(frame, object, dhint);
	}
}
