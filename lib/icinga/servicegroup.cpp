/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "i2-icinga.h"

using namespace icinga;

map<String, vector<Service::WeakPtr> > ServiceGroup::m_MembersCache;
bool ServiceGroup::m_MembersCacheValid;

REGISTER_TYPE(ServiceGroup, NULL);

String ServiceGroup::GetDisplayName(void) const
{
	String value = Get("alias");

	if (!value.IsEmpty())
		return value;
	else
		return GetName();
}

String ServiceGroup::GetNotesUrl(void) const
{
	return Get("notes_url");
}

String ServiceGroup::GetActionUrl(void) const
{
	return Get("action_url");
}

bool ServiceGroup::Exists(const String& name)
{
	return (DynamicObject::GetObject("ServiceGroup", name));
}

ServiceGroup::Ptr ServiceGroup::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("ServiceGroup", name);

	if (!configObject)
		BOOST_THROW_EXCEPTION(invalid_argument("ServiceGroup '" + name + "' does not exist."));

	return dynamic_pointer_cast<ServiceGroup>(configObject);
}

set<Service::Ptr> ServiceGroup::GetMembers(void) const
{
	set<Service::Ptr> services;

	ValidateMembersCache();

	BOOST_FOREACH(const Service::WeakPtr& svc, m_MembersCache[GetName()]) {
		Service::Ptr service = svc.lock();

		if (!service)
			continue;

		services.insert(service);
	}

	return services;
}

void ServiceGroup::InvalidateMembersCache(void)
{
	m_MembersCacheValid = false;
	m_MembersCache.clear();
}

void ServiceGroup::ValidateMembersCache(void)
{
	if (m_MembersCacheValid)
		return;

	m_MembersCache.clear();

	DynamicObject::Ptr object;
	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Service")->GetObjects()) {
		const Service::Ptr& service = static_pointer_cast<Service>(object);

		Dictionary::Ptr dict;
		dict = service->GetGroups();

		if (dict) {
			Value servicegroup;
			BOOST_FOREACH(tie(tuples::ignore, servicegroup), dict) {
				if (!ServiceGroup::Exists(servicegroup))
					Logger::Write(LogWarning, "icinga", "Service group '" + static_cast<String>(servicegroup) + "' used but not defined.");

				m_MembersCache[servicegroup].push_back(service);
			}
		}
	}

	m_MembersCacheValid = true;
}

