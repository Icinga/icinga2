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

boost::mutex ServiceGroup::m_Mutex;
map<String, vector<Service::WeakPtr> > ServiceGroup::m_MembersCache;
bool ServiceGroup::m_MembersCacheValid = true;

REGISTER_TYPE(ServiceGroup, NULL);

ServiceGroup::ServiceGroup(const Dictionary::Ptr& properties)
	: DynamicObject(properties)
{
	RegisterAttribute("display_name", Attribute_Config, &m_DisplayName);
	RegisterAttribute("notes_url", Attribute_Config, &m_NotesUrl);
	RegisterAttribute("action_url", Attribute_Config, &m_ActionUrl);
}

ServiceGroup::~ServiceGroup(void)
{
	RefreshMembersCache();
}

void ServiceGroup::OnRegistrationCompleted(void)
{
	RefreshMembersCache();
}

String ServiceGroup::GetDisplayName(void) const
{
	if (!m_DisplayName.Get().IsEmpty())
		return m_DisplayName;
	else
		return GetName();
}

String ServiceGroup::GetNotesUrl(void) const
{
	return m_NotesUrl;
}

String ServiceGroup::GetActionUrl(void) const
{
	return m_ActionUrl;
}

/**
 * @threadsafety Always.
 */
ServiceGroup::Ptr ServiceGroup::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("ServiceGroup", name);

	if (!configObject)
		BOOST_THROW_EXCEPTION(invalid_argument("ServiceGroup '" + name + "' does not exist."));

	return dynamic_pointer_cast<ServiceGroup>(configObject);
}

set<Service::Ptr> ServiceGroup::GetMembers(const ServiceGroup::Ptr& self)
{
	String name;

	{
		ObjectLock olock(self);
		name = self->GetName();
	}

	set<Service::Ptr> services;

	{
		boost::mutex::scoped_lock lock(m_Mutex);

		BOOST_FOREACH(const Service::WeakPtr& wservice, m_MembersCache[name]) {
			Service::Ptr service = wservice.lock();

			if (!service)
				continue;

			services.insert(service);
		}
	}

	return services;
}

void ServiceGroup::InvalidateMembersCache(void)
{
	{
		boost::mutex::scoped_lock lock(m_Mutex);

		if (m_MembersCacheValid)
			Utility::QueueAsyncCallback(boost::bind(&ServiceGroup::RefreshMembersCache));

		m_MembersCacheValid = false;
	}
}

void ServiceGroup::RefreshMembersCache(void)
{
	{
		boost::mutex::scoped_lock lock(m_Mutex);

		if (m_MembersCacheValid)
			return;

		m_MembersCacheValid = true;
	}

	map<String, vector<Service::WeakPtr> > newMembersCache;

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		const Service::Ptr& service = static_pointer_cast<Service>(object);
		ObjectLock olock(service);

		Dictionary::Ptr dict;
		dict = service->GetGroups();

		if (dict) {
			ObjectLock mlock(dict);
			Value servicegroup;
			BOOST_FOREACH(tie(tuples::ignore, servicegroup), dict) {
				newMembersCache[servicegroup].push_back(service);
			}
		}
	}

	boost::mutex::scoped_lock lock(m_Mutex);
	m_MembersCache.swap(newMembersCache);
}
