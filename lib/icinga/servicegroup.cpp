/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/servicegroup.hpp"
#include "icinga/servicegroup-ti.cpp"
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
	String groupName = group->GetName();

	CONTEXT("Evaluating rule for group '" + groupName + "'");

	Host::Ptr host = service->GetHost();

	ScriptFrame frame(true);
	if (group->GetScope())
		group->GetScope()->CopyTo(frame.Locals);
	frame.Locals->Set("host", host);
	frame.Locals->Set("service", service);

	if (!group->GetFilter()->Evaluate(frame).GetValue().ToBool())
		return false;

	Log(LogDebug, "ServiceGroup")
		<< "Assigning membership for group '" << groupName << "' to service '" << service->GetName() << "'";

	Array::Ptr groups = service->GetGroups();

	if (groups && !groups->Contains(groupName))
		groups->Add(groupName);

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
	std::unique_lock<std::mutex> lock(m_ServiceGroupMutex);
	return m_Members;
}

void ServiceGroup::AddMember(const Service::Ptr& service)
{
	service->AddGroup(GetName());

	std::unique_lock<std::mutex> lock(m_ServiceGroupMutex);
	m_Members.insert(service);
}

void ServiceGroup::RemoveMember(const Service::Ptr& service)
{
	std::unique_lock<std::mutex> lock(m_ServiceGroupMutex);
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
