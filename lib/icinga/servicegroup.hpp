/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef SERVICEGROUP_H
#define SERVICEGROUP_H

#include "icinga/i2-icinga.hpp"
#include "icinga/servicegroup-ti.hpp"
#include "icinga/service.hpp"

namespace icinga
{

class ConfigItem;

/**
 * An Icinga service group.
 *
 * @ingroup icinga
 */
class ServiceGroup final : public ObjectImpl<ServiceGroup>
{
public:
	DECLARE_OBJECT(ServiceGroup);
	DECLARE_OBJECTNAME(ServiceGroup);

	std::set<Service::Ptr> GetMembers() const;
	void AddMember(const Service::Ptr& service);
	void RemoveMember(const Service::Ptr& service);

	bool ResolveGroupMembership(const Service::Ptr& service, bool add = true, int rstack = 0);

	static void EvaluateObjectRules(const Service::Ptr& service);

private:
	mutable std::mutex m_ServiceGroupMutex;
	std::set<Service::Ptr> m_Members;

	static bool EvaluateObjectRule(const Service::Ptr& service, const intrusive_ptr<ConfigItem>& group);
};

}

#endif /* SERVICEGROUP_H */
