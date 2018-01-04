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

#include "icinga/user.hpp"
#include "icinga/user.tcpp"
#include "icinga/usergroup.hpp"
#include "icinga/notification.hpp"
#include "icinga/usergroup.hpp"
#include "base/objectlock.hpp"
#include "base/exception.hpp"

using namespace icinga;

REGISTER_TYPE(User);

void User::OnConfigLoaded()
{
	ObjectImpl<User>::OnConfigLoaded();

	SetTypeFilter(FilterArrayToInt(GetTypes(), Notification::GetTypeFilterMap(), ~0));
	SetStateFilter(FilterArrayToInt(GetStates(), Notification::GetStateFilterMap(), ~0));
}

void User::OnAllConfigLoaded()
{
	ObjectImpl<User>::OnAllConfigLoaded();

	UserGroup::EvaluateObjectRules(this);

	Array::Ptr groups = GetGroups();

	if (groups) {
		groups = groups->ShallowClone();

		ObjectLock olock(groups);

		for (const String& name : groups) {
			UserGroup::Ptr ug = UserGroup::GetByName(name);

			if (ug)
				ug->ResolveGroupMembership(this, true);
		}
	}
}

void User::Stop(bool runtimeRemoved)
{
	ObjectImpl<User>::Stop(runtimeRemoved);

	Array::Ptr groups = GetGroups();

	if (groups) {
		ObjectLock olock(groups);

		for (const String& name : groups) {
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

TimePeriod::Ptr User::GetPeriod() const
{
	return TimePeriod::GetByName(GetPeriodRaw());
}

void User::ValidateStates(const Array::Ptr& value, const ValidationUtils& utils)
{
	ObjectImpl<User>::ValidateStates(value, utils);

	int filter = FilterArrayToInt(value, Notification::GetStateFilterMap(), 0);

	if (filter == -1 || (filter & ~(StateFilterUp | StateFilterDown | StateFilterOK | StateFilterWarning | StateFilterCritical | StateFilterUnknown)) != 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "states" }, "State filter is invalid."));
}

void User::ValidateTypes(const Array::Ptr& value, const ValidationUtils& utils)
{
	ObjectImpl<User>::ValidateTypes(value, utils);

	int filter = FilterArrayToInt(value, Notification::GetTypeFilterMap(), 0);

	if (filter == -1 || (filter & ~(NotificationDowntimeStart | NotificationDowntimeEnd | NotificationDowntimeRemoved |
		NotificationCustom | NotificationAcknowledgement | NotificationProblem | NotificationRecovery |
		NotificationFlappingStart | NotificationFlappingEnd)) != 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "types" }, "Type filter is invalid."));
}
