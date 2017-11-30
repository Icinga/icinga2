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

bool Dependency::EvaluateApplyRuleInstance(const Checkable::Ptr& checkable, const String& name, ScriptFrame& frame, const ApplyRule& rule)
{
	if (!rule.EvaluateFilter(frame))
		return false;

	DebugInfo di = rule.GetDebugInfo();

#ifdef _DEBUG
	Log(LogDebug, "Dependency")
		<< "Applying dependency '" << name << "' to object '" << checkable->GetName() << "' for rule " << di;
#endif /* _DEBUG */

	ConfigItemBuilder::Ptr builder = new ConfigItemBuilder(di);
	builder->SetType(Dependency::TypeInstance);
	builder->SetName(name);
	builder->SetScope(frame.Locals->ShallowClone());
	builder->SetIgnoreOnError(rule.GetIgnoreOnError());

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	builder->AddExpression(new SetExpression(MakeIndexer(ScopeThis, "parent_host_name"), OpSetLiteral, MakeLiteral(host->GetName()), di));
	builder->AddExpression(new SetExpression(MakeIndexer(ScopeThis, "child_host_name"), OpSetLiteral, MakeLiteral(host->GetName()), di));

	if (service)
		builder->AddExpression(new SetExpression(MakeIndexer(ScopeThis, "child_service_name"), OpSetLiteral, MakeLiteral(service->GetShortName()), di));

	String zone = checkable->GetZoneName();

	if (!zone.IsEmpty())
		builder->AddExpression(new SetExpression(MakeIndexer(ScopeThis, "zone"), OpSetLiteral, MakeLiteral(zone), di));

	builder->AddExpression(new SetExpression(MakeIndexer(ScopeThis, "package"), OpSetLiteral, MakeLiteral(rule.GetPackage()), di));

	builder->AddExpression(new ImportDefaultTemplatesExpression());

	builder->AddExpression(new OwnedExpression(rule.GetExpression()));

	ConfigItem::Ptr dependencyItem = builder->Compile();
	dependencyItem->Register();

	return true;
}

bool Dependency::EvaluateApplyRule(const Checkable::Ptr& checkable, const ApplyRule& rule)
{
	DebugInfo di = rule.GetDebugInfo();

	std::ostringstream msgbuf;
	msgbuf << "Evaluating 'apply' rule (" << di << ")";
	CONTEXT(msgbuf.str());

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	ScriptFrame frame;
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
		Array::Ptr instances = new Array();
		instances->Add("");
		vinstances = instances;
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

void Dependency::EvaluateApplyRules(const Host::Ptr& host)
{
	CONTEXT("Evaluating 'apply' rules for host '" + host->GetName() + "'");

	for (ApplyRule& rule : ApplyRule::GetRules("Dependency")) {
		if (rule.GetTargetType() != "Host")
			continue;

		if (EvaluateApplyRule(host, rule))
			rule.AddMatch();
	}
}

void Dependency::EvaluateApplyRules(const Service::Ptr& service)
{
	CONTEXT("Evaluating 'apply' rules for service '" + service->GetName() + "'");

	for (ApplyRule& rule : ApplyRule::GetRules("Dependency")) {
		if (rule.GetTargetType() != "Service")
			continue;

		if (EvaluateApplyRule(service, rule))
			rule.AddMatch();
	}
}
