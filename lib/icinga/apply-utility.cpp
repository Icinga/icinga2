// SPDX-FileCopyrightText: 2019 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "base/debuginfo.hpp"
#include "base/scriptframe.hpp"
#include "base/value.hpp"
#include "config/configcompiler.hpp"
#include "config/expression.hpp"
#include "icinga/apply-utility.hpp"
#include <utility>
#include <vector>

using namespace icinga;

/* Icinga DSL function which sets this.zone of a config object originated from an apply rule
 * to either the zone of the config object the apply rule matched or the zone of the apply rule.
 */
static const Value l_MakeCommonZoneDsl = ([]() -> Value {
	const char *dsl = R"EOF(

function(parent_object_zone, apply_rule_zone) {
	var child_zone = get_object(Zone, apply_rule_zone)

	var common_zone = if (child_zone && child_zone.global) {
		parent_object_zone
	} else {
		apply_rule_zone
	}

	if (common_zone != "") {
		this.zone = common_zone
	}
}

)EOF";

	ScriptFrame frame (false);
	auto expr (ConfigCompiler::CompileText("<anonymous>", dsl));

	return std::move(expr->Evaluate(frame).GetValue());
})();

/**
 * Create a DSL expression which sets this.zone of a config object originated from an apply rule
 *
 * @param parent_object_zone The zone of the config object the apply rule matched
 * @param apply_rule_zone The zone of the apply rule
 *
 * @return The newly created DSL expression or nullptr if nothing to do
 */
std::unique_ptr<Expression> ApplyUtility::MakeCommonZone(String parent_object_zone, String apply_rule_zone, const DebugInfo& debug_info)
{
	if (parent_object_zone == apply_rule_zone) {
		if (parent_object_zone.IsEmpty()) {
			return nullptr;
		}

		/* this.zone = parent_object_zone */
		return std::unique_ptr<Expression>(new SetExpression(MakeIndexer(ScopeThis, "zone"), OpSetLiteral, MakeLiteral(std::move(parent_object_zone)), debug_info));
	}

	std::vector<std::unique_ptr<Expression>> args;
	args.reserve(3);
	args.emplace_back(new GetScopeExpression(ScopeThis));
	args.emplace_back(MakeLiteral(std::move(parent_object_zone)));
	args.emplace_back(MakeLiteral(std::move(apply_rule_zone)));

	/* l_MakeCommonZoneDsl.call(this, parent_object_zone, apply_rule_zone) */
	return std::unique_ptr<Expression>(new FunctionCallExpression(
		std::unique_ptr<Expression>(new IndexerExpression(MakeLiteral(l_MakeCommonZoneDsl), MakeLiteral("call"))),
		std::move(args),
		debug_info
	));
}
