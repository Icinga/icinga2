/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/user.hpp"
#include "icinga/notification.hpp"
#include "icinga/usergroup.hpp"
#include "config/configcompilercontext.hpp"
#include "base/scriptfunction.hpp"
#include "base/objectlock.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(User);
REGISTER_SCRIPTFUNCTION(ValidateUserFilters, &User::ValidateFilters);

boost::signals2::signal<void (const User::Ptr&, bool, const MessageOrigin&)> User::OnEnableNotificationsChanged;

void User::OnConfigLoaded(void)
{
	SetTypeFilter(FilterArrayToInt(GetTypes(), ~0));
	SetStateFilter(FilterArrayToInt(GetStates(), ~0));

	Array::Ptr groups = GetGroups();

	if (groups) {
		groups = groups->ShallowClone();

		ObjectLock olock(groups);

		BOOST_FOREACH(const String& name, groups) {
			UserGroup::Ptr ug = UserGroup::GetByName(name);

			if (ug)
				ug->ResolveGroupMembership(this, true);
		}
	}
}

void User::Stop(void)
{
	DynamicObject::Stop();

	Array::Ptr groups = GetGroups();

	if (groups) {
		ObjectLock olock(groups);

		BOOST_FOREACH(const String& name, groups) {
			UserGroup::Ptr ug = UserGroup::GetByName(name);

			if (ug)
				ug->ResolveGroupMembership(this, false);
		}
	}
}

void User::AddGroup(const String& name)
{
	boost::mutex::scoped_lock lock(m_UserMutex);

	Array::Ptr groups = GetGroups();

	if (groups && groups->Contains(name))
		return;

	if (!groups)
		groups = new Array();

	groups->Add(name);
}

TimePeriod::Ptr User::GetPeriod(void) const
{
	return TimePeriod::GetByName(GetPeriodRaw());
}

void User::ValidateFilters(const String& location, const Dictionary::Ptr& attrs)
{
	int sfilter = FilterArrayToInt(attrs->Get("states"), 0);

	if ((sfilter & ~(StateFilterUp | StateFilterDown | StateFilterOK | StateFilterWarning | StateFilterCritical | StateFilterUnknown)) != 0) {
		ConfigCompilerContext::GetInstance()->AddMessage(true, "Validation failed for " +
		    location + ": State filter is invalid.");
	}

	int tfilter = FilterArrayToInt(attrs->Get("types"), 0);

	if ((tfilter & ~(1 << NotificationDowntimeStart | 1 << NotificationDowntimeEnd | 1 << NotificationDowntimeRemoved |
	    1 << NotificationCustom | 1 << NotificationAcknowledgement | 1 << NotificationProblem | 1 << NotificationRecovery |
	    1 << NotificationFlappingStart | 1 << NotificationFlappingEnd)) != 0) {
		ConfigCompilerContext::GetInstance()->AddMessage(true, "Validation failed for " +
		    location + ": Type filter is invalid.");
	}
}

int User::GetModifiedAttributes(void) const
{
	int attrs = 0;

	if (GetOverrideVars())
		attrs |= ModAttrCustomVariable;

	return attrs;
}

void User::SetModifiedAttributes(int flags, const MessageOrigin& origin)
{
	if ((flags & ModAttrCustomVariable) == 0) {
		SetOverrideVars(Empty);
		OnVarsChanged(this, GetVars(), origin);
	}
}

bool User::GetEnableNotifications(void) const
{
	if (!GetOverrideEnableNotifications().IsEmpty())
		return GetOverrideEnableNotifications();
	else
		return GetEnableNotificationsRaw();
}

void User::SetEnableNotifications(bool enabled, const MessageOrigin& origin)
{
	SetOverrideEnableNotifications(enabled);

	OnEnableNotificationsChanged(this, enabled, origin);
}

