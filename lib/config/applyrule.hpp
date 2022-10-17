/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
class ApplyRule
{
public:
	typedef std::map<String, std::vector<String> > TypeMap;
	typedef std::map<String, std::vector<ApplyRule> > RuleMap;

	const String& GetTargetType() const
	{
		return m_TargetType;
	}

	String GetName() const;
	Expression::Ptr GetExpression() const;
	Expression::Ptr GetFilter() const;
	String GetPackage() const;
	String GetFKVar() const;
	String GetFVVar() const;
	Expression::Ptr GetFTerm() const;
	bool GetIgnoreOnError() const;
	DebugInfo GetDebugInfo() const;
	Dictionary::Ptr GetScope() const;
	void AddMatch();
	bool HasMatches() const;

	bool EvaluateFilter(ScriptFrame& frame) const;

	static void AddRule(const String& sourceType, const String& targetType, const String& name, const Expression::Ptr& expression,
		const Expression::Ptr& filter, const String& package, const String& fkvar, const String& fvvar, const Expression::Ptr& fterm,
		bool ignoreOnError, const DebugInfo& di, const Dictionary::Ptr& scope);
	static std::vector<ApplyRule>& GetRules(const String& type);

	static void RegisterType(const String& sourceType, const std::vector<String>& targetTypes);
	static bool IsValidSourceType(const String& sourceType);
	static bool IsValidTargetType(const String& sourceType, const String& targetType);
	static std::vector<String> GetTargetTypes(const String& sourceType);

	static void CheckMatches(bool silent);

private:
	String m_TargetType;
	String m_Name;
	Expression::Ptr m_Expression;
	Expression::Ptr m_Filter;
	String m_Package;
	String m_FKVar;
	String m_FVVar;
	Expression::Ptr m_FTerm;
	bool m_IgnoreOnError;
	DebugInfo m_DebugInfo;
	Dictionary::Ptr m_Scope;
	bool m_HasMatches;

	static TypeMap m_Types;
	static RuleMap m_Rules;

	ApplyRule(String targetType, String name, Expression::Ptr expression,
		Expression::Ptr filter, String package, String fkvar, String fvvar, Expression::Ptr fterm,
		bool ignoreOnError, DebugInfo di, Dictionary::Ptr scope);
};

}

#endif /* APPLYRULE_H */
