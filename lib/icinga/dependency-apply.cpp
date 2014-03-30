/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-present Icinga Development Team (http://www.icinga.org) *
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

#include "icinga/dependency.h"
#include "config/configitembuilder.h"
#include "base/initialize.h"
#include "base/dynamictype.h"
#include "base/convert.h"
#include "base/logger_fwd.h"
#include "base/context.h"
#include <boost/foreach.hpp>

using namespace icinga;

INITIALIZE_ONCE(&Dependency::RegisterApplyRuleHandler);

void Dependency::RegisterApplyRuleHandler(void)
{
	ApplyRule::RegisterType("Dependency", "Service", &Dependency::EvaluateApplyRules);
}

void Dependency::EvaluateApplyRules(const std::vector<ApplyRule>& rules)
{
	BOOST_FOREACH(const Service::Ptr& service, DynamicType::GetObjects<Service>()) {
		CONTEXT("Evaluating 'apply' rules for Service '" + service->GetName() + "'");

		Dictionary::Ptr locals = make_shared<Dictionary>();
		locals->Set("host", service->GetHost());
		locals->Set("service", service);

		BOOST_FOREACH(const ApplyRule& rule, rules) {
			DebugInfo di = rule.GetDebugInfo();

			std::ostringstream msgbuf;
			msgbuf << "Evaluating 'apply' rule (" << di << ")";
			CONTEXT(msgbuf.str());

			if (!rule.EvaluateFilter(locals))
				continue;

			std::ostringstream msgbuf2;
			msgbuf2 << "Applying dependency '" << rule.GetName() << "' to service '" << service->GetName() << "' for rule " << di;
			Log(LogDebug, "icinga", msgbuf2.str());

			std::ostringstream namebuf;
			namebuf << service->GetName() << "!" << rule.GetName();
			String name = namebuf.str();

			ConfigItemBuilder::Ptr builder = make_shared<ConfigItemBuilder>(di);
			builder->SetType("Dependency");
			builder->SetName(name);
			builder->SetScope(rule.GetScope());

			builder->AddExpression(make_shared<AExpression>(&AExpression::OpSet,
			    make_shared<AExpression>(&AExpression::OpLiteral, "child_host", di),
			    make_shared<AExpression>(&AExpression::OpLiteral, service->GetHost()->GetName(),
			    di), di));

			builder->AddExpression(make_shared<AExpression>(&AExpression::OpSet,
			    make_shared<AExpression>(&AExpression::OpLiteral, "child_service", di),
			    make_shared<AExpression>(&AExpression::OpLiteral, service->GetShortName(), di), di));

			builder->AddExpression(rule.GetExpression());

			ConfigItem::Ptr serviceItem = builder->Compile();
			serviceItem->Register();
			DynamicObject::Ptr dobj = serviceItem->Commit();
			dobj->OnConfigLoaded();
		}
	}
}
