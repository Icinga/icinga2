/* Icinga 2 | (c) 2022 Icinga GmbH | GPLv2+ */

#include "base/string.hpp"
#include "config/applyrule.hpp"
#include "config/expression.hpp"
#include <utility>
#include <vector>

using namespace icinga;

/**
 * @returns All ApplyRules targeting only specific parent objects including the given host. (See AddTargetedRule().)
 */
const std::vector<ApplyRule::Ptr>& ApplyRule::GetTargetedHostRules(const Type::Ptr& sourceType, const String& host)
{
	auto perSourceType (m_Rules.find(sourceType.get()));

	if (perSourceType != m_Rules.end()) {
		auto perHost (perSourceType->second.Targeted.find(host));

		if (perHost != perSourceType->second.Targeted.end()) {
			return perHost->second.ForHost;
		}
	}

	static const std::vector<ApplyRule::Ptr> noRules;
	return noRules;
}

/**
 * @returns All ApplyRules targeting only specific parent objects including the given service. (See AddTargetedRule().)
 */
const std::vector<ApplyRule::Ptr>& ApplyRule::GetTargetedServiceRules(const Type::Ptr& sourceType, const String& host, const String& service)
{
	auto perSourceType (m_Rules.find(sourceType.get()));

	if (perSourceType != m_Rules.end()) {
		auto perHost (perSourceType->second.Targeted.find(host));

		if (perHost != perSourceType->second.Targeted.end()) {
			auto perService (perHost->second.ForServices.find(service));

			if (perService != perHost->second.ForServices.end()) {
				return perService->second;
			}
		}
	}

	static const std::vector<ApplyRule::Ptr> noRules;
	return noRules;
}

/**
 * If the given ApplyRule targets only specific parent objects, add it to the respective "index".
 *
 * - The above means for apply T "N" to Host: assign where host.name == "H" [ || host.name == "h" ... ]
 * - For apply T "N" to Service it means: assign where host.name == "H" && service.name == "S" [ || host.name == "h" && service.name == "s" ... ]
 *
 * The order of operands of || && == doesn't matter.
 *
 * @returns Whether the rule has been added to the "index".
 */
bool ApplyRule::AddTargetedRule(const ApplyRule::Ptr& rule, const String& targetType, ApplyRule::PerSourceType& rules)
{
	if (targetType == "Host") {
		std::vector<const String *> hosts;

		if (GetTargetHosts(rule->m_Filter.get(), hosts)) {
			for (auto host : hosts) {
				rules.Targeted[*host].ForHost.emplace_back(rule);
			}

			return true;
		}
	} else if (targetType == "Service") {
		std::vector<std::pair<const String *, const String *>> services;

		if (GetTargetServices(rule->m_Filter.get(), services)) {
			for (auto service : services) {
				rules.Targeted[*service.first].ForServices[*service.second].emplace_back(rule);
			}

			return true;
		}
	}

	return false;
}

/**
 * If the given assign filter is like the following, extract the host names ("H", "h", ...) into the vector:
 *
 * host.name == "H" [ || host.name == "h" ... ]
 *
 * The order of operands of || == doesn't matter.
 *
 * @returns Whether the given assign filter is like above.
 */
bool ApplyRule::GetTargetHosts(Expression* assignFilter, std::vector<const String *>& hosts)
{
	auto lor (dynamic_cast<LogicalOrExpression*>(assignFilter));

	if (lor) {
		return GetTargetHosts(lor->GetOperand1().get(), hosts)
			&& GetTargetHosts(lor->GetOperand2().get(), hosts);
	}

	auto name (GetComparedName(assignFilter, "host"));

	if (name) {
		hosts.emplace_back(name);
		return true;
	}

	return false;
}

/**
 * If the given assign filter is like the following, extract the host+service names ("H"+"S", "h"+"s", ...) into the vector:
 *
 * host.name == "H" && service.name == "S" [ || host.name == "h" && service.name == "s" ... ]
 *
 * The order of operands of || && == doesn't matter.
 *
 * @returns Whether the given assign filter is like above.
 */
bool ApplyRule::GetTargetServices(Expression* assignFilter, std::vector<std::pair<const String *, const String *>>& services)
{
	auto lor (dynamic_cast<LogicalOrExpression*>(assignFilter));

	if (lor) {
		return GetTargetServices(lor->GetOperand1().get(), services)
			&& GetTargetServices(lor->GetOperand2().get(), services);
	}

	auto service (GetTargetService(assignFilter));

	if (service.first) {
		services.emplace_back(service);
		return true;
	}

	return false;
}

/**
 * If the given filter is like the following, extract the host+service names ("H"+"S"):
 *
 * host.name == "H" && service.name == "S"
 *
 * The order of operands of && == doesn't matter.
 *
 * @returns {host, service} on success and {nullptr, nullptr} on failure.
 */
std::pair<const String *, const String *> ApplyRule::GetTargetService(Expression* assignFilter)
{
	auto land (dynamic_cast<LogicalAndExpression*>(assignFilter));

	if (!land) {
		return {nullptr, nullptr};
	}

	auto op1 (land->GetOperand1().get());
	auto op2 (land->GetOperand2().get());
	auto host (GetComparedName(op1, "host"));

	if (!host) {
		std::swap(op1, op2);
		host = GetComparedName(op1, "host");
	}

	if (host) {
		auto service (GetComparedName(op2, "service"));

		if (service) {
			return {host, service};
		}
	}

	return {nullptr, nullptr};
}

/**
 * If the given filter is like the following, extract the object name ("N"):
 *
 * $lcType$.name == "N"
 *
 * The order of operands of == doesn't matter.
 *
 * @returns The object name on success and nullptr on failure.
 */
const String * ApplyRule::GetComparedName(Expression* assignFilter, const char * lcType)
{
	auto eq (dynamic_cast<EqualExpression*>(assignFilter));

	if (!eq) {
		return nullptr;
	}

	auto op1 (eq->GetOperand1().get());
	auto op2 (eq->GetOperand2().get());

	if (IsNameIndexer(op1, lcType)) {
		return GetLiteralStringValue(op2);
	}

	if (IsNameIndexer(op2, lcType)) {
		return GetLiteralStringValue(op1);
	}

	return nullptr;
}

/**
 * @returns Whether the given expression is like $lcType$.name.
 */
bool ApplyRule::IsNameIndexer(Expression* exp, const char * lcType)
{
	auto ixr (dynamic_cast<IndexerExpression*>(exp));

	if (!ixr) {
		return false;
	}

	auto var (dynamic_cast<VariableExpression*>(ixr->GetOperand1().get()));

	if (!var || var->GetVariable() != lcType) {
		return false;
	}

	auto val (GetLiteralStringValue(ixr->GetOperand2().get()));

	return val && *val == "name";
}

/**
 * @returns If the given expression is a string literal, the string. nullptr on failure.
 */
const String * ApplyRule::GetLiteralStringValue(Expression* exp)
{
	auto lit (dynamic_cast<LiteralExpression*>(exp));

	if (!lit) {
		return nullptr;
	}

	auto& val (lit->GetValue());

	if (!val.IsString()) {
		return nullptr;
	}

	return &val.Get<String>();
}
