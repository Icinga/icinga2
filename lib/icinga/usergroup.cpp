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

boost::mutex UserGroup::m_Mutex;
map<String, vector<User::WeakPtr> > UserGroup::m_MembersCache;
bool UserGroup::m_MembersCacheValid = true;

REGISTER_TYPE(UserGroup, NULL);

UserGroup::UserGroup(const Dictionary::Ptr& properties)
	: DynamicObject(properties)
{
	RegisterAttribute("display_name", Attribute_Config, &m_DisplayName);
	RegisterAttribute("notes_url", Attribute_Config, &m_NotesUrl);
	RegisterAttribute("action_url", Attribute_Config, &m_ActionUrl);
}

UserGroup::~UserGroup(void)
{
	InvalidateMembersCache();
}

void UserGroup::OnRegistrationCompleted(void)
{
	InvalidateMembersCache();
}

String UserGroup::GetDisplayName(void) const
{
	if (!m_DisplayName.IsEmpty())
		return m_DisplayName;
	else
		return GetName();
}

String UserGroup::GetNotesUrl(void) const
{
	return m_NotesUrl;
}

String UserGroup::GetActionUrl(void) const
{
	return m_ActionUrl;
}

/**
 * @threadsafety Always.
 */
UserGroup::Ptr UserGroup::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("UserGroup", name);

	if (!configObject)
		BOOST_THROW_EXCEPTION(invalid_argument("UserGroup '" + name + "' does not exist."));

	return dynamic_pointer_cast<UserGroup>(configObject);
}

set<User::Ptr> UserGroup::GetMembers(const UserGroup::Ptr& self)
{
	String name;

	{
		ObjectLock olock(self);
		name = self->GetName();
	}

	set<User::Ptr> users;

	{
		boost::mutex::scoped_lock lock(m_Mutex);

		BOOST_FOREACH(const User::WeakPtr& wuser, m_MembersCache[name]) {
			User::Ptr user = wuser.lock();

			if (!user)
				continue;

			users.insert(user);
		}
	}

	return users;
}

void UserGroup::InvalidateMembersCache(void)
{
	{
		boost::mutex::scoped_lock lock(m_Mutex);

		if (m_MembersCacheValid)
			Utility::QueueAsyncCallback(boost::bind(&UserGroup::RefreshMembersCache));

		m_MembersCacheValid = false;
	}
}

void UserGroup::RefreshMembersCache(void)
{
	{
		boost::mutex::scoped_lock lock(m_Mutex);

		if (m_MembersCacheValid)
			return;

		m_MembersCacheValid = true;
	}

	map<String, vector<User::WeakPtr> > newMembersCache;

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("User")) {
		const User::Ptr& user = static_pointer_cast<User>(object);
		ObjectLock olock(user);

		Dictionary::Ptr dict;
		dict = user->GetGroups();

		if (dict) {
			ObjectLock mlock(dict);
			Value UserGroup;
			BOOST_FOREACH(tie(tuples::ignore, UserGroup), dict) {
				newMembersCache[UserGroup].push_back(user);
			}
		}
	}

	boost::mutex::scoped_lock lock(m_Mutex);
	m_MembersCache.swap(newMembersCache);
}
