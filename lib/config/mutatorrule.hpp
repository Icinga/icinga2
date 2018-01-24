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

#ifndef MUTATORRULE_H
#define MUTATORRULE_H

#include "config/i2-config.hpp"
#include "config/expression.hpp"
#include "base/debuginfo.hpp"

namespace icinga
{

/**
 * @ingroup config
 */
class MutatorRule
{
public:
	typedef std::vector<MutatorRule> RuleVector;

	String GetNamePattern() const;
	std::set<Type::Ptr> GetTargetTypes() const;
	std::shared_ptr<Expression> GetExpression() const;
	DebugInfo GetDebugInfo() const;
	Dictionary::Ptr GetScope() const;

	void EvaluateRule(ScriptFrame& frame, const ConfigObject::Ptr& object, DebugHint *dhint = nullptr) const;

	static void AddRule(const String& namePattern, const std::set<Type::Ptr>& targets, const std::shared_ptr<Expression>& expression,
		const DebugInfo& di, const Dictionary::Ptr& scope);
	static void EvaluateRules(ScriptFrame& frame, const ConfigObject::Ptr& object, DebugHint *dhint = nullptr);

private:
	String m_NamePattern;
	std::set<Type::Ptr> m_TargetTypes;
	std::shared_ptr<Expression> m_Expression;
	DebugInfo m_DebugInfo;
	Dictionary::Ptr m_Scope;

	static RuleVector m_Rules;

	MutatorRule(String namePattern, std::set<Type::Ptr> targetTypes, std::shared_ptr<Expression> expression,
		DebugInfo di, Dictionary::Ptr scope);
};

}

#endif /* MUTATORRULE_H */
