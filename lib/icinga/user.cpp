/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/user.h"
#include "icinga/notification.h"
#include "icinga/usergroup.h"
#include "config/configcompilercontext.h"
#include "base/dynamictype.h"
#include "base/scriptfunction.h"
#include "base/utility.h"
#include "base/objectlock.h"

using namespace icinga;

REGISTER_TYPE(User);
REGISTER_SCRIPTFUNCTION(ValidateUserFilters, &User::ValidateFilters);

void User::OnConfigLoaded(void)
{
	SetNotificationTypeFilter(FilterArrayToInt(GetNotificationTypeFilterRaw(), 0));
	SetNotificationStateFilter(FilterArrayToInt(GetNotificationStateFilterRaw(), 0));

	Array::Ptr groups = GetGroups();

	if (groups) {
		ObjectLock olock(groups);

		BOOST_FOREACH(const String& name, groups) {
			UserGroup::Ptr ug = UserGroup::GetByName(name);

			if (ug)
				ug->AddMember(GetSelf());
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
				ug->RemoveMember(GetSelf());
		}
	}
}

TimePeriod::Ptr User::GetNotificationPeriod(void) const
{
	return TimePeriod::GetByName(GetNotificationPeriodRaw());
}

void User::ValidateFilters(const String& location, const Dictionary::Ptr& attrs)
{
	int sfilter = FilterArrayToInt(attrs->Get("notification_state_filter"), 0);

	if ((sfilter & ~(StateFilterUp | StateFilterDown | StateFilterOK | StateFilterWarning | StateFilterCritical | StateFilterUnknown)) != 0) {
		ConfigCompilerContext::GetInstance()->AddMessage(true, "Validation failed for " +
		    location + ": State filter is invalid.");
	}

	int tfilter = FilterArrayToInt(attrs->Get("notification_type_filter"), 0);

	if ((tfilter & ~(1 << NotificationDowntimeStart | 1 << NotificationDowntimeEnd | 1 << NotificationDowntimeRemoved |
	    1 << NotificationCustom | 1 << NotificationAcknowledgement | 1 << NotificationProblem | 1 << NotificationRecovery |
	    1 << NotificationFlappingStart | 1 << NotificationFlappingEnd)) != 0) {
		ConfigCompilerContext::GetInstance()->AddMessage(true, "Validation failed for " +
		    location + ": Type filter is invalid.");
	}
}

bool User::ResolveMacro(const String& macro, const CheckResult::Ptr&, String *result) const
{
	String key;
	Dictionary::Ptr vars;

	/* require prefix for object macros */
	if (macro.SubStr(0, 5) == "user.") {
		key = macro.SubStr(5);

		if (key.SubStr(0, 5) == "vars.") {
			vars = GetVars();
			String vars_key = key.SubStr(5);

			if (vars && vars->Contains(vars_key)) {
				*result = vars->Get(vars_key);
				return true;
			}
		} else if (key == "name") {
			*result = GetName();
			return true;
		} else if (key == "displayname") {
			*result = GetDisplayName();
			return true;
		}
	} else {
		vars = GetVars();

		if (vars && vars->Contains(macro)) {
			*result = vars->Get(macro);
			return true;
		}
	}

	return false;
}
