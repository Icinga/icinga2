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

#include "config/objectrule.hpp"
#include <boost/foreach.hpp>
#include <set>

using namespace icinga;

ObjectRule::RuleMap ObjectRule::m_Rules;
ObjectRule::CallbackMap ObjectRule::m_Callbacks;

ObjectRule::ObjectRule(const String& name, const boost::shared_ptr<Expression>& filter,
    const DebugInfo& di, const Object::Ptr& scope)
	: m_Name(name), m_Filter(filter), m_DebugInfo(di), m_Scope(scope)
{ }

String ObjectRule::GetName(void) const
{
	return m_Name;
}

boost::shared_ptr<Expression> ObjectRule::GetFilter(void) const
{
	return m_Filter;
}

DebugInfo ObjectRule::GetDebugInfo(void) const
{
	return m_DebugInfo;
}

Object::Ptr ObjectRule::GetScope(void) const
{
	return m_Scope;
}

void ObjectRule::AddRule(const String& sourceType, const String& name,
    const boost::shared_ptr<Expression>& filter, const DebugInfo& di, const Object::Ptr& scope)
{
	m_Rules[sourceType].push_back(ObjectRule(name, filter, di, scope));
}

bool ObjectRule::EvaluateFilter(const Object::Ptr& scope) const
{
	return m_Filter->Evaluate(scope).ToBool();
}

void ObjectRule::EvaluateRules(bool clear)
{
	std::pair<String, Callback> kv;
	BOOST_FOREACH(kv, m_Callbacks) {
		const Callback& callback = kv.second;

		RuleMap::const_iterator it = m_Rules.find(kv.first);

		if (it == m_Rules.end())
			continue;

		callback(it->second);
	}

	if (clear)
		m_Rules.clear();
}

void ObjectRule::RegisterType(const String& sourceType, const ObjectRule::Callback& callback)
{
	m_Callbacks[sourceType] = callback;
}

bool ObjectRule::IsValidSourceType(const String& sourceType)
{
	return m_Callbacks.find(sourceType) != m_Callbacks.end();
}

