/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef HOSTGROUP_H
#define HOSTGROUP_H

#include "icinga/i2-icinga.hpp"
#include "icinga/hostgroup-ti.hpp"
#include "icinga/host.hpp"

namespace icinga
{

class ConfigItem;

/**
 * An Icinga host group.
 *
 * @ingroup icinga
 */
class HostGroup final : public ObjectImpl<HostGroup>
{
public:
	DECLARE_OBJECT(HostGroup);
	DECLARE_OBJECTNAME(HostGroup);

	std::set<Host::Ptr> GetMembers() const;
	void AddMember(const Host::Ptr& host);
	void RemoveMember(const Host::Ptr& host);

	bool ResolveGroupMembership(const Host::Ptr& host, bool add = true, int rstack = 0);

	static void EvaluateObjectRules(const Host::Ptr& host);

private:
	mutable std::mutex m_HostGroupMutex;
	std::set<Host::Ptr> m_Members;

	static bool EvaluateObjectRule(const Host::Ptr& host, const intrusive_ptr<ConfigItem>& item);
};

}

#endif /* HOSTGROUP_H */
