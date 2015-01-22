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

#ifndef OBJECTRULE_H
#define OBJECTRULE_H

#include "config/i2-config.hpp"
#include "config/expression.hpp"
#include "base/debuginfo.hpp"
#include "base/dynamictype.hpp"

namespace icinga
{

/**
 * @ingroup config
 */
class I2_CONFIG_API ObjectRule
{
public:
	typedef boost::function<void (const std::vector<ObjectRule>& rules)> Callback;
	typedef std::map<String, Callback> CallbackMap;
	typedef std::map<String, std::vector<ObjectRule> > RuleMap;

	String GetName(void) const;
	boost::shared_ptr<Expression> GetFilter(void) const;
	DebugInfo GetDebugInfo(void) const;
	Object::Ptr GetScope(void) const;

	bool EvaluateFilter(const Object::Ptr& scope) const;

	static void AddRule(const String& sourceType, const String& name,
	    const boost::shared_ptr<Expression>& filter, const DebugInfo& di, const Object::Ptr& scope);
	static void EvaluateRules(bool clear);

	static void RegisterType(const String& sourceType, const ObjectRule::Callback& callback);
	static bool IsValidSourceType(const String& sourceType);

private:
	String m_Name;
	boost::shared_ptr<Expression> m_Filter;
	DebugInfo m_DebugInfo;
	Object::Ptr m_Scope;

	static CallbackMap m_Callbacks;
	static RuleMap m_Rules;

	ObjectRule(const String& name, const boost::shared_ptr<Expression>& filter, const DebugInfo& di, const Object::Ptr& scope);
};

}

#endif /* OBJECTRULE_H */
