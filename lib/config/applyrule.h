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
	typedef std::pair<String, String> TypeCombination;
	typedef boost::function<void (const std::vector<ApplyRule>& rules)> Callback;
	typedef std::map<TypeCombination, Callback> CallbackMap;
	typedef std::map<TypeCombination, std::vector<ApplyRule> > RuleMap;

	String GetTemplate(void) const;
	AExpression::Ptr GetExpression(void) const;
	DebugInfo GetDebugInfo(void) const;

	static void AddRule(const String& sourceType, const String& tmpl, const String& targetType, const AExpression::Ptr& expression, const DebugInfo& di);
	static void EvaluateRules(void);

	static void RegisterCombination(const String& sourceType, const String& targetType, const ApplyRule::Callback& callback);
	static bool IsValidCombination(const String& sourceType, const String& targetType);

private:
	String m_Template;
	AExpression::Ptr m_Expression;
	DebugInfo m_DebugInfo;

	static CallbackMap m_Callbacks;
	static RuleMap m_Rules;

	ApplyRule(const String& tmpl, const AExpression::Ptr& expression, const DebugInfo& di);
};

}

#endif /* APPLYRULE_H */
