/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/notification.hpp"
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
	ApplyRule::RegisterType("Notification", { "Host", "Service" });
});

bool Notification::EvaluateApplyRuleInstance(const Checkable::Ptr& checkable, const String& name, ScriptFrame& frame, const ApplyRule& rule)
{
	if (!rule.EvaluateFilter(frame))
		return false;

	DebugInfo di = rule.GetDebugInfo();

#ifdef _DEBUG
	Log(LogDebug, "Notification")
		<< "Applying notification '" << name << "' to object '" << checkable->GetName() << "' for rule " << di;
#endif /* _DEBUG */

	ConfigItemBuilder builder{di};
	builder.SetType(Notification::TypeInstance);
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

	ConfigItem::Ptr notificationItem = builder.Compile();
	notificationItem->Register();

	return true;
}

bool Notification::EvaluateApplyRule(const Checkable::Ptr& checkable, const ApplyRule& rule)
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

	Value vinstances;

	if (rule.GetFTerm()) {
		try {
			vinstances = rule.GetFTerm()->Evaluate(frame);
		} catch (const std::exception&) {
			/* Silently ignore errors here and assume there are no instances. */
			return false;
		}
	} else {
		vinstances = new Array({ "" });
	}

	bool match = false;

	if (vinstances.IsObjectType<Array>()) {
		if (!rule.GetFVVar().IsEmpty())
			BOOST_THROW_EXCEPTION(ScriptError("Dictionary iterator requires value to be a dictionary.", di));

		Array::Ptr arr = vinstances;

		ObjectLock olock(arr);
		for (const Value& instance : arr) {
			String name = rule.GetName();

			if (!rule.GetFKVar().IsEmpty()) {
				frame.Locals->Set(rule.GetFKVar(), instance);
				name += instance;
			}

			if (EvaluateApplyRuleInstance(checkable, name, frame, rule))
				match = true;
		}
	} else if (vinstances.IsObjectType<Dictionary>()) {
		if (rule.GetFVVar().IsEmpty())
			BOOST_THROW_EXCEPTION(ScriptError("Array iterator requires value to be an array.", di));

		Dictionary::Ptr dict = vinstances;

		for (const String& key : dict->GetKeys()) {
			frame.Locals->Set(rule.GetFKVar(), key);
			frame.Locals->Set(rule.GetFVVar(), dict->Get(key));

			if (EvaluateApplyRuleInstance(checkable, rule.GetName() + key, frame, rule))
				match = true;
		}
	}

	return match;
}

void Notification::EvaluateApplyRules(const Host::Ptr& host)
{
	CONTEXT("Evaluating 'apply' rules for host '" + host->GetName() + "'");

	for (auto& rule : ApplyRule::GetRules(Notification::TypeInstance, Host::TypeInstance))
	{
		if (EvaluateApplyRule(host, rule))
			rule.AddMatch();
	}
}

void Notification::EvaluateApplyRules(const Service::Ptr& service)
{
	CONTEXT("Evaluating 'apply' rules for service '" + service->GetName() + "'");

	for (auto& rule : ApplyRule::GetRules(Notification::TypeInstance, Service::TypeInstance)) {
		if (EvaluateApplyRule(service, rule))
			rule.AddMatch();
	}
}
