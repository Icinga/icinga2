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

#include "icinga/host.h"
#include "config/configitembuilder.h"
#include "base/initialize.h"
#include "base/dynamictype.h"
#include "base/convert.h"
#include "base/logger_fwd.h"
#include "base/context.h"
#include <boost/foreach.hpp>

using namespace icinga;

INITIALIZE_ONCE(&Host::RegisterApplyRuleHandler);

void Host::RegisterApplyRuleHandler(void)
{
	ApplyRule::RegisterCombination("Service", "Host", &Host::EvaluateApplyRules);
}

void Host::EvaluateApplyRules(const std::vector<ApplyRule>& rules)
{
	BOOST_FOREACH(const Host::Ptr& host, DynamicType::GetObjects<Host>()) {
		CONTEXT("Evaluating 'apply' rules for Host '" + host->GetName() + "'");

		Dictionary::Ptr locals = make_shared<Dictionary>();
		locals->Set("host", host->GetName());

		Array::Ptr groups = host->GetGroups();
		if (!groups)
			groups = make_shared<Array>();
		locals->Set("hostgroups", groups);

		BOOST_FOREACH(const ApplyRule& rule, rules) {
			std::ostringstream msgbuf;
			msgbuf << "Evaluating 'apply' rule (" << rule.GetDebugInfo() << ")";
			CONTEXT(msgbuf.str());

			Value result = rule.GetExpression()->Evaluate(locals);

			try {
				if (!static_cast<bool>(result))
					continue;
			} catch (...) {
				std::ostringstream msgbuf;
				msgbuf << "Apply rule (" << rule.GetDebugInfo() << ") returned invalid data type, expected bool: " + JsonSerialize(result);
				Log(LogCritical, "icinga", msgbuf.str());

				continue;
			}

			if (result.IsEmpty())
				continue;

			std::ostringstream namebuf;
			namebuf << host->GetName() << "!apply!" << rule.GetTemplate();
			String name = namebuf.str();

			ConfigItemBuilder::Ptr builder = make_shared<ConfigItemBuilder>(rule.GetDebugInfo());
			builder->SetType("Service");
			builder->SetName(name);
			builder->AddExpression("host", OperatorSet, host->GetName());

			builder->AddParent(rule.GetTemplate());

			ConfigItem::Ptr serviceItem = builder->Compile();
			serviceItem->Register();
			DynamicObject::Ptr dobj = serviceItem->Commit();
			dobj->OnConfigLoaded();
		}
	}
}
