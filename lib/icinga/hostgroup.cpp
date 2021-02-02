/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/hostgroup.hpp"
#include "icinga/hostgroup-ti.cpp"
#include "config/objectrule.hpp"
#include "config/configitem.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/objectlock.hpp"
#include "base/context.hpp"
#include "base/workqueue.hpp"

using namespace icinga;

REGISTER_TYPE(HostGroup);

INITIALIZE_ONCE([]() {
	ObjectRule::RegisterType("HostGroup");
});

bool HostGroup::EvaluateObjectRule(const Host::Ptr& host, const ConfigItem::Ptr& group)
{
	String groupName = group->GetName();

	CONTEXT("Evaluating rule for group '" + groupName + "'");

	ScriptFrame frame(true);
	if (group->GetScope())
		group->GetScope()->CopyTo(frame.Locals);
	frame.Locals->Set("host", host);

	if (!group->GetFilter()->Evaluate(frame).GetValue().ToBool())
		return false;

	Log(LogDebug, "HostGroup")
		<< "Assigning membership for group '" << groupName << "' to host '" << host->GetName() << "'";

	Array::Ptr groups = host->GetGroups();

	if (groups && !groups->Contains(groupName))
		groups->Add(groupName);

	return true;
}

void HostGroup::EvaluateObjectRules(const Host::Ptr& host)
{
	CONTEXT("Evaluating group memberships for host '" + host->GetName() + "'");

	for (const ConfigItem::Ptr& group : ConfigItem::GetItems(HostGroup::TypeInstance))
	{
		if (!group->GetFilter())
			continue;

		EvaluateObjectRule(host, group);
	}
}

std::set<Host::Ptr> HostGroup::GetMembers() const
{
	std::unique_lock<std::mutex> lock(m_HostGroupMutex);
	return m_Members;
}

void HostGroup::AddMember(const Host::Ptr& host)
{
	host->AddGroup(GetName());

	std::unique_lock<std::mutex> lock(m_HostGroupMutex);
	m_Members.insert(host);
}

void HostGroup::RemoveMember(const Host::Ptr& host)
{
	std::unique_lock<std::mutex> lock(m_HostGroupMutex);
	m_Members.erase(host);
}

bool HostGroup::ResolveGroupMembership(const Host::Ptr& host, bool add, int rstack) {

	if (add && rstack > 20) {
		Log(LogWarning, "HostGroup")
			<< "Too many nested groups for group '" << GetName() << "': Host '"
			<< host->GetName() << "' membership assignment failed.";

		return false;
	}

	Array::Ptr groups = GetGroups();

	if (groups && groups->GetLength() > 0) {
		ObjectLock olock(groups);

		for (const String& name : groups) {
			HostGroup::Ptr group = HostGroup::GetByName(name);

			if (group && !group->ResolveGroupMembership(host, add, rstack + 1))
				return false;
		}
	}

	if (add)
		AddMember(host);
	else
		RemoveMember(host);

	return true;
}
