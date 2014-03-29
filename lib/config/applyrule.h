/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-present Icinga Development Team (http://www.icinga.org) *
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

#ifndef APPLYRULE_H
#define APPLYRULE_H

#include "config/i2-config.h"
#include "config/aexpression.h"
#include "config/debuginfo.h"
#include "base/dynamictype.h"

namespace icinga
{

/**
 * @ingroup config
 */
class I2_CONFIG_API ApplyRule
{
public:
	typedef boost::function<void (const std::vector<ApplyRule>& rules)> Callback;
	typedef std::map<String, std::pair<Callback, String> > CallbackMap;
	typedef std::map<String, std::vector<ApplyRule> > RuleMap;

	String GetName(void) const;
	AExpression::Ptr GetExpression(void) const;
	AExpression::Ptr GetFilter(void) const;
	DebugInfo GetDebugInfo(void) const;
	Dictionary::Ptr GetScope(void) const;

	bool EvaluateFilter(const Dictionary::Ptr& scope) const;

	static void AddRule(const String& sourceType, const String& name, const AExpression::Ptr& expression,
	    const AExpression::Ptr& filter, const DebugInfo& di, const Dictionary::Ptr& scope);
	static void EvaluateRules(void);

	static void RegisterType(const String& sourceType, const String& targetType, const ApplyRule::Callback& callback);
	static bool IsValidType(const String& sourceType);

private:
	String m_Name;
	AExpression::Ptr m_Expression;
	AExpression::Ptr m_Filter;
	DebugInfo m_DebugInfo;
	Dictionary::Ptr m_Scope;

	static CallbackMap m_Callbacks;
	static RuleMap m_Rules;

	ApplyRule(const String& tmpl, const AExpression::Ptr& expression,
	    const AExpression::Ptr& filter, const DebugInfo& di, const Dictionary::Ptr& scope);
};

}

#endif /* APPLYRULE_H */
