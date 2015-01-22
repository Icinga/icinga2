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

#include "icinga/usergroup.hpp"
#include "config/objectrule.hpp"
#include "base/dynamictype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/context.hpp"
#include "base/workqueue.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(UserGroup);

INITIALIZE_ONCE(&UserGroup::RegisterObjectRuleHandler);

void UserGroup::RegisterObjectRuleHandler(void)
{
	ObjectRule::RegisterType("UserGroup", &UserGroup::EvaluateObjectRules);
}

bool UserGroup::EvaluateObjectRuleOne(const User::Ptr& user, const ObjectRule& rule)
{
	DebugInfo di = rule.GetDebugInfo();

	std::ostringstream msgbuf;
	msgbuf << "Evaluating 'object' rule (" << di << ")";
	CONTEXT(msgbuf.str());

	Dictionary::Ptr locals = new Dictionary();
	locals->Set("__parent", rule.GetScope());
	locals->Set("user", user);

	if (!rule.EvaluateFilter(locals))
		return false;

	Log(LogDebug, "UserGroup")
	    << "Assigning membership for group '" << rule.GetName() << "' to user '" << user->GetName() << "' for rule " << di;

	String group_name = rule.GetName();
	UserGroup::Ptr group = UserGroup::GetByName(group_name);

	if (!group) {
		Log(LogCritical, "UserGroup")
		    << "Invalid membership assignment. Group '" << group_name << "' does not exist.";
		return false;
	}

	/* assign user group membership */
	group->ResolveGroupMembership(user, true);

	return true;
}

void UserGroup::EvaluateObjectRule(const ObjectRule& rule)
{
	BOOST_FOREACH(const User::Ptr& user, DynamicType::GetObjectsByType<User>()) {
		CONTEXT("Evaluating group membership in '" + rule.GetName() + "' for user '" + user->GetName() + "'");

		EvaluateObjectRuleOne(user, rule);
	}
}

void UserGroup::EvaluateObjectRules(const std::vector<ObjectRule>& rules)
{
	ParallelWorkQueue upq;

	BOOST_FOREACH(const ObjectRule& rule, rules) {
		upq.Enqueue(boost::bind(UserGroup::EvaluateObjectRule, boost::cref(rule)));
	}

	upq.Join();
}

std::set<User::Ptr> UserGroup::GetMembers(void) const
{
	boost::mutex::scoped_lock lock(m_UserGroupMutex);
	return m_Members;
}

void UserGroup::AddMember(const User::Ptr& user)
{
	user->AddGroup(GetName());

	boost::mutex::scoped_lock lock(m_UserGroupMutex);
	m_Members.insert(user);
}

void UserGroup::RemoveMember(const User::Ptr& user)
{
	boost::mutex::scoped_lock lock(m_UserGroupMutex);
	m_Members.erase(user);
}

bool UserGroup::ResolveGroupMembership(const User::Ptr& user, bool add, int rstack) {

	if (add && rstack > 20) {
		Log(LogWarning, "UserGroup")
		    << "Too many nested groups for group '" << GetName() << "': User '"
		    << user->GetName() << "' membership assignment failed.";

		return false;
	}

	Array::Ptr groups = GetGroups();

	if (groups && groups->GetLength() > 0) {
		ObjectLock olock(groups);

		BOOST_FOREACH(const String& name, groups) {
			UserGroup::Ptr group = UserGroup::GetByName(name);

			if (group && !group->ResolveGroupMembership(user, add, rstack + 1))
				return false;
		}
	}

	if (add)
		AddMember(user);
	else
		RemoveMember(user);

	return true;
}

