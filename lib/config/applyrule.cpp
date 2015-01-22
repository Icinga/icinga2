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

#include "config/applyrule.hpp"
#include "base/logger.hpp"
#include <boost/foreach.hpp>
#include <set>

using namespace icinga;

ApplyRule::RuleMap ApplyRule::m_Rules;
ApplyRule::CallbackMap ApplyRule::m_Callbacks;

ApplyRule::ApplyRule(const String& targetType, const String& name, const boost::shared_ptr<Expression>& expression,
    const boost::shared_ptr<Expression>& filter, const String& fkvar, const String& fvvar, const boost::shared_ptr<Expression>& fterm,
    const DebugInfo& di, const Object::Ptr& scope)
	: m_TargetType(targetType), m_Name(name), m_Expression(expression), m_Filter(filter), m_FKVar(fkvar),
	  m_FVVar(fvvar), m_FTerm(fterm), m_DebugInfo(di), m_Scope(scope)
{ }

String ApplyRule::GetTargetType(void) const
{
	return m_TargetType;
}

String ApplyRule::GetName(void) const
{
	return m_Name;
}

boost::shared_ptr<Expression> ApplyRule::GetExpression(void) const
{
	return m_Expression;
}

boost::shared_ptr<Expression> ApplyRule::GetFilter(void) const
{
	return m_Filter;
}

String ApplyRule::GetFKVar(void) const
{
	return m_FKVar;
}

String ApplyRule::GetFVVar(void) const
{
	return m_FVVar;
}

boost::shared_ptr<Expression> ApplyRule::GetFTerm(void) const
{
	return m_FTerm;
}

DebugInfo ApplyRule::GetDebugInfo(void) const
{
	return m_DebugInfo;
}

Object::Ptr ApplyRule::GetScope(void) const
{
	return m_Scope;
}

void ApplyRule::AddRule(const String& sourceType, const String& targetType, const String& name,
    const boost::shared_ptr<Expression>& expression, const boost::shared_ptr<Expression>& filter, const String& fkvar,
    const String& fvvar, const boost::shared_ptr<Expression>& fterm, const DebugInfo& di, const Object::Ptr& scope)
{
	m_Rules[sourceType].push_back(ApplyRule(targetType, name, expression, filter, fkvar, fvvar, fterm, di, scope));
}

bool ApplyRule::EvaluateFilter(const Object::Ptr& scope) const
{
	return m_Filter->Evaluate(scope).ToBool();
}

void ApplyRule::EvaluateRules(bool clear)
{
	std::set<String> completedTypes;

	while (completedTypes.size() < m_Callbacks.size()) {
		std::pair<String, std::pair<Callback, std::vector<String> > > kv;
		BOOST_FOREACH(kv, m_Callbacks) {
			const String& sourceType = kv.first;

			if (completedTypes.find(sourceType) != completedTypes.end())
				continue;

			const Callback& callback = kv.second.first;
			const std::vector<String>& targetTypes = kv.second.second;

			bool cont = false;

			BOOST_FOREACH(const String& targetType, targetTypes) {
				if (IsValidSourceType(targetType) && completedTypes.find(targetType) == completedTypes.end()) {
					cont = true;
					break;
				}
			}

			if (cont)
				continue;

			completedTypes.insert(sourceType);

			RuleMap::const_iterator it = m_Rules.find(kv.first);

			if (it == m_Rules.end())
				continue;

			callback(it->second);
		}
	}

	if (clear)
		m_Rules.clear();
}

void ApplyRule::RegisterType(const String& sourceType, const std::vector<String>& targetTypes, const ApplyRule::Callback& callback)
{
	m_Callbacks[sourceType] = make_pair(callback, targetTypes);
}

bool ApplyRule::IsValidSourceType(const String& sourceType)
{
	return m_Callbacks.find(sourceType) != m_Callbacks.end();
}

bool ApplyRule::IsValidTargetType(const String& sourceType, const String& targetType)
{
	CallbackMap::const_iterator it = m_Callbacks.find(sourceType);

	if (it == m_Callbacks.end())
		return false;

	if (it->second.second.size() == 1 && targetType == "")
		return true;

	BOOST_FOREACH(const String& type, it->second.second) {
		if (type == targetType)
			return true;
	}

	return false;
}

std::vector<String> ApplyRule::GetTargetTypes(const String& sourceType)
{
	CallbackMap::const_iterator it = m_Callbacks.find(sourceType);

	if (it == m_Callbacks.end())
		return std::vector<String>();

	return it->second.second;
}

