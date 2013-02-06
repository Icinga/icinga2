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

void ConfigType::ValidateItem(const ConfigItem::Ptr& item) const
{
	Dictionary::Ptr attrs = item->Link();

	/* Don't validate abstract items. */
	if (attrs->Get("__abstract"))
		return;

	vector<String> locations;
	locations.push_back("Object '" + item->GetName() + "' (Type: '" + item->GetType() + "')");

	ConfigType::Ptr parent;
	if (m_Parent.IsEmpty()) {
		if (GetName() != "DynamicObject")
			parent = ConfigCompilerContext::GetContext()->GetType("DynamicObject");
	} else {
		parent = ConfigCompilerContext::GetContext()->GetType(m_Parent);
	}

	vector<TypeRuleList::Ptr> ruleLists;
	if (parent)
		ruleLists.push_back(parent->m_RuleList);

	ruleLists.push_back(m_RuleList);

	ValidateDictionary(attrs, ruleLists, locations);
}

String ConfigType::LocationToString(const vector<String>& locations)
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
    const vector<TypeRuleList::Ptr>& ruleLists, vector<String>& locations)
{
	BOOST_FOREACH(const TypeRuleList::Ptr& ruleList, ruleLists) {
		BOOST_FOREACH(const String& require, ruleList->GetRequires()) {
			locations.push_back("Attribute '" + require + "'");

			Value value = dictionary->Get(require);

			if (value.IsEmpty()) {
				ConfigCompilerContext::GetContext()->AddError(false,
				    "Required attribute is missing: " + LocationToString(locations));
			}

			locations.pop_back();
		}

		String validator = ruleList->GetValidator();

		if (!validator.IsEmpty()) {
			ScriptFunction::Ptr func = ScriptFunction::GetByName(validator);

			if (!func)
				throw_exception(invalid_argument("Validator function '" + validator + "' does not exist."));

			vector<Value> arguments;
			arguments.push_back(LocationToString(locations));
			arguments.push_back(dictionary);

			ScriptTask::Ptr task = boost::make_shared<ScriptTask>(func, arguments);
			task->Start();
			task->Wait();
			task->GetResult();
		}
	}

	String key;
	Value value;
	BOOST_FOREACH(tie(key, value), dictionary) {
		TypeValidationResult overallResult = ValidationUnknownField;
		vector<TypeRuleList::Ptr> subRuleLists;

		locations.push_back("Attribute '" + key + "'");

		BOOST_FOREACH(const TypeRuleList::Ptr& ruleList, ruleLists) {
			TypeRuleList::Ptr subRuleList;
			TypeValidationResult result = ruleList->ValidateAttribute(key, value, &subRuleList);

			if (subRuleList)
				subRuleLists.push_back(subRuleList);

			if (result == ValidationOK) {
				overallResult = result;
				break;
			}

			if (result == ValidationInvalidType)
				overallResult = result;
		}

		if (overallResult == ValidationUnknownField)
			ConfigCompilerContext::GetContext()->AddError(true, "Unknown attribute: " + LocationToString(locations));
		else if (overallResult == ValidationInvalidType)
			ConfigCompilerContext::GetContext()->AddError(false, "Invalid type for attribute: " + LocationToString(locations));

		if (subRuleLists.size() > 0 && value.IsObjectType<Dictionary>())
			ValidateDictionary(value, subRuleLists, locations);

		locations.pop_back();
	}
}

