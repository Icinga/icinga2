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

#ifndef SERVICEGROUP_H
#define SERVICEGROUP_H

#include "icinga/i2-icinga.hpp"
#include "icinga/servicegroup.thpp"
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
	mutable boost::mutex m_ServiceGroupMutex;
	std::set<Service::Ptr> m_Members;

	static bool EvaluateObjectRule(const Service::Ptr& service, const intrusive_ptr<ConfigItem>& group);
};

}

#endif /* SERVICEGROUP_H */
