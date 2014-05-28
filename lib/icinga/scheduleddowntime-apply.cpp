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

#include "icinga/scheduleddowntime.hpp"
#include "icinga/service.hpp"
#include "config/configitembuilder.hpp"
#include "config/applyrule.hpp"
#include "base/initialize.hpp"
#include "base/dynamictype.hpp"
#include "base/logger_fwd.hpp"
#include "base/context.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

INITIALIZE_ONCE(&ScheduledDowntime::RegisterApplyRuleHandler);

void ScheduledDowntime::RegisterApplyRuleHandler(void)
{
	std::vector<String> targets;
	targets.push_back("Host");
	targets.push_back("Service");
	ApplyRule::RegisterType("ScheduledDowntime", targets, &ScheduledDowntime::EvaluateApplyRules);
}

bool ScheduledDowntime::EvaluateApplyRule(const Checkable::Ptr& checkable, const ApplyRule& rule)
{
	DebugInfo di = rule.GetDebugInfo();

	std::ostringstream msgbuf;
	msgbuf << "Evaluating 'apply' rule (" << di << ")";
	CONTEXT(msgbuf.str());

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr locals = make_shared<Dictionary>();
	locals->Set("host", host);
	if (service)
		locals->Set("service", service);

	if (!rule.EvaluateFilter(locals))
		return false;

	std::ostringstream msgbuf2;
	msgbuf2 << "Applying scheduled downtime '" << rule.GetName() << "' to object '" << checkable->GetName() << "' for rule " << di;
	Log(LogDebug, "ScheduledDowntime", msgbuf2.str());

	ConfigItemBuilder::Ptr builder = make_shared<ConfigItemBuilder>(di);
	builder->SetType("ScheduledDowntime");
	builder->SetName(rule.GetName());
	builder->SetScope(rule.GetScope());

	builder->AddExpression(make_shared<AExpression>(&AExpression::OpSet,
	    make_shared<AExpression>(&AExpression::OpLiteral, "host_name", di),
	    make_shared<AExpression>(&AExpression::OpLiteral, host->GetName(), di),
	    di));

	if (service) {
		builder->AddExpression(make_shared<AExpression>(&AExpression::OpSet,
		    make_shared<AExpression>(&AExpression::OpLiteral, "service_name", di),
		    make_shared<AExpression>(&AExpression::OpLiteral, service->GetShortName(), di),
		    di));
	}

	String zone = checkable->GetZone();

	if (!zone.IsEmpty()) {
		builder->AddExpression(make_shared<AExpression>(&AExpression::OpSet,
		    make_shared<AExpression>(&AExpression::OpLiteral, "zone", di),
		    make_shared<AExpression>(&AExpression::OpLiteral, zone, di),
		    di));
	}

	builder->AddExpression(rule.GetExpression());

	ConfigItem::Ptr downtimeItem = builder->Compile();
	downtimeItem->Register();
	DynamicObject::Ptr dobj = downtimeItem->Commit();
	dobj->OnConfigLoaded();

	return true;
}

void ScheduledDowntime::EvaluateApplyRules(const std::vector<ApplyRule>& rules)
{
	int apply_count = 0;

	BOOST_FOREACH(const ApplyRule& rule, rules) {
		if (rule.GetTargetType() == "Host") {
			apply_count = 0;

			BOOST_FOREACH(const Host::Ptr& host, DynamicType::GetObjects<Host>()) {
				CONTEXT("Evaluating 'apply' rules for host '" + host->GetName() + "'");

				if (EvaluateApplyRule(host, rule))
					apply_count++;
			}

			if (apply_count == 0)
				Log(LogWarning, "ScheduledDowntime", "Apply rule '" + rule.GetName() + "' for host does not match anywhere!");

		} else if (rule.GetTargetType() == "Service") {
			apply_count = 0;

			BOOST_FOREACH(const Service::Ptr& service, DynamicType::GetObjects<Service>()) {
				CONTEXT("Evaluating 'apply' rules for Service '" + service->GetName() + "'");

				if(EvaluateApplyRule(service, rule))
					apply_count++;
			}

			if (apply_count == 0)
				Log(LogWarning, "ScheduledDowntime", "Apply rule '" + rule.GetName() + "' for service does not match anywhere!");

		} else {
			Log(LogWarning, "ScheduledDowntime", "Wrong target type for apply rule '" + rule.GetName() + "'!");
		}
	}
}
