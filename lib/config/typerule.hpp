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

#ifndef TYPERULE_H
#define TYPERULE_H

#include "config/i2-config.hpp"
#include "config/typerulelist.hpp"
#include "base/debuginfo.hpp"

namespace icinga
{

/**
 * Utilities for type rules.
 *
 * @ingroup config
 */
class I2_CONFIG_API TypeRuleUtilities
{
public:
	virtual bool ValidateName(const String& type, const String& name, String *hint) const;
};

/**
 * The allowed type for a type rule.
 *
 * @ingroup config
 */
enum TypeSpecifier
{
	TypeAny,
	TypeScalar,
	TypeNumber,
	TypeString,
	TypeDictionary,
	TypeArray,
	TypeFunction,
	TypeName
};

/**
 * A configuration type rule.
 *
 * @ingroup config
 */
struct I2_CONFIG_API TypeRule
{
public:
	TypeRule(TypeSpecifier type, const String& nameType,
	    const String& namePattern, const TypeRuleList::Ptr& subRules,
	    const DebugInfo& debuginfo);

	TypeRuleList::Ptr GetSubRules(void) const;

	bool MatchName(const String& name) const;
	bool MatchValue(const Value& value, String *hint, const TypeRuleUtilities *utils) const;

private:
	TypeSpecifier m_Type;
	String m_NameType;
	String m_NamePattern;
	TypeRuleList::Ptr m_SubRules;
	DebugInfo m_DebugInfo;
};

}

#endif /* TYPERULE_H */
