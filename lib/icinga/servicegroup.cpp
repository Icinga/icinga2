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

#include "icinga/servicegroup.hpp"
#include "icinga/servicegroup.tcpp"
#include "config/objectrule.hpp"
#include "config/configitem.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/context.hpp"
#include "base/workqueue.hpp"

using namespace icinga;

REGISTER_TYPE(ServiceGroup);

INITIALIZE_ONCE([]() {
	ObjectRule::RegisterType("ServiceGroup");
});

bool ServiceGroup::EvaluateObjectRule(const Service::Ptr& service, const ConfigItem::Ptr& group)
{
	String group_name = group->GetName();

	CONTEXT("Evaluating rule for group '" + group_name + "'");

	Host::Ptr host = service->GetHost();

	ScriptFrame frame(true);
	if (group->GetScope())
		group->GetScope()->CopyTo(frame.Locals);
	frame.Locals->Set("host", host);
	frame.Locals->Set("service", service);

	if (!group->GetFilter()->Evaluate(frame).GetValue().ToBool())
		return false;

	Log(LogDebug, "ServiceGroup")
		<< "Assigning membership for group '" << group_name << "' to service '" << service->GetName() << "'";

	Array::Ptr groups = service->GetGroups();
	groups->Add(group_name);

	return true;
}

void ServiceGroup::EvaluateObjectRules(const Service::Ptr& service)
{
	CONTEXT("Evaluating group membership for service '" + service->GetName() + "'");

	for (const ConfigItem::Ptr& group : ConfigItem::GetItems(ServiceGroup::TypeInstance))
	{
		if (!group->GetFilter())
			continue;

		EvaluateObjectRule(service, group);
	}
}

std::set<Service::Ptr> ServiceGroup::GetMembers() const
{
	boost::mutex::scoped_lock lock(m_ServiceGroupMutex);
	return m_Members;
}

void ServiceGroup::AddMember(const Service::Ptr& service)
{
	service->AddGroup(GetName());

	boost::mutex::scoped_lock lock(m_ServiceGroupMutex);
	m_Members.insert(service);
}

void ServiceGroup::RemoveMember(const Service::Ptr& service)
{
	boost::mutex::scoped_lock lock(m_ServiceGroupMutex);
	m_Members.erase(service);
}

bool ServiceGroup::ResolveGroupMembership(const Service::Ptr& service, bool add, int rstack) {

	if (add && rstack > 20) {
		Log(LogWarning, "ServiceGroup")
			<< "Too many nested groups for group '" << GetName() << "': Service '"
			<< service->GetName() << "' membership assignment failed.";

		return false;
	}

	Array::Ptr groups = GetGroups();

	if (groups && groups->GetLength() > 0) {
		ObjectLock olock(groups);

		for (const String& name : groups) {
			ServiceGroup::Ptr group = ServiceGroup::GetByName(name);

			if (group && !group->ResolveGroupMembership(service, add, rstack + 1))
				return false;
		}
	}

	if (add)
		AddMember(service);
	else
		RemoveMember(service);

	return true;
}
