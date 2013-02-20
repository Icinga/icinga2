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
bool HostGroup::m_MembersCacheValid = true;

REGISTER_TYPE(HostGroup, NULL);

HostGroup::HostGroup(const Dictionary::Ptr& properties)
	: DynamicObject(properties)
{ }

String HostGroup::GetDisplayName(void) const
{
	String value = Get("display_name");

	if (!value.IsEmpty())
		return value;
	else
		return GetName();
}

String HostGroup::GetNotesUrl(void) const
{
	return Get("notes_url");
}

String HostGroup::GetActionUrl(void) const
{
	return Get("action_url");
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

		ValidateMembersCache();

		BOOST_FOREACH(const Host::WeakPtr& hst, m_MembersCache[name]) {
			Host::Ptr host = hst.lock();

			if (!host)
				continue;

			hosts.insert(host);
		}
	}

	return hosts;
}

void HostGroup::InvalidateMembersCache(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	m_MembersCacheValid = false;
	m_MembersCache.clear();
}

/**
 * @threadsafety Caller must hold m_Mutex.
 */
void HostGroup::ValidateMembersCache(void)
{
	if (m_MembersCacheValid)
		return;

	m_MembersCache.clear();

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
					Logger::Write(LogWarning, "icinga", "Host group '" + static_cast<String>(hostgroup) + "' used but not defined.");

				m_MembersCache[hostgroup].push_back(host);
			}
		}
	}

	m_MembersCacheValid = true;
}
