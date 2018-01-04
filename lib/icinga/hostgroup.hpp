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

#ifndef HOSTGROUP_H
#define HOSTGROUP_H

#include "icinga/i2-icinga.hpp"
#include "icinga/hostgroup.thpp"
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

	std::set<Host::Ptr> GetMembers(void) const;
	void AddMember(const Host::Ptr& host);
	void RemoveMember(const Host::Ptr& host);

	bool ResolveGroupMembership(const Host::Ptr& host, bool add = true, int rstack = 0);

	static void EvaluateObjectRules(const Host::Ptr& host);

private:
	mutable boost::mutex m_HostGroupMutex;
	std::set<Host::Ptr> m_Members;

	static bool EvaluateObjectRule(const Host::Ptr& host, const intrusive_ptr<ConfigItem>& item);
};

}

#endif /* HOSTGROUP_H */
