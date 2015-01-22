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
#include "config/configcompilercontext.hpp"
#include "base/initialize.hpp"
#include "base/dynamictype.hpp"
#include "base/logger.hpp"
#include "base/context.hpp"
#include "base/workqueue.hpp"
#include "base/configerror.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

INITIALIZE_ONCE(&Service::RegisterApplyRuleHandler);

void Service::RegisterApplyRuleHandler(void)
{
	std::vector<String> targets;
	targets.push_back("Host");
	ApplyRule::RegisterType("Service", targets, &Service::EvaluateApplyRules);
}

void Service::EvaluateApplyRuleOneInstance(const Host::Ptr& host, const String& name, const Dictionary::Ptr& locals, const ApplyRule& rule)
{
	DebugInfo di = rule.GetDebugInfo();

	Log(LogDebug, "Service")
	    << "Applying service '" << name << "' to host '" << host->GetName() << "' for rule " << di;

	ConfigItemBuilder::Ptr builder = new ConfigItemBuilder(di);
	builder->SetType("Service");
	builder->SetName(name);
	builder->SetScope(locals);

	builder->AddExpression(new SetExpression(MakeIndexer("host_name"), OpSetLiteral, MakeLiteral(host->GetName()), di));

	builder->AddExpression(new SetExpression(MakeIndexer("name"), OpSetLiteral, MakeLiteral(name), di));

	String zone = host->GetZone();

	if (!zone.IsEmpty())
		builder->AddExpression(new SetExpression(MakeIndexer("zone"), OpSetLiteral, MakeLiteral(zone), di));

	builder->AddExpression(new OwnedExpression(rule.GetExpression()));

	ConfigItem::Ptr serviceItem = builder->Compile();
	DynamicObject::Ptr dobj = serviceItem->Commit();
	dobj->OnConfigLoaded();
}

bool Service::EvaluateApplyRuleOne(const Host::Ptr& host, const ApplyRule& rule)
{
	DebugInfo di = rule.GetDebugInfo();

	std::ostringstream msgbuf;
	msgbuf << "Evaluating 'apply' rule (" << di << ")";
	CONTEXT(msgbuf.str());

	Dictionary::Ptr locals = new Dictionary();
	locals->Set("__parent", rule.GetScope());
	locals->Set("host", host);

	if (!rule.EvaluateFilter(locals))
		return false;

	Value vinstances;

	if (rule.GetFTerm()) {
		vinstances = rule.GetFTerm()->Evaluate(locals);
	} else {
		Array::Ptr instances = new Array();
		instances->Add("");
		vinstances = instances;
	}

	if (vinstances.IsObjectType<Array>()) {
		if (!rule.GetFVVar().IsEmpty())
			BOOST_THROW_EXCEPTION(ConfigError("Array iterator requires value to be an array.") << errinfo_debuginfo(di));

		Array::Ptr arr = vinstances;

		ObjectLock olock(arr);
		BOOST_FOREACH(const String& instance, arr) {
			String name = rule.GetName();

			if (!rule.GetFKVar().IsEmpty()) {
				locals->Set(rule.GetFKVar(), instance);
				name += instance;
			}

			EvaluateApplyRuleOneInstance(host, name, locals, rule);
		}
	} else if (vinstances.IsObjectType<Dictionary>()) {
		if (rule.GetFVVar().IsEmpty())
			BOOST_THROW_EXCEPTION(ConfigError("Dictionary iterator requires value to be a dictionary.") << errinfo_debuginfo(di));
	
		Dictionary::Ptr dict = vinstances;

		ObjectLock olock(dict);
		BOOST_FOREACH(const Dictionary::Pair& kv, dict) {
			locals->Set(rule.GetFKVar(), kv.first);
			locals->Set(rule.GetFVVar(), kv.second);

			EvaluateApplyRuleOneInstance(host, rule.GetName() + kv.first, locals, rule);
		}
	}

	return true;
}

void Service::EvaluateApplyRule(const ApplyRule& rule)
{
	int apply_count = 0;

	BOOST_FOREACH(const Host::Ptr& host, DynamicType::GetObjectsByType<Host>()) {
		CONTEXT("Evaluating 'apply' rules for host '" + host->GetName() + "'");

		try {
			if (EvaluateApplyRuleOne(host, rule))
				apply_count++;
		} catch (const ConfigError& ex) {
			const DebugInfo *di = boost::get_error_info<errinfo_debuginfo>(ex);
			ConfigCompilerContext::GetInstance()->AddMessage(true, ex.what(), di ? *di : DebugInfo());
		}
	}

	if (apply_count == 0)
		Log(LogWarning, "Service")
		    << "Apply rule '" << rule.GetName() << "' for host does not match anywhere!";
}

void Service::EvaluateApplyRules(const std::vector<ApplyRule>& rules)
{
	ParallelWorkQueue upq;

	BOOST_FOREACH(const ApplyRule& rule, rules) {
		upq.Enqueue(boost::bind(&Service::EvaluateApplyRule, boost::cref(rule)));
	}

	upq.Join();
}
