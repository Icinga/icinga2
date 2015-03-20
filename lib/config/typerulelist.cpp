/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "config/typerulelist.hpp"
#include "config/typerule.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

/**
 * Adds a validator method for a rule list.
 *
 * @param validator The validator.
 */
void TypeRuleList::AddValidator(const String& validator)
{
	m_Validators.push_back(validator);
}

/**
 * Retrieves the validator methods.
 *
 * @returns The validator methods.
 */
std::vector<String> TypeRuleList::GetValidators(void) const
{
	return m_Validators;
}

/**
 * Adds an attribute to the list of required attributes.
 *
 * @param attr The new required attribute.
 */
void TypeRuleList::AddRequire(const String& attr)
{
	m_Requires.push_back(attr);
}

/**
 * Retrieves the list of required attributes.
 *
 * @returns The list of required attributes.
 */
std::vector<String> TypeRuleList::GetRequires(void) const
{
	return m_Requires;
}

/**
 * Adds all requires from the specified rule list.
 *
 * @param ruleList The rule list to copy requires from.
 */
void TypeRuleList::AddRequires(const TypeRuleList::Ptr& ruleList)
{
	BOOST_FOREACH(const String& require, ruleList->m_Requires) {
		AddRequire(require);
	}
}

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
 * Validates a field.
 *
 * @param name The name of the attribute.
 * @param value The value of the attribute.
 * @param[out] subRules The list of sub-rules for the matching rule.
 * @param[out] hint A hint describing the validation failure.
 * @returns The validation result.
 */
TypeValidationResult TypeRuleList::ValidateAttribute(const String& name,
    const Value& value, TypeRuleList::Ptr *subRules, String *hint,
    const TypeRuleUtilities *utils) const
{
	bool foundField = false;
	BOOST_FOREACH(const TypeRule& rule, m_Rules) {
		if (!rule.MatchName(name))
			continue;

		foundField = true;

		if (rule.MatchValue(value, hint, utils)) {
			*subRules = rule.GetSubRules();
			return ValidationOK;
		}
	}

	if (foundField)
		return ValidationInvalidType;
	else
		return ValidationUnknownField;
}
