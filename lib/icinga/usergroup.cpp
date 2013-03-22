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

#include "icinga/usergroup.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/timer.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

static boost::mutex l_Mutex;
static std::map<String, std::vector<User::WeakPtr> > l_MembersCache;
static bool l_MembersCacheNeedsUpdate = false;
static Timer::Ptr l_MembersCacheTimer;

REGISTER_TYPE(UserGroup);

UserGroup::UserGroup(const Dictionary::Ptr& serializedUpdate)
	: DynamicObject(serializedUpdate)
{
	RegisterAttribute("display_name", Attribute_Config, &m_DisplayName);
}

UserGroup::~UserGroup(void)
{
	InvalidateMembersCache();
}

void UserGroup::OnRegistrationCompleted(void)
{
	ASSERT(!OwnsLock());

	InvalidateMembersCache();
}

String UserGroup::GetDisplayName(void) const
{
	if (!m_DisplayName.IsEmpty())
		return m_DisplayName;
	else
		return GetName();
}

UserGroup::Ptr UserGroup::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("UserGroup", name);

	if (!configObject)
		BOOST_THROW_EXCEPTION(std::invalid_argument("UserGroup '" + name + "' does not exist."));

	return dynamic_pointer_cast<UserGroup>(configObject);
}

std::set<User::Ptr> UserGroup::GetMembers(void) const
{
	std::set<User::Ptr> users;

	{
		boost::mutex::scoped_lock lock(l_Mutex);

		BOOST_FOREACH(const User::WeakPtr& wuser, l_MembersCache[GetName()]) {
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
	boost::mutex::scoped_lock lock(l_Mutex);

	if (l_MembersCacheNeedsUpdate)
		return; /* Someone else has already requested a refresh. */

	if (!l_MembersCacheTimer) {
		l_MembersCacheTimer = boost::make_shared<Timer>();
		l_MembersCacheTimer->SetInterval(0.5);
		l_MembersCacheTimer->OnTimerExpired.connect(boost::bind(&UserGroup::RefreshMembersCache));
		l_MembersCacheTimer->Start();
	}

	l_MembersCacheNeedsUpdate = true;
}

void UserGroup::RefreshMembersCache(void)
{
	{
		boost::mutex::scoped_lock lock(l_Mutex);

		if (!l_MembersCacheNeedsUpdate)
			return;

		l_MembersCacheNeedsUpdate = false;
	}

	Log(LogDebug, "icinga", "Updating UserGroup members cache.");

	std::map<String, std::vector<User::WeakPtr> > newMembersCache;

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("User")) {
		const User::Ptr& user = static_pointer_cast<User>(object);

		Array::Ptr groups = user->GetGroups();

		if (groups) {
			ObjectLock mlock(groups);
			BOOST_FOREACH(const Value& group, groups) {
				newMembersCache[group].push_back(user);
			}
		}
	}

	boost::mutex::scoped_lock lock(l_Mutex);
	l_MembersCache.swap(newMembersCache);
}
