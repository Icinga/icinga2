/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "config/i2-config.hpp"
#include "config/expression.hpp"
#include "base/debuginfo.hpp"

namespace icinga
{

/**
 * @ingroup config
 */
class I2_CONFIG_API ApplyRule
{
public:
	typedef std::map<String, std::vector<String> > TypeMap;
	typedef std::map<String, std::vector<ApplyRule> > RuleMap;

	String GetTargetType(void) const;
	String GetName(void) const;
	boost::shared_ptr<Expression> GetExpression(void) const;
	boost::shared_ptr<Expression> GetFilter(void) const;
	String GetPackage(void) const;
	String GetFKVar(void) const;
	String GetFVVar(void) const;
	boost::shared_ptr<Expression> GetFTerm(void) const;
	bool GetIgnoreOnError(void) const;
	DebugInfo GetDebugInfo(void) const;
	Dictionary::Ptr GetScope(void) const;
	void AddMatch(void);
	bool HasMatches(void) const;

	bool EvaluateFilter(ScriptFrame& frame) const;

	static void AddRule(const String& sourceType, const String& targetType, const String& name, const boost::shared_ptr<Expression>& expression,
	    const boost::shared_ptr<Expression>& filter, const String& package, const String& fkvar, const String& fvvar, const boost::shared_ptr<Expression>& fterm,
	    bool ignoreOnError, const DebugInfo& di, const Dictionary::Ptr& scope);
	static std::vector<ApplyRule>& GetRules(const String& type);

	static void RegisterType(const String& sourceType, const std::vector<String>& targetTypes);
	static bool IsValidSourceType(const String& sourceType);
	static bool IsValidTargetType(const String& sourceType, const String& targetType);
	static std::vector<String> GetTargetTypes(const String& sourceType);

	static void CheckMatches(void);

private:
	String m_TargetType;
	String m_Name;
	boost::shared_ptr<Expression> m_Expression;
	boost::shared_ptr<Expression> m_Filter;
	String m_Package;
	String m_FKVar;
	String m_FVVar;
	boost::shared_ptr<Expression> m_FTerm;
	bool m_IgnoreOnError;
	DebugInfo m_DebugInfo;
	Dictionary::Ptr m_Scope;
	bool m_HasMatches;

	static TypeMap m_Types;
	static RuleMap m_Rules;

	ApplyRule(const String& targetType, const String& name, const boost::shared_ptr<Expression>& expression,
	    const boost::shared_ptr<Expression>& filter, const String& package, const String& fkvar, const String& fvvar, const boost::shared_ptr<Expression>& fterm,
	    bool ignoreOnError, const DebugInfo& di, const Dictionary::Ptr& scope);
};

}

#endif /* APPLYRULE_H */
