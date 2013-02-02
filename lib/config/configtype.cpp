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

ConfigType::TypeMap ConfigType::m_Types;

ConfigType::ConfigType(const String& name, const DebugInfo& debuginfo)
	: m_Name(name), m_RuleList(boost::make_shared<TypeRuleList>()), m_DebugInfo(debuginfo)
{ }

String ConfigType::GetName(void) const
{
	return m_Name;
}

String ConfigType::GetParent(void) const
{
	return m_Parent;
}

void ConfigType::SetParent(const String& parent)
{
	m_Parent = parent;
}

TypeRuleList::Ptr ConfigType::GetRuleList(void) const
{
	return m_RuleList;
}

DebugInfo ConfigType::GetDebugInfo(void) const
{
	return m_DebugInfo;
}

void ConfigType::ValidateObject(const DynamicObject::Ptr& object) const
{
	DynamicObject::AttributeConstIterator it;
	const DynamicObject::AttributeMap& attributes = object->GetAttributes();
	
	for (it = attributes.begin(); it != attributes.end(); it++) {
		if ((it->second.Type & Attribute_Config) == 0)
			continue;

		if (!ValidateAttribute(it->first, it->second.Data))
			Logger::Write(LogWarning, "config", "Configuration attribute '" + it->first +
			    "' on object '" + object->GetName() + "' of type '" + object->GetType()->GetName() + "' is unknown or contains an invalid type.");
	}
}

void ConfigType::ValidateDictionary(const Dictionary::Ptr& dictionary, const TypeRuleList::Ptr& ruleList)
{
	String key;
	Value value;
	BOOST_FOREACH(tie(key, value), dictionary) {
		// TODO: implement (#3619)
	}
}

bool ConfigType::ValidateAttribute(const String& name, const Value& value) const
{
	ConfigType::Ptr parent;
	
	if (m_Parent.IsEmpty()) {
		if (GetName() != "DynamicObject")
			parent = ConfigType::GetByName("DynamicObject");
	} else {
		parent = ConfigType::GetByName(m_Parent);
	}
	
	if (parent && parent->ValidateAttribute(name, value))
	    return true;
	
	TypeRuleList::Ptr subRules;

	if (!m_RuleList->FindMatch(name, value, &subRules))
		return false;
	
	if (subRules && value.IsObjectType<Dictionary>())
		ValidateDictionary(value, subRules);

	return true;
}

void ConfigType::Commit(void)
{
	m_Types[GetName()] = GetSelf();
}

ConfigType::Ptr ConfigType::GetByName(const String& name)
{
	ConfigType::TypeMap::iterator it;
	
	it = m_Types.find(name);
	
	if (it == m_Types.end())
		return ConfigType::Ptr();

	return it->second;
}
