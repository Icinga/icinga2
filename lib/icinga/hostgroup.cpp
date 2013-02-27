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

boost::mutex HostGroup::m_Mutex;
map<String, vector<Host::WeakPtr> > HostGroup::m_MembersCache;

REGISTER_TYPE(HostGroup, NULL);

HostGroup::HostGroup(const Dictionary::Ptr& properties)
	: DynamicObject(properties)
{
	RegisterAttribute("display_name", Attribute_Config, &m_DisplayName);
	RegisterAttribute("notes_url", Attribute_Config, &m_NotesUrl);
	RegisterAttribute("action_url", Attribute_Config, &m_ActionUrl);
}

HostGroup::~HostGroup(void)
{
	RefreshMembersCache();
}

void HostGroup::OnRegistrationCompleted(void)
{
	RefreshMembersCache();
}

String HostGroup::GetDisplayName(void) const
{
	if (!m_DisplayName.IsEmpty())
		return m_DisplayName;
	else
		return GetName();
}

String HostGroup::GetNotesUrl(void) const
{
	return m_NotesUrl;
}

String HostGroup::GetActionUrl(void) const
{
	return m_ActionUrl;
}

/**
 * @threadsafety Always.
 */
bool HostGroup::Exists(const String& name)
{
	return (DynamicObject::GetObject("HostGroup", name));
}

/**
 * @threadsafety Always.
 */
HostGroup::Ptr HostGroup::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("HostGroup", name);

	if (!configObject)
		BOOST_THROW_EXCEPTION(invalid_argument("HostGroup '" + name + "' does not exist."));

	return dynamic_pointer_cast<HostGroup>(configObject);
}

set<Host::Ptr> HostGroup::GetMembers(const HostGroup::Ptr& self)
{
	String name;

	{
		ObjectLock olock(self);
		name = self->GetName();
	}

	set<Host::Ptr> hosts;

	{
		boost::mutex::scoped_lock lock(m_Mutex);

		BOOST_FOREACH(const Host::WeakPtr& whost, m_MembersCache[name]) {
			Host::Ptr host = whost.lock();

			if (!host)
				continue;

			hosts.insert(host);
		}
	}

	return hosts;
}

void HostGroup::RefreshMembersCache(void)
{
	map<String, vector<weak_ptr<Host> > > newMembersCache;

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Host")) {
		const Host::Ptr& host = static_pointer_cast<Host>(object);
		ObjectLock olock(host);

		Dictionary::Ptr dict;
		dict = host->GetGroups();

		if (dict) {
			ObjectLock mlock(dict);
			Value hostgroup;
			BOOST_FOREACH(tie(tuples::ignore, hostgroup), dict) {
				if (!HostGroup::Exists(hostgroup))
					continue;

				newMembersCache[hostgroup].push_back(host);
			}
		}
	}

	boost::mutex::scoped_lock lock(m_Mutex);
	m_MembersCache.swap(newMembersCache);
}
