/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

bool Service::EvaluateApplyRuleOne(const Host::Ptr& host, const ApplyRule& rule)
{
	DebugInfo di = rule.GetDebugInfo();

	std::ostringstream msgbuf;
	msgbuf << "Evaluating 'apply' rule (" << di << ")";
	CONTEXT(msgbuf.str());

	Dictionary::Ptr locals = make_shared<Dictionary>();
	locals->Set("host", host);

	if (!rule.EvaluateFilter(locals))
		return false;

	Log(LogDebug, "Service")
	    << "Applying service '" << rule.GetName() << "' to host '" << host->GetName() << "' for rule " << di;

	ConfigItemBuilder::Ptr builder = make_shared<ConfigItemBuilder>(di);
	builder->SetType("Service");
	builder->SetName(rule.GetName());
	builder->SetScope(rule.GetScope());

	builder->AddExpression(make_shared<Expression>(&Expression::OpSet,
	    make_shared<Expression>(&Expression::OpLiteral, "host_name", di), 
	    make_shared<Expression>(&Expression::OpLiteral, host->GetName(), di),
	    di));

	builder->AddExpression(make_shared<Expression>(&Expression::OpSet,
	    make_shared<Expression>(&Expression::OpLiteral, "name", di),
	    make_shared<Expression>(&Expression::OpLiteral, rule.GetName(), di),
	    di));

	String zone = host->GetZone();

	if (!zone.IsEmpty()) {
		builder->AddExpression(make_shared<Expression>(&Expression::OpSet,
		    make_shared<Expression>(&Expression::OpLiteral, "zone", di),
		    make_shared<Expression>(&Expression::OpLiteral, zone, di),
		    di));
	}

	builder->AddExpression(rule.GetExpression());

	ConfigItem::Ptr serviceItem = builder->Compile();
	serviceItem->Register();
	DynamicObject::Ptr dobj = serviceItem->Commit();
	dobj->OnConfigLoaded();

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
