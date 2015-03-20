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

#ifndef TYPERULELIST_H
#define TYPERULELIST_H

#include "config/i2-config.hpp"
#include "base/value.hpp"
#include <vector>

namespace icinga
{

struct TypeRule;
class TypeRuleUtilities;

/**
 * @ingroup config
 */
enum TypeValidationResult
{
	ValidationOK,
	ValidationInvalidType,
	ValidationUnknownField
};

/**
 * A list of configuration type rules.
 *
 * @ingroup config
 */
class I2_CONFIG_API TypeRuleList : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(TypeRuleList);

	void AddValidator(const String& validator);
	std::vector<String> GetValidators(void) const;

	void AddRequire(const String& attr);
	void AddRequires(const TypeRuleList::Ptr& ruleList);
	std::vector<String> GetRequires(void) const;

	void AddRule(const TypeRule& rule);
	void AddRules(const TypeRuleList::Ptr& ruleList);

	TypeValidationResult ValidateAttribute(const String& name, const Value& value,
	    TypeRuleList::Ptr *subRules, String *hint, const TypeRuleUtilities *utils) const;

	size_t GetLength(void) const;

private:
	std::vector<String> m_Validators;
	std::vector<String> m_Requires;
	std::vector<TypeRule> m_Rules;
};

}

#endif /* TYPERULELIST_H */
