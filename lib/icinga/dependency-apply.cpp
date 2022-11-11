/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/dependency.hpp"
#include "icinga/service.hpp"
#include "config/configitembuilder.hpp"
#include "config/applyrule.hpp"
#include "base/initialize.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/context.hpp"
#include "base/workqueue.hpp"
#include "base/exception.hpp"

using namespace icinga;

INITIALIZE_ONCE([]() {
	ApplyRule::RegisterType("Dependency", { "Host", "Service" });
});

bool Dependency::EvaluateApplyRuleInstance(const Checkable::Ptr& checkable, const String& name, ScriptFrame& frame, const ApplyRule& rule, bool skipFilter)
{
	if (!skipFilter && !rule.EvaluateFilter(frame))
		return false;

	auto& di (rule.GetDebugInfo());

#ifdef _DEBUG
	Log(LogDebug, "Dependency")
		<< "Applying dependency '" << name << "' to object '" << checkable->GetName() << "' for rule " << di;
#endif /* _DEBUG */

	ConfigItemBuilder builder{di};
	builder.SetType(Dependency::TypeInstance);
	builder.SetName(name);
	builder.SetScope(frame.Locals->ShallowClone());
	builder.SetIgnoreOnError(rule.GetIgnoreOnError());

	builder.AddExpression(new ImportDefaultTemplatesExpression());

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	builder.AddExpression(new SetExpression(MakeIndexer(ScopeThis, "parent_host_name"), OpSetLiteral, MakeLiteral(host->GetName()), di));
	builder.AddExpression(new SetExpression(MakeIndexer(ScopeThis, "child_host_name"), OpSetLiteral, MakeLiteral(host->GetName()), di));

	if (service)
		builder.AddExpression(new SetExpression(MakeIndexer(ScopeThis, "child_service_name"), OpSetLiteral, MakeLiteral(service->GetShortName()), di));

	String zone = checkable->GetZoneName();

	if (!zone.IsEmpty())
		builder.AddExpression(new SetExpression(MakeIndexer(ScopeThis, "zone"), OpSetLiteral, MakeLiteral(zone), di));

	builder.AddExpression(new SetExpression(MakeIndexer(ScopeThis, "package"), OpSetLiteral, MakeLiteral(rule.GetPackage()), di));

	builder.AddExpression(new OwnedExpression(rule.GetExpression()));

	ConfigItem::Ptr dependencyItem = builder.Compile();
	dependencyItem->Register();

	return true;
}

bool Dependency::EvaluateApplyRule(const Checkable::Ptr& checkable, const ApplyRule& rule, TotalTimeSpentOnApplyMismatches& totalTimeSpentOnApplyMismatches, bool skipFilter)
{
	bool match = false;
	BenchmarkApplyRuleEvaluation bare (totalTimeSpentOnApplyMismatches, match);

	auto& di (rule.GetDebugInfo());

	std::ostringstream msgbuf;
	msgbuf << "Evaluating 'apply' rule (" << di << ")";
	CONTEXT(msgbuf.str());

	ScriptFrame frame (false);

	if (rule.GetScope() || rule.GetFTerm()) {
		frame.Locals = new Dictionary();

		if (rule.GetScope()) {
			rule.GetScope()->CopyTo(frame.Locals);
		}

		checkable->GetFrozenLocalsForApply()->CopyTo(frame.Locals);
		frame.Locals->Freeze();
	} else {
		frame.Locals = checkable->GetFrozenLocalsForApply();
	}

	if (rule.GetFTerm()) {
		Value vinstances;

		try {
			vinstances = rule.GetFTerm()->Evaluate(frame);
		} catch (const std::exception&) {
			/* Silently ignore errors here and assume there are no instances. */
			return false;
		}

		if (vinstances.IsObjectType<Array>()) {
			if (!rule.GetFVVar().IsEmpty())
				BOOST_THROW_EXCEPTION(ScriptError("Dictionary iterator requires value to be a dictionary.", di));

			Array::Ptr arr = vinstances;

			ObjectLock olock(arr);
			for (const Value& instance : arr) {
				String name = rule.GetName();

				frame.Locals->Set(rule.GetFKVar(), instance, true);
				name += instance;

				if (EvaluateApplyRuleInstance(checkable, name, frame, rule, skipFilter))
					match = true;
			}
		} else if (vinstances.IsObjectType<Dictionary>()) {
			if (rule.GetFVVar().IsEmpty())
				BOOST_THROW_EXCEPTION(ScriptError("Array iterator requires value to be an array.", di));

			Dictionary::Ptr dict = vinstances;
			ObjectLock olock (dict);

			for (auto& kv : dict) {
				frame.Locals->Set(rule.GetFKVar(), kv.first, true);
				frame.Locals->Set(rule.GetFVVar(), kv.second, true);

				if (EvaluateApplyRuleInstance(checkable, rule.GetName() + kv.first, frame, rule, skipFilter))
					match = true;
			}
		}
	} else if (EvaluateApplyRuleInstance(checkable, rule.GetName(), frame, rule, skipFilter)) {
		match = true;
	}

	return match;
}

void Dependency::EvaluateApplyRules(const Host::Ptr& host, TotalTimeSpentOnApplyMismatches& totalTimeSpentOnApplyMismatches)
{
	CONTEXT("Evaluating 'apply' rules for host '" + host->GetName() + "'");

	for (auto& rule : ApplyRule::GetRules(Dependency::TypeInstance, Host::TypeInstance)) {
		if (EvaluateApplyRule(host, *rule, totalTimeSpentOnApplyMismatches))
			rule->AddMatch();
	}

	for (auto& rule : ApplyRule::GetTargetedHostRules(Dependency::TypeInstance, host->GetName())) {
		if (EvaluateApplyRule(host, *rule, totalTimeSpentOnApplyMismatches, true))
			rule->AddMatch();
	}
}

void Dependency::EvaluateApplyRules(const Service::Ptr& service, TotalTimeSpentOnApplyMismatches& totalTimeSpentOnApplyMismatches)
{
	CONTEXT("Evaluating 'apply' rules for service '" + service->GetName() + "'");

	for (auto& rule : ApplyRule::GetRules(Dependency::TypeInstance, Service::TypeInstance)) {
		if (EvaluateApplyRule(service, *rule, totalTimeSpentOnApplyMismatches))
			rule->AddMatch();
	}

	for (auto& rule : ApplyRule::GetTargetedServiceRules(Dependency::TypeInstance, service->GetHost()->GetName(), service->GetShortName())) {
		if (EvaluateApplyRule(service, *rule, totalTimeSpentOnApplyMismatches, true))
			rule->AddMatch();
	}
}
