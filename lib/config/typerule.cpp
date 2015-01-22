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

#include "config/typerule.hpp"
#include "config/configitem.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/dictionary.hpp"
#include "base/array.hpp"

using namespace icinga;

TypeRule::TypeRule(TypeSpecifier type, const String& nameType,
    const String& namePattern, const TypeRuleList::Ptr& subRules,
    const DebugInfo& debuginfo)
	: m_Type(type), m_NameType(nameType), m_NamePattern(namePattern), m_SubRules(subRules), m_DebugInfo(debuginfo)
{ }

TypeRuleList::Ptr TypeRule::GetSubRules(void) const
{
	return m_SubRules;
}

bool TypeRule::MatchName(const String& name) const
{
	return (Utility::Match(m_NamePattern, name));
}

bool TypeRule::MatchValue(const Value& value, String *hint, const TypeRuleUtilities *utils) const
{
	ConfigItem::Ptr item;

	if (value.IsEmpty())
		return true;

	switch (m_Type) {
		case TypeAny:
			return true;

		case TypeString:
			/* fall through; any scalar can be converted to a string */
		case TypeScalar:
			return value.IsScalar();

		case TypeNumber:
			try {
				Convert::ToDouble(value);
			} catch (...) {
				return false;
			}

			return true;

		case TypeDictionary:
			return value.IsObjectType<Dictionary>();

		case TypeArray:
			return value.IsObjectType<Array>();

		case TypeName:
			if (!value.IsScalar())
				return false;

			return utils->ValidateName(m_NameType, value, hint);

		default:
			return false;
	}
}

bool TypeRuleUtilities::ValidateName(const String& type, const String& name, String *hint) const
{
	if (name.IsEmpty())
		return true;

	ConfigItem::Ptr item = ConfigItem::GetObject(type, name);

	if (!item) {
		*hint = "Object '" + name + "' of type '" + type + "' does not exist.";
		return false;
	}

	if (item->IsAbstract()) {
		*hint = "Object '" + name + "' of type '" + type + "' must not be a template.";
		return false;
	}

	return true;
}
