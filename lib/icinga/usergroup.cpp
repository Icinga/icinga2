/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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
#include "icinga/usergroup.tcpp"
#include "config/objectrule.hpp"
#include "config/configitem.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/context.hpp"
#include "base/workqueue.hpp"

using namespace icinga;

REGISTER_TYPE(UserGroup);

INITIALIZE_ONCE([]() {
	ObjectRule::RegisterType("UserGroup");
});

bool UserGroup::EvaluateObjectRule(const User::Ptr& user, const ConfigItem::Ptr& group)
{
	String group_name = group->GetName();

	CONTEXT("Evaluating rule for group '" + group_name + "'");

	ScriptFrame frame(true);
	if (group->GetScope())
		group->GetScope()->CopyTo(frame.Locals);
	frame.Locals->Set("user", user);

	if (!group->GetFilter()->Evaluate(frame).GetValue().ToBool())
		return false;

	Log(LogDebug, "UserGroup")
		<< "Assigning membership for group '" << group_name << "' to user '" << user->GetName() << "'";

	Array::Ptr groups = user->GetGroups();
	groups->Add(group_name);

	return true;
}

void UserGroup::EvaluateObjectRules(const User::Ptr& user)
{
	CONTEXT("Evaluating group membership for user '" + user->GetName() + "'");

	for (const ConfigItem::Ptr& group : ConfigItem::GetItems(UserGroup::TypeInstance))
	{
		if (!group->GetFilter())
			continue;

		EvaluateObjectRule(user, group);
	}
}

std::set<User::Ptr> UserGroup::GetMembers() const
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

		for (const String& name : groups) {
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

