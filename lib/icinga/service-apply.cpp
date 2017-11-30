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
	ApplyRule::RegisterType("Service", { "Host" });
});

bool Service::EvaluateApplyRuleInstance(const Host::Ptr& host, const String& name, ScriptFrame& frame, const ApplyRule& rule)
{
	if (!rule.EvaluateFilter(frame))
		return false;

	DebugInfo di = rule.GetDebugInfo();

#ifdef _DEBUG
	Log(LogDebug, "Service")
	    << "Applying service '" << name << "' to host '" << host->GetName() << "' for rule " << di;
#endif /* _DEBUG */

	ConfigItemBuilder::Ptr builder = new ConfigItemBuilder(di);
	builder->SetType(Service::TypeInstance);
	builder->SetName(name);
	builder->SetScope(frame.Locals->ShallowClone());
	builder->SetIgnoreOnError(rule.GetIgnoreOnError());

	builder->AddExpression(new SetExpression(MakeIndexer(ScopeThis, "host_name"), OpSetLiteral, MakeLiteral(host->GetName()), di));

	builder->AddExpression(new SetExpression(MakeIndexer(ScopeThis, "name"), OpSetLiteral, MakeLiteral(name), di));

	String zone = host->GetZoneName();

	if (!zone.IsEmpty())
		builder->AddExpression(new SetExpression(MakeIndexer(ScopeThis, "zone"), OpSetLiteral, MakeLiteral(zone), di));

	builder->AddExpression(new SetExpression(MakeIndexer(ScopeThis, "package"), OpSetLiteral, MakeLiteral(rule.GetPackage()), di));

	builder->AddExpression(new OwnedExpression(rule.GetExpression()));

	builder->AddExpression(new ImportDefaultTemplatesExpression());

	ConfigItem::Ptr serviceItem = builder->Compile();
	serviceItem->Register();

	return true;
}

bool Service::EvaluateApplyRule(const Host::Ptr& host, const ApplyRule& rule)
{
	DebugInfo di = rule.GetDebugInfo();

	std::ostringstream msgbuf;
	msgbuf << "Evaluating 'apply' rule (" << di << ")";
	CONTEXT(msgbuf.str());

	ScriptFrame frame;
	if (rule.GetScope())
		rule.GetScope()->CopyTo(frame.Locals);
	frame.Locals->Set("host", host);

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

			if (EvaluateApplyRuleInstance(host, name, frame, rule))
				match = true;
		}
	} else if (vinstances.IsObjectType<Dictionary>()) {
		if (rule.GetFVVar().IsEmpty())
			BOOST_THROW_EXCEPTION(ScriptError("Array iterator requires value to be an array.", di));
	
		Dictionary::Ptr dict = vinstances;

		for (const String& key : dict->GetKeys()) {
			frame.Locals->Set(rule.GetFKVar(), key);
			frame.Locals->Set(rule.GetFVVar(), dict->Get(key));

			if (EvaluateApplyRuleInstance(host, rule.GetName() + key, frame, rule))
				match = true;
		}
	}

	return match;
}

void Service::EvaluateApplyRules(const Host::Ptr& host)
{
	for (ApplyRule& rule : ApplyRule::GetRules("Service")) {
		CONTEXT("Evaluating 'apply' rules for host '" + host->GetName() + "'");

		if (EvaluateApplyRule(host, rule))
			rule.AddMatch();
	}
}
