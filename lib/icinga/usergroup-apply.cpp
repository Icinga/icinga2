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

#include "icinga/hostgroup.h"
#include "icinga/service.h"
#include "config/configitembuilder.h"
#include "base/initialize.h"
#include "base/dynamictype.h"
#include "base/convert.h"
#include "base/logger_fwd.h"
#include "base/context.h"
#include <boost/foreach.hpp>

using namespace icinga;

INITIALIZE_ONCE(&UserGroup::RegisterApplyRuleHandler);

void UserGroup::RegisterApplyRuleHandler(void)
{
	std::vector<String> targets;
	targets.push_back("User");
	ApplyRule::RegisterType("UserGroup", targets, &UserGroup::EvaluateApplyRules);
}

bool UserGroup::EvaluateApplyRule(const User::Ptr& user, const ApplyRule& rule)
{
	DebugInfo di = rule.GetDebugInfo();

	std::ostringstream msgbuf;
	msgbuf << "Evaluating 'apply' rule (" << di << ")";
	CONTEXT(msgbuf.str());

	Dictionary::Ptr locals = make_shared<Dictionary>();
	locals->Set("user", user);

	if (!rule.EvaluateFilter(locals))
		return false;

	std::ostringstream msgbuf2;
	msgbuf2 << "Applying usergroup '" << rule.GetName() << "' to object '" << user->GetName() << "' for rule " << di;
	Log(LogDebug, "icinga", msgbuf2.str());


	String group_name = rule.GetName();

	ConfigItemBuilder::Ptr builder = make_shared<ConfigItemBuilder>(di);
	builder->SetType("UserGroup");
	builder->SetName(group_name);
	builder->SetScope(rule.GetScope());

	builder->AddExpression(rule.GetExpression());

	UserGroup::Ptr group = UserGroup::GetByName(group_name);

	/* if group does not exist, create it only once */
	if (!group) {
		ConfigItem::Ptr usergroupItem = builder->Compile();
		usergroupItem->Register();
		DynamicObject::Ptr dobj = usergroupItem->Commit();

		group = dynamic_pointer_cast<UserGroup>(dobj);

		if (!group) {
			Log(LogCritical, "icinga", "Unable to create UserGroup '" + group_name + "' for apply rule.");
			return false;
		}

		Log(LogDebug, "icinga", "UserGroup '" + group_name + "' created for apply rule.");
	} else
		Log(LogDebug, "icinga", "UserGroup '" + group_name + "' already exists. Skipping apply rule creation.");

	/* assign user group membership */
	group->AddMember(user);

	return true;
}

void UserGroup::EvaluateApplyRules(const std::vector<ApplyRule>& rules)
{
	int apply_count = 0;

	BOOST_FOREACH(const ApplyRule& rule, rules) {
		if (rule.GetTargetType() == "User") {
			apply_count = 0;

			BOOST_FOREACH(const User::Ptr& user, DynamicType::GetObjects<User>()) {
				CONTEXT("Evaluating 'apply' rules for User '" + user->GetName() + "'");

				if(EvaluateApplyRule(user, rule))
					apply_count++;
			}

			if (apply_count == 0)
				Log(LogWarning, "icinga", "Apply rule '" + rule.GetName() + "' for user does not match anywhere!");

		} else {
			Log(LogWarning, "icinga", "Wrong target type for apply rule '" + rule.GetName() + "'!");
		}
	}
}
