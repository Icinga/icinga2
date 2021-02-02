/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/usergroup.hpp"
#include "icinga/usergroup-ti.cpp"
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
	String groupName = group->GetName();

	CONTEXT("Evaluating rule for group '" + groupName + "'");

	ScriptFrame frame(true);
	if (group->GetScope())
		group->GetScope()->CopyTo(frame.Locals);
	frame.Locals->Set("user", user);

	if (!group->GetFilter()->Evaluate(frame).GetValue().ToBool())
		return false;

	Log(LogDebug, "UserGroup")
		<< "Assigning membership for group '" << groupName << "' to user '" << user->GetName() << "'";

	Array::Ptr groups = user->GetGroups();

	if (groups && !groups->Contains(groupName))
		groups->Add(groupName);

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
	std::unique_lock<std::mutex> lock(m_UserGroupMutex);
	return m_Members;
}

void UserGroup::AddMember(const User::Ptr& user)
{
	user->AddGroup(GetName());

	std::unique_lock<std::mutex> lock(m_UserGroupMutex);
	m_Members.insert(user);
}

void UserGroup::RemoveMember(const User::Ptr& user)
{
	std::unique_lock<std::mutex> lock(m_UserGroupMutex);
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

