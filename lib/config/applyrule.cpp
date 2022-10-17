/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "config/applyrule.hpp"
#include "base/logger.hpp"
#include <set>

using namespace icinga;

ApplyRule::RuleMap ApplyRule::m_Rules;
ApplyRule::TypeMap ApplyRule::m_Types;

ApplyRule::ApplyRule(String targetType, String name, Expression::Ptr expression,
	Expression::Ptr filter, String package, String fkvar, String fvvar, Expression::Ptr fterm,
	bool ignoreOnError, DebugInfo di, Dictionary::Ptr scope)
	: m_TargetType(std::move(targetType)), m_Name(std::move(name)), m_Expression(std::move(expression)), m_Filter(std::move(filter)), m_Package(std::move(package)), m_FKVar(std::move(fkvar)),
	m_FVVar(std::move(fvvar)), m_FTerm(std::move(fterm)), m_IgnoreOnError(ignoreOnError), m_DebugInfo(std::move(di)), m_Scope(std::move(scope)), m_HasMatches(false)
{ }

String ApplyRule::GetTargetType() const
{
	return m_TargetType;
}

String ApplyRule::GetName() const
{
	return m_Name;
}

Expression::Ptr ApplyRule::GetExpression() const
{
	return m_Expression;
}

Expression::Ptr ApplyRule::GetFilter() const
{
	return m_Filter;
}

String ApplyRule::GetPackage() const
{
	return m_Package;
}

String ApplyRule::GetFKVar() const
{
	return m_FKVar;
}

String ApplyRule::GetFVVar() const
{
	return m_FVVar;
}

Expression::Ptr ApplyRule::GetFTerm() const
{
	return m_FTerm;
}

bool ApplyRule::GetIgnoreOnError() const
{
	return m_IgnoreOnError;
}

DebugInfo ApplyRule::GetDebugInfo() const
{
	return m_DebugInfo;
}

Dictionary::Ptr ApplyRule::GetScope() const
{
	return m_Scope;
}

void ApplyRule::AddRule(const String& sourceType, const String& targetType, const String& name,
	const Expression::Ptr& expression, const Expression::Ptr& filter, const String& package, const String& fkvar,
	const String& fvvar, const Expression::Ptr& fterm, bool ignoreOnError, const DebugInfo& di, const Dictionary::Ptr& scope)
{
	auto actualTargetType (&targetType);

	if (*actualTargetType == "") {
		auto& targetTypes (GetTargetTypes(sourceType));

		if (targetTypes.size() == 1u) {
			actualTargetType = &targetTypes[0];
		}
	}

	m_Rules[Type::GetByName(sourceType).get()][Type::GetByName(*actualTargetType).get()].emplace_back(ApplyRule(
		targetType, name, expression, filter, package, fkvar, fvvar, fterm, ignoreOnError, di, scope
	));
}

bool ApplyRule::EvaluateFilter(ScriptFrame& frame) const
{
	return Convert::ToBool(m_Filter->Evaluate(frame));
}

void ApplyRule::RegisterType(const String& sourceType, const std::vector<String>& targetTypes)
{
	m_Types[sourceType] = targetTypes;
}

bool ApplyRule::IsValidSourceType(const String& sourceType)
{
	return m_Types.find(sourceType) != m_Types.end();
}

bool ApplyRule::IsValidTargetType(const String& sourceType, const String& targetType)
{
	auto it = m_Types.find(sourceType);

	if (it == m_Types.end())
		return false;

	if (it->second.size() == 1 && targetType == "")
		return true;

	for (const String& type : it->second) {
		if (type == targetType)
			return true;
	}

	return false;
}

const std::vector<String>& ApplyRule::GetTargetTypes(const String& sourceType)
{
	auto it = m_Types.find(sourceType);

	if (it == m_Types.end()) {
		static const std::vector<String> noTypes;
		return noTypes;
	}

	return it->second;
}

void ApplyRule::AddMatch()
{
	m_HasMatches = true;
}

bool ApplyRule::HasMatches() const
{
	return m_HasMatches;
}

std::vector<ApplyRule>& ApplyRule::GetRules(const Type::Ptr& sourceType, const Type::Ptr& targetType)
{
	auto perSourceType (m_Rules.find(sourceType.get()));

	if (perSourceType != m_Rules.end()) {
		auto perTargetType (perSourceType->second.find(targetType.get()));

		if (perTargetType != perSourceType->second.end()) {
			return perTargetType->second;
		}
	}

	static std::vector<ApplyRule> noRules;
	return noRules;
}

void ApplyRule::CheckMatches(bool silent)
{
	for (auto& perSourceType : m_Rules) {
		for (auto& perTargetType : perSourceType.second) {
			for (auto& rule : perTargetType.second) {
				if (!rule.HasMatches() && !silent) {
					Log(LogWarning, "ApplyRule")
						<< "Apply rule '" << rule.GetName() << "' (" << rule.GetDebugInfo() << ") for type '"
						<< perSourceType.first->GetName() << "' does not match anywhere!";
				}
			}
		}
	}
}
