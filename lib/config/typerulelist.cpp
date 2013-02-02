/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "i2-config.h"

using namespace icinga;

/**
 * Adds a rule to a rule list.
 *
 * @param rule The rule that should be added.
 */
void TypeRuleList::AddRule(const TypeRule& rule)
{
	m_Rules.push_back(rule);
}

/**
 * Adds all rules from the specified rule list.
 *
 * @param ruleList The rule list to copy rules from.
 */
void TypeRuleList::AddRules(const TypeRuleList::Ptr& ruleList)
{
	BOOST_FOREACH(const TypeRule& rule, ruleList->m_Rules) {
		AddRule(rule);
	}
}

/**
 * Returns the number of rules currently contained in the list.
 *
 * @returns The length of the list.
 */
size_t TypeRuleList::GetLength(void) const
{
	return m_Rules.size();
}

/**
 * Finds a matching rule.
 *
 * @param name The name of the attribute.
 * @param value The value of the attribute.
 * *@param[out] subRules The list of sub-rules for the matching rule.
 */
bool TypeRuleList::FindMatch(const String& name, const Value& value, TypeRuleList::Ptr *subRules)
{
	BOOST_FOREACH(const TypeRule& rule, m_Rules) {
		if (rule.Matches(name, value)) {
			*subRules = rule.GetSubRules();
			return true;
		}
	}
	
	return false;
}
