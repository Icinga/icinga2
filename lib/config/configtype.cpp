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

#include "config/configtype.hpp"
#include "config/configcompilercontext.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/singleton.hpp"
#include "base/scriptfunction.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

ConfigType::ConfigType(const String& name, const DebugInfo& debuginfo)
	: m_Name(name), m_RuleList(new TypeRuleList()), m_DebugInfo(debuginfo)
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

void ConfigType::AddParentRules(std::vector<TypeRuleList::Ptr>& ruleLists, const ConfigType::Ptr& item)
{
	ConfigType::Ptr parent;
	if (item->m_Parent.IsEmpty()) {
		if (item->GetName() != "DynamicObject")
			parent = ConfigType::GetByName("DynamicObject");
	} else {
		parent = ConfigType::GetByName(item->m_Parent);
	}

	if (parent) {
		AddParentRules(ruleLists, parent);
		ruleLists.push_back(parent->m_RuleList);
	}
}

void ConfigType::ValidateItem(const String& name, const Dictionary::Ptr& attrs, const DebugInfo& debugInfo, const TypeRuleUtilities *utils)
{
	String location = "Object '" + name + "' (Type: '" + GetName() + "')";

	if (!debugInfo.Path.IsEmpty())
		location += " at " + debugInfo.Path + ":" + Convert::ToString(debugInfo.FirstLine);
	
	std::vector<String> locations;
	locations.push_back(location);

	std::vector<TypeRuleList::Ptr> ruleLists;
	AddParentRules(ruleLists, this);
	ruleLists.push_back(m_RuleList);

	ValidateDictionary(attrs, ruleLists, locations, utils);
}

String ConfigType::LocationToString(const std::vector<String>& locations)
{
	bool first = true;
	String stack;
	BOOST_FOREACH(const String& location, locations) {
		if (!first)
			stack += " -> ";
		else
			first = false;

		stack += location;
	}

	return stack;
}

void ConfigType::ValidateDictionary(const Dictionary::Ptr& dictionary,
    const std::vector<TypeRuleList::Ptr>& ruleLists, std::vector<String>& locations,
    const TypeRuleUtilities *utils)
{
	BOOST_FOREACH(const TypeRuleList::Ptr& ruleList, ruleLists) {
		BOOST_FOREACH(const String& require, ruleList->GetRequires()) {
			locations.push_back("Attribute '" + require + "'");

			Value value = dictionary->Get(require);

			if (value.IsEmpty() || (value.IsString() && static_cast<String>(value).IsEmpty())) {
				ConfigCompilerContext::GetInstance()->AddMessage(true,
				    "Required attribute is missing: " + LocationToString(locations));
			}

			locations.pop_back();
		}

		String validator = ruleList->GetValidator();

		if (!validator.IsEmpty()) {
			ScriptFunction::Ptr func = ScriptFunction::GetByName(validator);

			if (!func)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Validator function '" + validator + "' does not exist."));

			std::vector<Value> arguments;
			arguments.push_back(LocationToString(locations));
			arguments.push_back(dictionary);

			func->Invoke(arguments);
		}
	}

	ObjectLock olock(dictionary);

	BOOST_FOREACH(const Dictionary::Pair& kv, dictionary) {
		TypeValidationResult overallResult = ValidationUnknownField;
		std::vector<TypeRuleList::Ptr> subRuleLists;
		String hint;

		locations.push_back("Attribute '" + kv.first + "'");

		BOOST_FOREACH(const TypeRuleList::Ptr& ruleList, ruleLists) {
			TypeRuleList::Ptr subRuleList;
			TypeValidationResult result = ruleList->ValidateAttribute(kv.first, kv.second, &subRuleList, &hint, utils);

			if (subRuleList)
				subRuleLists.push_back(subRuleList);

			if (overallResult == ValidationOK)
				continue;

			if (result == ValidationOK) {
				overallResult = result;
				continue;
			}

			if (result == ValidationInvalidType)
				overallResult = result;
		}

		if (overallResult == ValidationUnknownField)
			ConfigCompilerContext::GetInstance()->AddMessage(true, "Unknown attribute: " + LocationToString(locations));
		else if (overallResult == ValidationInvalidType) {
			String message = "Invalid value for attribute: " + LocationToString(locations);

			if (!hint.IsEmpty())
				message += ": " + hint;

			ConfigCompilerContext::GetInstance()->AddMessage(true, message);
		}

		if (!subRuleLists.empty() && kv.second.IsObjectType<Dictionary>())
			ValidateDictionary(kv.second, subRuleLists, locations, utils);
		else if (!subRuleLists.empty() && kv.second.IsObjectType<Array>())
			ValidateArray(kv.second, subRuleLists, locations, utils);

		locations.pop_back();
	}
}

