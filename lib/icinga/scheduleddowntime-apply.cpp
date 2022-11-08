/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/scheduleddowntime.hpp"
#include "icinga/service.hpp"
#include "config/configitembuilder.hpp"
#include "config/applyrule.hpp"
#include "base/initialize.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/context.hpp"
#include "base/exception.hpp"

using namespace icinga;

INITIALIZE_ONCE([]() {
	ApplyRule::RegisterType("ScheduledDowntime", { "Host", "Service" });
});

bool ScheduledDowntime::EvaluateApplyRuleInstance(const Checkable::Ptr& checkable, const String& name, ScriptFrame& frame, const ApplyRule& rule, bool skipFilter)
{
	if (!skipFilter && !rule.EvaluateFilter(frame))
		return false;

	DebugInfo di = rule.GetDebugInfo();

#ifdef _DEBUG
	Log(LogDebug, "ScheduledDowntime")
		<< "Applying scheduled downtime '" << rule.GetName() << "' to object '" << checkable->GetName() << "' for rule " << di;
#endif /* _DEBUG */

	ConfigItemBuilder builder{di};
	builder.SetType(ScheduledDowntime::TypeInstance);
	builder.SetName(name);
	builder.SetScope(frame.Locals->ShallowClone());
	builder.SetIgnoreOnError(rule.GetIgnoreOnError());

	builder.AddExpression(new ImportDefaultTemplatesExpression());

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	builder.AddExpression(new SetExpression(MakeIndexer(ScopeThis, "host_name"), OpSetLiteral, MakeLiteral(host->GetName()), di));

	if (service)
		builder.AddExpression(new SetExpression(MakeIndexer(ScopeThis, "service_name"), OpSetLiteral, MakeLiteral(service->GetShortName()), di));

	String zone = checkable->GetZoneName();

	if (!zone.IsEmpty())
		builder.AddExpression(new SetExpression(MakeIndexer(ScopeThis, "zone"), OpSetLiteral, MakeLiteral(zone), di));

	builder.AddExpression(new SetExpression(MakeIndexer(ScopeThis, "package"), OpSetLiteral, MakeLiteral(rule.GetPackage()), di));

	builder.AddExpression(new OwnedExpression(rule.GetExpression()));

	ConfigItem::Ptr downtimeItem = builder.Compile();
	downtimeItem->Register();

	return true;
}

bool ScheduledDowntime::EvaluateApplyRule(const Checkable::Ptr& checkable, const ApplyRule& rule, bool skipFilter)
{
	DebugInfo di = rule.GetDebugInfo();

	std::ostringstream msgbuf;
	msgbuf << "Evaluating 'apply' rule (" << di << ")";
	CONTEXT(msgbuf.str());

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	ScriptFrame frame(true);
	if (rule.GetScope())
		rule.GetScope()->CopyTo(frame.Locals);
	frame.Locals->Set("host", host);
	if (service)
		frame.Locals->Set("service", service);

	bool match = false;

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

				frame.Locals->Set(rule.GetFKVar(), instance);
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
				frame.Locals->Set(rule.GetFKVar(), kv.first);
				frame.Locals->Set(rule.GetFVVar(), kv.second);

				if (EvaluateApplyRuleInstance(checkable, rule.GetName() + kv.first, frame, rule, skipFilter))
					match = true;
			}
		}
	} else if (EvaluateApplyRuleInstance(checkable, rule.GetName(), frame, rule, skipFilter)) {
		match = true;
	}

	return match;
}

void ScheduledDowntime::EvaluateApplyRules(const Host::Ptr& host)
{
	CONTEXT("Evaluating 'apply' rules for host '" + host->GetName() + "'");

	for (auto& rule : ApplyRule::GetRules(ScheduledDowntime::TypeInstance, Host::TypeInstance)) {
		if (EvaluateApplyRule(host, *rule))
			rule->AddMatch();
	}

	for (auto& rule : ApplyRule::GetTargetedHostRules(ScheduledDowntime::TypeInstance, host->GetName())) {
		if (EvaluateApplyRule(host, *rule, true))
			rule->AddMatch();
	}
}

void ScheduledDowntime::EvaluateApplyRules(const Service::Ptr& service)
{
	CONTEXT("Evaluating 'apply' rules for service '" + service->GetName() + "'");

	for (auto& rule : ApplyRule::GetRules(ScheduledDowntime::TypeInstance, Service::TypeInstance)) {
		if (EvaluateApplyRule(service, *rule))
			rule->AddMatch();
	}

	for (auto& rule : ApplyRule::GetTargetedServiceRules(ScheduledDowntime::TypeInstance, service->GetHost()->GetName(), service->GetShortName())) {
		if (EvaluateApplyRule(service, *rule, true))
			rule->AddMatch();
	}
}
