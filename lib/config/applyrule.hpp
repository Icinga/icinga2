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

	String GetTargetType() const;
	String GetName() const;
	std::shared_ptr<Expression> GetExpression() const;
	std::shared_ptr<Expression> GetFilter() const;
	String GetPackage() const;
	String GetFKVar() const;
	String GetFVVar() const;
	std::shared_ptr<Expression> GetFTerm() const;
	bool GetIgnoreOnError() const;
	DebugInfo GetDebugInfo() const;
	Dictionary::Ptr GetScope() const;
	void AddMatch();
	bool HasMatches() const;

	bool EvaluateFilter(ScriptFrame& frame) const;

	static void AddRule(const String& sourceType, const String& targetType, const String& name, const std::shared_ptr<Expression>& expression,
		const std::shared_ptr<Expression>& filter, const String& package, const String& fkvar, const String& fvvar, const std::shared_ptr<Expression>& fterm,
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
	std::shared_ptr<Expression> m_Expression;
	std::shared_ptr<Expression> m_Filter;
	String m_Package;
	String m_FKVar;
	String m_FVVar;
	std::shared_ptr<Expression> m_FTerm;
	bool m_IgnoreOnError;
	DebugInfo m_DebugInfo;
	Dictionary::Ptr m_Scope;
	bool m_HasMatches;

	static TypeMap m_Types;
	static RuleMap m_Rules;

	ApplyRule(String targetType, String name, std::shared_ptr<Expression> expression,
		std::shared_ptr<Expression> filter, String package, String fkvar, String fvvar, std::shared_ptr<Expression> fterm,
		bool ignoreOnError, DebugInfo di, Dictionary::Ptr scope);
};

}

#endif /* APPLYRULE_H */
