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

#include "icinga/hostgroup.hpp"
#include "icinga/hostgroup.tcpp"
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
	String group_name = group->GetName();

	CONTEXT("Evaluating rule for group '" + group_name + "'");

	ScriptFrame frame(true);
	if (group->GetScope())
		group->GetScope()->CopyTo(frame.Locals);
	frame.Locals->Set("host", host);

	if (!group->GetFilter()->Evaluate(frame).GetValue().ToBool())
		return false;

	Log(LogDebug, "HostGroup")
		<< "Assigning membership for group '" << group_name << "' to host '" << host->GetName() << "'";

	Array::Ptr groups = host->GetGroups();
	groups->Add(group_name);

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
	boost::mutex::scoped_lock lock(m_HostGroupMutex);
	return m_Members;
}

void HostGroup::AddMember(const Host::Ptr& host)
{
	host->AddGroup(GetName());

	boost::mutex::scoped_lock lock(m_HostGroupMutex);
	m_Members.insert(host);
}

void HostGroup::RemoveMember(const Host::Ptr& host)
{
	boost::mutex::scoped_lock lock(m_HostGroupMutex);
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
