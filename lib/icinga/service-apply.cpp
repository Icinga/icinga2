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

INITIALIZE_ONCE(&Service::RegisterApplyRuleHandler);

void Service::RegisterApplyRuleHandler(void)
{
	std::vector<String> targets;
	targets.push_back("Host");
	ApplyRule::RegisterType("Service", targets);
}

void Service::EvaluateApplyRuleInstance(const Host::Ptr& host, const String& name, ScriptFrame& frame, const ApplyRule& rule)
{
	DebugInfo di = rule.GetDebugInfo();

	Log(LogDebug, "Service")
	    << "Applying service '" << name << "' to host '" << host->GetName() << "' for rule " << di;

	ConfigItemBuilder::Ptr builder = new ConfigItemBuilder(di);
	builder->SetType("Service");
	builder->SetName(name);
	builder->SetScope(frame.Locals);

	builder->AddExpression(new SetExpression(MakeIndexer(ScopeCurrent, "host_name"), OpSetLiteral, MakeLiteral(host->GetName()), di));

	builder->AddExpression(new SetExpression(MakeIndexer(ScopeCurrent, "name"), OpSetLiteral, MakeLiteral(name), di));

	String zone = host->GetZone();

	if (!zone.IsEmpty())
		builder->AddExpression(new SetExpression(MakeIndexer(ScopeCurrent, "zone"), OpSetLiteral, MakeLiteral(zone), di));

	builder->AddExpression(new OwnedExpression(rule.GetExpression()));

	ConfigItem::Ptr serviceItem = builder->Compile();
	serviceItem->Commit();
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

			EvaluateApplyRuleInstance(host, name, frame, rule);
		}
	} else if (vinstances.IsObjectType<Dictionary>()) {
		if (rule.GetFVVar().IsEmpty())
			BOOST_THROW_EXCEPTION(ScriptError("Dictionary iterator requires value to be a dictionary.", di));
	
		Dictionary::Ptr dict = vinstances;

		ObjectLock olock(dict);
		BOOST_FOREACH(const Dictionary::Pair& kv, dict) {
			frame.Locals->Set(rule.GetFKVar(), kv.first);
			frame.Locals->Set(rule.GetFVVar(), kv.second);

			EvaluateApplyRuleInstance(host, rule.GetName() + kv.first, frame, rule);
		}
	}

	return true;
}

void Service::EvaluateApplyRules(const Host::Ptr& host)
{
	BOOST_FOREACH(ApplyRule& rule, ApplyRule::GetRules("Service")) {
		CONTEXT("Evaluating 'apply' rules for host '" + host->GetName() + "'");

		if (EvaluateApplyRule(host, rule))
			rule.AddMatch();
	}
}
