/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/notification.hpp"
#include "icinga/service.hpp"
#include "config/configitembuilder.hpp"
#include "config/applyrule.hpp"
#include "base/initialize.hpp"
#include "base/dynamictype.hpp"
#include "base/logger.hpp"
#include "base/context.hpp"
#include "base/workqueue.hpp"
#include "base/exception.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

INITIALIZE_ONCE(&Notification::RegisterApplyRuleHandler);

void Notification::RegisterApplyRuleHandler(void)
{
	std::vector<String> targets;
	targets.push_back("Host");
	targets.push_back("Service");
	ApplyRule::RegisterType("Notification", targets);
}

void Notification::EvaluateApplyRuleInstance(const Checkable::Ptr& checkable, const String& name, ScriptFrame& frame, const ApplyRule& rule)
{
	DebugInfo di = rule.GetDebugInfo();

	Log(LogDebug, "Notification")
	    << "Applying notification '" << name << "' to object '" << checkable->GetName() << "' for rule " << di;

	ConfigItemBuilder::Ptr builder = new ConfigItemBuilder(di);
	builder->SetType("Notification");
	builder->SetName(name);
	builder->SetScope(frame.Locals);

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	builder->AddExpression(new SetExpression(MakeIndexer(ScopeCurrent, "host_name"), OpSetLiteral, MakeLiteral(host->GetName()), di));

	if (service)
		builder->AddExpression(new SetExpression(MakeIndexer(ScopeCurrent, "service_name"), OpSetLiteral, MakeLiteral(service->GetShortName()), di));

	String zone = checkable->GetZone();

	if (!zone.IsEmpty())
		builder->AddExpression(new SetExpression(MakeIndexer(ScopeCurrent, "zone"), OpSetLiteral, MakeLiteral(zone), di));

	builder->AddExpression(new OwnedExpression(rule.GetExpression()));

	ConfigItem::Ptr notificationItem = builder->Compile();
	notificationItem->Commit();
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

	ScriptFrame frame;
	if (rule.GetScope())
		rule.GetScope()->CopyTo(frame.Locals);
	frame.Locals->Set("host", host);
	if (service)
		frame.Locals->Set("service", service);

	if (!rule.EvaluateFilter(frame))
		return false;

	Value vinstances;

	if (rule.GetFTerm()) {
		vinstances = rule.GetFTerm()->Evaluate(frame);
	} else {
		Array::Ptr instances = new Array();
		instances->Add("");
		vinstances = instances;
	}

	if (vinstances.IsObjectType<Array>()) {
		if (!rule.GetFVVar().IsEmpty())
			BOOST_THROW_EXCEPTION(ScriptError("Array iterator requires value to be an array.", di));

		Array::Ptr arr = vinstances;

		ObjectLock olock(arr);
		BOOST_FOREACH(const String& instance, arr) {
			String name = rule.GetName();

			if (!rule.GetFKVar().IsEmpty()) {
				frame.Locals->Set(rule.GetFKVar(), instance);
				name += instance;
			}

			EvaluateApplyRuleInstance(checkable, name, frame, rule);
		}
	} else if (vinstances.IsObjectType<Dictionary>()) {
		if (rule.GetFVVar().IsEmpty())
			BOOST_THROW_EXCEPTION(ScriptError("Dictionary iterator requires value to be a dictionary.", di));
	
		Dictionary::Ptr dict = vinstances;

		ObjectLock olock(dict);
		BOOST_FOREACH(const Dictionary::Pair& kv, dict) {
			frame.Locals->Set(rule.GetFKVar(), kv.first);
			frame.Locals->Set(rule.GetFVVar(), kv.second);

			EvaluateApplyRuleInstance(checkable, rule.GetName() + kv.first, frame, rule);
		}
	}

	return true;
}

void Notification::EvaluateApplyRules(const Host::Ptr& host)
{
	CONTEXT("Evaluating 'apply' rules for host '" + host->GetName() + "'");

	BOOST_FOREACH(ApplyRule& rule, ApplyRule::GetRules("Notification"))
	{
		if (rule.GetTargetType() != "Host")
			continue;

		if (EvaluateApplyRule(host, rule))
			rule.AddMatch();
	}
}

void Notification::EvaluateApplyRules(const Service::Ptr& service)
{
	CONTEXT("Evaluating 'apply' rules for service '" + service->GetName() + "'");

	BOOST_FOREACH(ApplyRule& rule, ApplyRule::GetRules("Notification")) {
		if (rule.GetTargetType() != "Service")
			continue;

		if (EvaluateApplyRule(service, rule))
			rule.AddMatch();
	}
}
