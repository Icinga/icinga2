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

#include "icinga/dependency.hpp"
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

INITIALIZE_ONCE(&Dependency::RegisterApplyRuleHandler);

void Dependency::RegisterApplyRuleHandler(void)
{
	std::vector<String> targets;
	targets.push_back("Host");
	targets.push_back("Service");
	ApplyRule::RegisterType("Dependency", targets, &Dependency::EvaluateApplyRules);
}

void Dependency::EvaluateApplyRuleOneInstance(const Checkable::Ptr& checkable, const String& name, const Dictionary::Ptr& locals, const ApplyRule& rule)
{
	DebugInfo di = rule.GetDebugInfo();

	Log(LogDebug, "Dependency")
		<< "Applying dependency '" << name << "' to object '" << checkable->GetName() << "' for rule " << di;

	ConfigItemBuilder::Ptr builder = new ConfigItemBuilder(di);
	builder->SetType("Dependency");
	builder->SetName(name);
	builder->SetScope(locals);

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	builder->AddExpression(new SetExpression(MakeIndexer("parent_host_name"), OpSetLiteral, MakeLiteral(host->GetName()), di));
	builder->AddExpression(new SetExpression(MakeIndexer("child_host_name"), OpSetLiteral, MakeLiteral(host->GetName()), di));

	if (service)
		builder->AddExpression(new SetExpression(MakeIndexer("child_service_name"), OpSetLiteral, MakeLiteral(service->GetShortName()), di));

	String zone = checkable->GetZone();

	if (!zone.IsEmpty())
		builder->AddExpression(new SetExpression(MakeIndexer("zone"), OpSetLiteral, MakeLiteral(zone), di));

	builder->AddExpression(new OwnedExpression(rule.GetExpression()));

	ConfigItem::Ptr dependencyItem = builder->Compile();
	DynamicObject::Ptr dobj = dependencyItem->Commit();
	dobj->OnConfigLoaded();

}

bool Dependency::EvaluateApplyRuleOne(const Checkable::Ptr& checkable, const ApplyRule& rule)
{
	DebugInfo di = rule.GetDebugInfo();

	std::ostringstream msgbuf;
	msgbuf << "Evaluating 'apply' rule (" << di << ")";
	CONTEXT(msgbuf.str());

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr locals = new Dictionary();
	locals->Set("__parent", rule.GetScope());
	locals->Set("host", host);
	if (service)
		locals->Set("service", service);

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

			EvaluateApplyRuleOneInstance(checkable, name, locals, rule);
		}
	} else if (vinstances.IsObjectType<Dictionary>()) {
		if (rule.GetFVVar().IsEmpty())
			BOOST_THROW_EXCEPTION(ConfigError("Dictionary iterator requires value to be a dictionary.") << errinfo_debuginfo(di));
	
		Dictionary::Ptr dict = vinstances;

		ObjectLock olock(dict);
		BOOST_FOREACH(const Dictionary::Pair& kv, dict) {
			locals->Set(rule.GetFKVar(), kv.first);
			locals->Set(rule.GetFVVar(), kv.second);

			EvaluateApplyRuleOneInstance(checkable, rule.GetName() + kv.first, locals, rule);
		}
	}

	return true;
}

void Dependency::EvaluateApplyRule(const ApplyRule& rule)
{
	int apply_count = 0;

	if (rule.GetTargetType() == "Host") {
		apply_count = 0;

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
			Log(LogWarning, "Dependency")
			    << "Apply rule '" << rule.GetName() << "' for host does not match anywhere!";

	} else if (rule.GetTargetType() == "Service") {
		apply_count = 0;

		BOOST_FOREACH(const Service::Ptr& service, DynamicType::GetObjectsByType<Service>()) {
			CONTEXT("Evaluating 'apply' rules for Service '" + service->GetName() + "'");

			try {
				if (EvaluateApplyRuleOne(service, rule))
					apply_count++;
			} catch (const ConfigError& ex) {
				const DebugInfo *di = boost::get_error_info<errinfo_debuginfo>(ex);
				ConfigCompilerContext::GetInstance()->AddMessage(true, ex.what(), di ? *di : DebugInfo());
			}
		}

		if (apply_count == 0)
			Log(LogWarning, "Dependency")
			    << "Apply rule '" << rule.GetName() << "' for service does not match anywhere!";

	} else {
		Log(LogWarning, "Dependency")
		    << "Wrong target type for apply rule '" << rule.GetName() << "'!";
	}
}

void Dependency::EvaluateApplyRules(const std::vector<ApplyRule>& rules)
{
	ParallelWorkQueue upq;

	BOOST_FOREACH(const ApplyRule& rule, rules) {
		upq.Enqueue(boost::bind(&Dependency::EvaluateApplyRule, boost::cref(rule)));
	}

	upq.Join();
}
