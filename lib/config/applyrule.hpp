/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef APPLYRULE_H
#define APPLYRULE_H

#include "config/i2-config.hpp"
#include "config/expression.hpp"
#include "base/atomic.hpp"
#include "base/debuginfo.hpp"
#include "base/shared-object.hpp"
#include "base/type.hpp"
#include <chrono>
#include <unordered_map>

namespace icinga
{

/**
 * @ingroup config
 */
class ApplyRule : public SharedObject
{
public:
	DECLARE_PTR_TYPEDEFS(ApplyRule);

	struct PerHost
	{
		std::set<ApplyRule::Ptr> ForHost;
		std::unordered_map<String /* service */, std::set<ApplyRule::Ptr>> ForServices;
	};

	struct PerSourceType
	{
		std::unordered_map<Type* /* target type */, std::vector<ApplyRule::Ptr>> Regular;
		std::unordered_map<String /* host */, PerHost> Targeted;
	};

	/*
	 * m_Rules[T::TypeInstance.get()].Targeted["H"].ForHost
	 * contains all apply rules like apply T "x" to Host { ... }
	 * which target only specific hosts incl. "H", e.g. via
	 * assign where host.name == "H" || host.name == "h".
	 *
	 * m_Rules[T::TypeInstance.get()].Targeted["H"].ForServices["S"]
	 * contains all apply rules like apply T "x" to Service { ... }
	 * which target only specific services on specific hosts,
	 * e.g. via assign where host.name == "H" && service.name == "S".
	 *
	 * m_Rules[T::TypeInstance.get()].Regular[C::TypeInstance.get()]
	 * contains all other apply rules like apply T "x" to C { ... }.
	 */
	typedef std::unordered_map<Type* /* source type */, PerSourceType> RuleMap;

	typedef std::map<String, std::vector<String> > TypeMap;

	String GetName() const;
	Expression::Ptr GetExpression() const;
	Expression::Ptr GetFilter() const;
	String GetPackage() const;
	String GetFKVar() const;
	String GetFVVar() const;
	Expression::Ptr GetFTerm() const;
	bool GetIgnoreOnError() const;
	const DebugInfo& GetDebugInfo() const;
	Dictionary::Ptr GetScope() const;
	void AddMatch();
	bool HasMatches() const;

	bool EvaluateFilter(ScriptFrame& frame) const;

	static void AddRule(const String& sourceType, const String& targetType, const String& name, const Expression::Ptr& expression,
		const Expression::Ptr& filter, const String& package, const String& fkvar, const String& fvvar, const Expression::Ptr& fterm,
		bool ignoreOnError, const DebugInfo& di, const Dictionary::Ptr& scope);
	static const std::vector<ApplyRule::Ptr>& GetRules(const Type::Ptr& sourceType, const Type::Ptr& targetType);
	static const std::set<ApplyRule::Ptr>& GetTargetedHostRules(const Type::Ptr& sourceType, const String& host);
	static const std::set<ApplyRule::Ptr>& GetTargetedServiceRules(const Type::Ptr& sourceType, const String& host, const String& service);

	static void RegisterType(const String& sourceType, const std::vector<String>& targetTypes);
	static bool IsValidSourceType(const String& sourceType);
	static bool IsValidTargetType(const String& sourceType, const String& targetType);
	static const std::vector<String>& GetTargetTypes(const String& sourceType);

	static void CheckMatches(bool silent);
	static void CheckMatches(const ApplyRule::Ptr& rule, Type* sourceType, bool silent);

private:
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

	static bool AddTargetedRule(const ApplyRule::Ptr& rule, const String& targetType, PerSourceType& rules);
	static bool GetTargetHosts(Expression* assignFilter, std::vector<const String *>& hosts);
	static bool GetTargetServices(Expression* assignFilter, std::vector<std::pair<const String *, const String *>>& services);
	static std::pair<const String *, const String *> GetTargetService(Expression* assignFilter);
	static const String * GetComparedName(Expression* assignFilter, const char * lcType);
	static bool IsNameIndexer(Expression* exp, const char * lcType);
	static const String * GetLiteralStringValue(Expression* exp);

	ApplyRule(String name, Expression::Ptr expression,
		Expression::Ptr filter, String package, String fkvar, String fvvar, Expression::Ptr fterm,
		bool ignoreOnError, DebugInfo di, Dictionary::Ptr scope);
};

class BenchmarkApplyRuleEvaluation;

class TotalTimeSpentOnApplyMismatches
{
	friend BenchmarkApplyRuleEvaluation;

public:
	TotalTimeSpentOnApplyMismatches() = default;
	TotalTimeSpentOnApplyMismatches(const TotalTimeSpentOnApplyMismatches&) = delete;
	TotalTimeSpentOnApplyMismatches(TotalTimeSpentOnApplyMismatches&&) = delete;
	TotalTimeSpentOnApplyMismatches& operator=(const TotalTimeSpentOnApplyMismatches&) = delete;
	TotalTimeSpentOnApplyMismatches& operator=(TotalTimeSpentOnApplyMismatches&&) = delete;

	double AsSeconds() const;

private:
	Atomic<std::chrono::steady_clock::rep> m_MonotonicTicks {0};
};

class BenchmarkApplyRuleEvaluation
{
public:
	inline BenchmarkApplyRuleEvaluation(TotalTimeSpentOnApplyMismatches& totalTimeSpentOnMismatches, const bool& ruleMatched)
		: m_TotalTimeSpentOnMismatches(totalTimeSpentOnMismatches),
		m_RuleMatched(ruleMatched), m_Start(std::chrono::steady_clock::now())
	{ }

	BenchmarkApplyRuleEvaluation(const BenchmarkApplyRuleEvaluation&) = delete;
	BenchmarkApplyRuleEvaluation(BenchmarkApplyRuleEvaluation&&) = delete;
	BenchmarkApplyRuleEvaluation& operator=(const BenchmarkApplyRuleEvaluation&) = delete;
	BenchmarkApplyRuleEvaluation& operator=(BenchmarkApplyRuleEvaluation&&) = delete;

	inline ~BenchmarkApplyRuleEvaluation()
	{
		if (!m_RuleMatched) {
			m_TotalTimeSpentOnMismatches.m_MonotonicTicks.fetch_add(
				(std::chrono::steady_clock::now() - m_Start).count()
			);
		}
	}

private:
	TotalTimeSpentOnApplyMismatches& m_TotalTimeSpentOnMismatches;
	const bool& m_RuleMatched;
	std::chrono::steady_clock::time_point m_Start;
};

}

#endif /* APPLYRULE_H */