void ConfigType::ValidateArray(const Array::Ptr& array,
    const std::vector<TypeRuleList::Ptr>& ruleLists, std::vector<String>& locations,
    const TypeRuleUtilities *utils)
{
	BOOST_FOREACH(const TypeRuleList::Ptr& ruleList, ruleLists) {
		BOOST_FOREACH(const String& require, ruleList->GetRequires()) {
			size_t index = Convert::ToLong(require);

			locations.push_back("Attribute '" + require + "'");

			if (array->GetLength() < index) {
				ConfigCompilerContext::GetInstance()->AddMessage(true,
				    "Required array index is missing: " + LocationToString(locations));
			}

			locations.pop_back();
		}

		String validator = ruleList->GetValidator();

		if (!validator.IsEmpty()) {
			ScriptFunction::Ptr func = ScriptFunction::GetByName(validator);

			if (!func)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Validator function '" + validator + "' does not exist."));

			std::vector<Value> arguments;
			arguments.push_back(LocationToString(locations));
			arguments.push_back(array);

			func->Invoke(arguments);
		}
	}

	ObjectLock olock(array);

	int index = 0;
	String key;
	BOOST_FOREACH(const Value& value, array) {
		key = Convert::ToString(index);
		index++;

		TypeValidationResult overallResult = ValidationUnknownField;
		std::vector<TypeRuleList::Ptr> subRuleLists;
		String hint;

		locations.push_back("Index " + key);

		BOOST_FOREACH(const TypeRuleList::Ptr& ruleList, ruleLists) {
			TypeRuleList::Ptr subRuleList;
			TypeValidationResult result = ruleList->ValidateAttribute(key, value, &subRuleList, &hint, utils);

			if (subRuleList)
				subRuleLists.push_back(subRuleList);

			if (overallResult == ValidationOK)
				continue;

			if (result == ValidationOK) {
				overallResult = result;
				continue;
			}

			if (result == ValidationInvalidType)
				overallResult = result;
		}

		if (overallResult == ValidationUnknownField)
			ConfigCompilerContext::GetInstance()->AddMessage(true, "Unknown attribute: " + LocationToString(locations));
		else if (overallResult == ValidationInvalidType) {
			String message = "Invalid value for array index: " + LocationToString(locations);

			if (!hint.IsEmpty())
				message += ": " + hint;

			ConfigCompilerContext::GetInstance()->AddMessage(true, message);
		}

		if (!subRuleLists.empty() && value.IsObjectType<Dictionary>())
			ValidateDictionary(value, subRuleLists, locations, utils);
		else if (!subRuleLists.empty() && value.IsObjectType<Array>())
			ValidateArray(value, subRuleLists, locations, utils);

		locations.pop_back();
	}
}

void ConfigType::Register(void)
{
	ConfigTypeRegistry::GetInstance()->Register(GetName(), this);
}

ConfigType::Ptr ConfigType::GetByName(const String& name)
{
	return ConfigTypeRegistry::GetInstance()->GetItem(name);
}

ConfigTypeRegistry::ItemMap ConfigType::GetTypes(void)
{
	return ConfigTypeRegistry::GetInstance()->GetItems();
}

void ConfigType::DiscardTypes(void)
{
	ConfigTypeRegistry::GetInstance()->Clear();
}

ConfigTypeRegistry *ConfigTypeRegistry::GetInstance(void)
{
	return Singleton<ConfigTypeRegistry>::GetInstance();
}
